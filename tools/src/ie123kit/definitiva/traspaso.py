"""Traspaso de una partida al juego siguiente: tus fichajes, sus técnicas y su experiencia.

Lo que lo hace posible (medido en las ROM NDS, ver ``docs/definitiva/HALLAZGOS_NDS.md``):

- Número de jugador: IE1 e IE2 comparten numeración; del IE2 al IE3 se usa la tabla de equivalencias.
- Número de técnica: es el mismo en los tres juegos.
- EXP: las curvas de ``lvup_exp.dat`` son idénticas en los tres juegos; el nivel sale de la EXP.

Modos de herencia (elige el jugador):

- ``completa``: nivel y EXP tal cual.
- ``equilibrada``: igual, pero nadie llega por encima de ``nivel_tope`` (el punto de la historia en que
  empieza el juego nuevo).
- ``solo_fichajes``: llegan los jugadores, todos a ``nivel_fichaje``.

Si el jugador ya está en la partida de destino, se queda con lo mejor de los dos: más EXP y los huecos
de técnica vacíos rellenos con las técnicas del juego anterior.
"""
from __future__ import annotations

from collections.abc import Callable, Mapping
from dataclasses import dataclass, field

from ie123kit.definitiva.partida import MAX_JUGADORES, JugadorPartida, Partida

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
        return j.nivel, j.exp
    if op.modo == EQUILIBRADA:
        if op.nivel_tope is None or j.nivel <= op.nivel_tope:
            return j.nivel, j.exp
        return op.nivel_tope, _exp(op, id_destino, op.nivel_tope)
    return op.nivel_fichaje, _exp(op, id_destino, op.nivel_fichaje)


def _exp(op: Opciones, id_destino: int, nivel: int) -> int:
    return op.exp_de_nivel(id_destino, nivel)


def _fundir(actual: JugadorPartida, nuevo: JugadorPartida) -> JugadorPartida:
    tecnicas = list(actual.tecnicas)
    niveles = list(actual.niveles_tecnica)
    for tec, niv in zip(nuevo.tecnicas, nuevo.niveles_tecnica, strict=True):
        if not tec:
            continue
        if tec in tecnicas:
            k = tecnicas.index(tec)
            niveles[k] = max(niveles[k], niv)
        elif 0 in tecnicas:
            k = tecnicas.index(0)
            tecnicas[k], niveles[k] = tec, niv
    if nuevo.exp > actual.exp:
        return actual.con(exp=nuevo.exp, nivel=nuevo.nivel, tecnicas=tuple(tecnicas), niveles_tecnica=tuple(niveles))
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
    posicion = {j.id: k for k, j in enumerate(destino_jugadores)}
    plantilla = destino_jugadores[0].crudo if destino_jugadores else None

    for j in origen.jugadores():
        id_destino = j.id if mapa_ids is None else mapa_ids.get(j.id)
        if id_destino is None:
            informe.sin_pareja.append(j.id)
            continue
        nivel, exp = _nivel_y_exp(j, id_destino, op)
        if id_destino in posicion:
            k = posicion[id_destino]
            nuevo = j.con(id=id_destino, nivel=nivel, exp=exp)
            destino.poner_jugador(k, _fundir(destino.jugadores()[k], nuevo))
            informe.mejorados.append((j.id, id_destino))
            continue
        if destino.num_jugadores >= MAX_JUGADORES:
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
        ficha = JugadorPartida(exp=exp, id=id_destino, tecnicas=j.tecnicas, niveles_tecnica=j.niveles_tecnica,
                               nivel=nivel, crudo=base)
        posicion[id_destino] = destino.anadir_jugador(ficha)
        informe.traspasados.append((j.id, id_destino))
    return informe
