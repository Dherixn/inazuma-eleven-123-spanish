"""Traspaso de una partida al juego siguiente: tus fichajes, sus técnicas y su experiencia.

Lo que lo hace posible (medido en la recopilación de 3DS, ver ``docs/definitiva/``):

- Número de jugador: se traduce por la cara, que se conserva entre juegos (tablas de equivalencias).
- Número de técnica: es el mismo en los tres juegos.
- **Se hereda el nivel, no la EXP**: cada juego tiene sus curvas (el IE1 de 3DS, 56; IE2 e IE3, 16 y
  distintas entre sí). La EXP de destino es la mínima de ese nivel con la curva del jugador en el juego
  nuevo (``exp_de_nivel``). El juego recalcula atributos y PE/PT máximos a partir del nivel.
- Técnicas: las que el jugador sabe a su nivel **en el juego nuevo** (``aprendidas``) más las que traía.

Modos de herencia (elige el jugador):

- ``completa``: cada uno llega con su nivel.
- ``equilibrada``: igual, pero nadie llega por encima de ``nivel_tope`` (el punto de la historia en que
  empieza el juego nuevo).
- ``solo_fichajes``: llegan los jugadores, todos a ``nivel_fichaje``.

Si el jugador ya está en la partida de destino, se queda con lo mejor de los dos: el nivel más alto y
los huecos de técnica vacíos rellenos. «Ya está» se decide por el **personaje** (``clave_destino``,
p. ej. ``DatosJuego.personaje``): uno puede tener varias fichas en un juego con caras distintas (el IE2
tiene a Jude del Royal y a Jude del Raimon) y así no se duplica.
"""
from __future__ import annotations

from collections.abc import Callable, Mapping, Sequence
from dataclasses import dataclass, field

from ie123kit.definitiva.partida import JugadorPartida, Partida

COMPLETA = "completa"
EQUILIBRADA = "equilibrada"
SOLO_FICHAJES = "solo_fichajes"
MODOS = (COMPLETA, EQUILIBRADA, SOLO_FICHAJES)


@dataclass(frozen=True)
class Opciones:
    modo: str = EQUILIBRADA
    nivel_tope: int | None = None
    nivel_fichaje: int | None = None
    # (id del jugador en el juego de destino, nivel) -> EXP mínima de ese nivel.
    exp_de_nivel: Callable[[int, int], int] | None = None
    # (id del jugador en el juego de destino, nivel) -> técnicas que ya sabe a ese nivel en ese juego.
    aprendidas: Callable[[int, int], Sequence[int]] | None = None
    # id del jugador en el juego de destino -> clave del personaje. Sin ella, «ya está» va por el id.
    clave_destino: Callable[[int], object] | None = None


@dataclass
class Informe:
    traspasados: list[tuple[int, int]] = field(default_factory=list)
    mejorados: list[tuple[int, int]] = field(default_factory=list)
    sin_pareja: list[int] = field(default_factory=list)
    sin_sitio: list[int] = field(default_factory=list)
    sin_plantilla: list[int] = field(default_factory=list)
    con_plantilla: list[int] = field(default_factory=list)


def exp_minima(curvas: list[list[int]], curva: int, nivel: int) -> int:
    """EXP acumulada para tener ``nivel`` con la curva dada (nivel 1 = 0)."""
    if not 1 <= nivel <= 99:
        raise ValueError(f"nivel fuera de rango: {nivel}")
    return 0 if nivel == 1 else curvas[curva][nivel - 2]


def validar_opciones(op: Opciones) -> None:
    """Falla antes de tocar ninguna partida si al modo le falta algo."""
    if op.modo not in MODOS:
        raise ValueError(f"modo desconocido: {op.modo!r} (válidos: {', '.join(MODOS)})")
    if op.modo == SOLO_FICHAJES and op.nivel_fichaje is None:
        raise ValueError("el modo solo_fichajes necesita nivel_fichaje")
    necesita_exp = op.modo == SOLO_FICHAJES or (op.modo == EQUILIBRADA and op.nivel_tope is not None)
    if necesita_exp and op.exp_de_nivel is None:
        raise ValueError(f"el modo {op.modo} necesita exp_de_nivel para recalcular la EXP")


def _nivel_y_exp(j: JugadorPartida, id_destino: int, op: Opciones) -> tuple[int, int]:
    if op.modo == COMPLETA:
        nivel = j.nivel
    elif op.modo == EQUILIBRADA:
        nivel = j.nivel if op.nivel_tope is None else min(j.nivel, op.nivel_tope)
    else:
        nivel = op.nivel_fichaje
    # Sin curvas (pruebas dentro de un mismo juego) se conserva la EXP; con ellas siempre se recalcula.
    exp = j.exp if op.exp_de_nivel is None else op.exp_de_nivel(id_destino, nivel)
    return nivel, exp


def _anadir_tecnicas(tecnicas: list[int], niveles: list[int], nuevas) -> None:
    """Añade (técnica, nivel de la técnica) en huecos libres; si ya está, se queda el nivel mayor."""
    for tec, niv in nuevas:
        if not tec:
            continue
        if tec in tecnicas:
            k = tecnicas.index(tec)
            niveles[k] = max(niveles[k], niv)
        elif 0 in tecnicas:
            k = tecnicas.index(0)
            tecnicas[k], niveles[k] = tec, niv


def _tecnicas_heredadas(op: Opciones, id_destino: int, nivel: int, j: JugadorPartida):
    aprendidas = [(t, 0) for t in op.aprendidas(id_destino, nivel)] if op.aprendidas else []
    return aprendidas + list(zip(j.tecnicas, j.niveles_tecnica, strict=True))


def _fundir(actual: JugadorPartida, nivel: int, exp: int, nuevas) -> JugadorPartida:
    tecnicas, niveles = list(actual.tecnicas), list(actual.niveles_tecnica)
    _anadir_tecnicas(tecnicas, niveles, nuevas)
    if (nivel, exp) > (actual.nivel, actual.exp):
        return actual.con(exp=exp, nivel=nivel, tecnicas=tuple(tecnicas), niveles_tecnica=tuple(niveles))
    return actual.con(tecnicas=tuple(tecnicas), niveles_tecnica=tuple(niveles))


def traspasar(origen: Partida, destino: Partida, mapa_ids: Mapping[int, int] | None, op: Opciones) -> Informe:
    """Copia los fichajes de ``origen`` en ``destino`` (en memoria; luego hay que sellar).

    ``mapa_ids`` traduce el número de jugador del juego de origen al de destino; ``None`` para
    IE1 → IE2, que comparten numeración.
    """
    validar_opciones(op)
    informe = Informe()
    mismo_formato = origen.formato.jugador_paso == destino.formato.jugador_paso
    destino_jugadores = destino.jugadores()
    clave = op.clave_destino or (lambda id_: id_)
    posicion = {clave(j.id): k for k, j in enumerate(destino_jugadores)}
    plantilla = destino_jugadores[0].crudo if destino_jugadores else None

    for j in origen.jugadores():
        id_destino = j.id if mapa_ids is None else mapa_ids.get(j.id)
        if id_destino is None:
            informe.sin_pareja.append(j.id)
            continue
        if clave(id_destino) in posicion:
            k = posicion[clave(id_destino)]
            actual = destino.jugadores()[k]
            # Se mejora la ficha que ya usa el juego (su id), no la que dice la tabla.
            nivel, exp = _nivel_y_exp(j, actual.id, op)
            nuevas = _tecnicas_heredadas(op, actual.id, max(nivel, actual.nivel), j)
            destino.poner_jugador(k, _fundir(actual, nivel, exp, nuevas))
            informe.mejorados.append((j.id, id_destino))
            continue
        nivel, exp = _nivel_y_exp(j, id_destino, op)
        if destino.num_jugadores >= destino.formato.max_jugadores:
            informe.sin_sitio.append(j.id)
            continue
        if mismo_formato:
            base = j.crudo
        elif plantilla is not None:
            # Los formatos difieren (IE2 0x70 → IE3 0x6C): los campos aún sin interpretar salen de
            # una ficha del propio juego de destino.
            base = plantilla
            informe.con_plantilla.append(j.id)
        else:
            # Partida de destino vacía y formatos distintos: no hay de dónde sacar los campos que aún
            # no se interpretan, y no se inventan.
            informe.sin_plantilla.append(j.id)
            continue
        tecnicas, niveles = [0] * len(j.tecnicas), [0] * len(j.tecnicas)
        _anadir_tecnicas(tecnicas, niveles, _tecnicas_heredadas(op, id_destino, nivel, j))
        ficha = JugadorPartida(exp=exp, id=id_destino, tecnicas=tuple(tecnicas), niveles_tecnica=tuple(niveles),
                               nivel=nivel, crudo=base)
        posicion[clave(id_destino)] = destino.anadir_jugador(ficha)
        informe.traspasados.append((j.id, id_destino))
    return informe
