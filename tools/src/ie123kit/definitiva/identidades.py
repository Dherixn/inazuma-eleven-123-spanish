"""Quién es quién en los tres juegos: la tabla que permite pasar un jugador de un juego al siguiente.

- IE1 → IE2: misma numeración. Se acepta el mismo índice si el nombre o la cara coinciden;
  el IE2 rellena con jugadores nuevos los huecos que el IE1 dejaba reservados.
- IE2 → IE3: el IE3 renumera. Se empareja por nombre y cara, que se conserva entre juegos;
  si el nombre cambia se acepta la cara sola cuando es única, y si no, el nombre solo cuando es único.

Cada emparejamiento lleva su criterio para que la revisión manual sepa de qué fiarse.
"""
from __future__ import annotations

import unicodedata
from collections import defaultdict
from dataclasses import dataclass

from ie123kit.definitiva.fichas import Crecimiento, Identidad

NOMBRE_Y_CARA = "nombre+cara"
CARA = "cara"
NOMBRE = "nombre"
AMBIGUO = "ambiguo"
MISMO_INDICE = "mismo indice"


@dataclass(frozen=True)
class Jugador:
    identidad: Identidad
    crecimiento: Crecimiento

    @property
    def indice(self) -> int:
        return self.identidad.indice

    @property
    def nombrado(self) -> bool:
        return bool(self.identidad.nombre)


@dataclass(frozen=True)
class Equivalencia:
    ie1: int | None
    ie2: int
    ie3: int | None
    nombre: str
    posicion: str
    elemento: str
    cara: int
    criterio_ie3: str
    jugable_ie2: bool
    jugable_ie3: bool


def jugadores(identidades: list[Identidad], crecimiento: list[Crecimiento]) -> list[Jugador]:
    if len(identidades) != len(crecimiento):
        raise ValueError(f"unitbase ({len(identidades)}) y unitstat ({len(crecimiento)}) no cuadran")
    return [Jugador(i, c) for i, c in zip(identidades, crecimiento, strict=True)]


def normalizar(nombre: str) -> str:
    sin_tildes = unicodedata.normalize("NFKD", nombre).encode("ascii", "ignore").decode("ascii")
    return " ".join(sin_tildes.lower().replace(".", " ").split())


def _elegir(candidatos: list[Jugador], origen: Jugador) -> tuple[Jugador, bool]:
    """Desempata entre varios candidatos; devuelve (elegido, era_ambiguo)."""
    if len(candidatos) == 1:
        return candidatos[0], False
    misma_pos = [c for c in candidatos if c.identidad.posicion == origen.identidad.posicion]
    if len(misma_pos) == 1:
        return misma_pos[0], False
    jugables = [c for c in (misma_pos or candidatos) if c.crecimiento.jugable]
    if len(jugables) == 1:
        return jugables[0], False
    return min(misma_pos or candidatos, key=lambda c: c.indice), True


def emparejar(origen: list[Jugador], destino: list[Jugador]) -> dict[int, tuple[int, str]]:
    """Índice de origen → (índice de destino, criterio). Los que no aparecen no tienen pareja."""
    por_nombre_cara: dict[tuple[str, int], list[Jugador]] = defaultdict(list)
    por_cara: dict[int, list[Jugador]] = defaultdict(list)
    por_nombre: dict[str, list[Jugador]] = defaultdict(list)
    for j in destino:
        if not j.nombrado:
            continue
        clave = normalizar(j.identidad.nombre)
        por_nombre_cara[(clave, j.identidad.cara)].append(j)
        por_cara[j.identidad.cara].append(j)
        por_nombre[clave].append(j)

    salida: dict[int, tuple[int, str]] = {}
    for j in origen:
        if not j.nombrado:
            continue
        clave = normalizar(j.identidad.nombre)
        if candidatos := por_nombre_cara.get((clave, j.identidad.cara)):
            elegido, dudoso = _elegir(candidatos, j)
            salida[j.indice] = (elegido.indice, AMBIGUO if dudoso else NOMBRE_Y_CARA)
        elif len(por_cara.get(j.identidad.cara, ())) == 1 and j.identidad.cara:
            salida[j.indice] = (por_cara[j.identidad.cara][0].indice, CARA)
        elif len(por_nombre.get(clave, ())) == 1:
            salida[j.indice] = (por_nombre[clave][0].indice, NOMBRE)
    return salida


def emparejar_por_cara(origen: list[Jugador], destino: list[Jugador]) -> dict[int, int]:
    """Origen → destino por cara, entre jugadores jugables. Es el criterio de la recopilación de 3DS.

    En el 3DS la cara es única entre los jugables de cada juego (IE2 → IE3: 1707 con un candidato,
    15 sin ninguno) y los nombres no sirven: cada juego los guarda con otro formato. Si una cara tuviera
    varios candidatos, se elige la misma posición y, si no, la ficha más baja.
    """
    por_cara: dict[int, list[Jugador]] = defaultdict(list)
    for j in destino:
        if j.identidad.cara and j.crecimiento.jugable:
            por_cara[j.identidad.cara].append(j)
    salida = {}
    for j in origen:
        candidatos = por_cara.get(j.identidad.cara) if j.identidad.cara and j.crecimiento.jugable else None
        if not candidatos:
            continue
        misma = [c for c in candidatos if c.identidad.posicion == j.identidad.posicion]
        salida[j.indice] = min(misma or candidatos, key=lambda c: c.indice).indice
    return salida


def _provisional(nombre: bytes) -> bool:
    """Nombres de relleno («？？？»): varios personajes distintos lo comparten, no identifican a nadie."""
    return bool(nombre) and not nombre.replace(b"\x81\x48", b"").replace(b"?", b"").replace(b" ", b"")


def preferir_altas(por_ficha: dict[int, int], destino: list[Jugador], datos, altas: set[int]) -> dict[int, int]:
    """Si la ficha de destino no es una de las que la historia de ese juego une al equipo (``altas``, ver
    ``altas.py``) y otra jugable del mismo personaje sí, se usa esa (la de la misma posición y, si no, la
    primera). Así el traspaso no duplica a quien la historia vuelva a fichar. ``datos``: ``DatosJuego`` del
    destino. Gemelo de ``PreferirAltas`` (plugin/Sources/Nucleo/DatosDestino.cpp)."""
    candidatas = [j for j in destino if j.crecimiento.jugable and datos.numero[j.indice] in altas]
    salida = {}
    for o, d in por_ficha.items():
        numero = datos.numero[d]
        if numero not in altas:
            f = datos.ficha(numero)
            clave = datos.personaje(numero)
            if not _provisional(datos.nombres[f]):
                hermanos = [j for j in candidatas if datos.personaje(datos.numero[j.indice]) == clave]
                if hermanos:
                    misma = [j for j in hermanos if j.identidad.posicion == destino[f].identidad.posicion]
                    d = (misma or hermanos)[0].indice
        salida[o] = d
    return salida


def mismo_indice(ie1: list[Jugador], ie2: list[Jugador]) -> dict[int, int]:
    """IE2 → IE1 cuando la ficha del mismo índice es el mismo jugador (nombre o cara)."""
    salida = {}
    for a, b in zip(ie1, ie2, strict=False):
        if not (a.nombrado and b.nombrado):
            continue
        if normalizar(a.identidad.nombre) == normalizar(b.identidad.nombre) or a.identidad.cara == b.identidad.cara:
            salida[b.indice] = a.indice
    return salida


def ie1_a_ie2(ie1: list[Jugador], ie2: list[Jugador]) -> dict[int, int]:
    """IE1 → IE2 para el traspaso: el mismo índice si es el mismo jugador; si no, por nombre y cara.

    En las ROM NDS es siempre el mismo índice: los cambios de nombre («Doug Lee» → «Pearson Gaze»,
    «T. Fielding» → «Tammy Fielding») conservan la cara y la posición, y ningún hueco del IE1 pasa a
    otro jugador. La tabla explícita evita depender de eso en el 3DS.
    """
    salida = {a: b for b, a in mismo_indice(ie1, ie2).items()}
    resto = [j for j in ie1 if j.indice not in salida]
    salida.update({a: b for a, (b, _) in emparejar(resto, ie2).items()})
    return salida


def tabla(ie1: list[Jugador], ie2: list[Jugador], ie3: list[Jugador]) -> list[Equivalencia]:
    hacia_ie1 = mismo_indice(ie1, ie2)
    hacia_ie3 = emparejar(ie2, ie3)
    salida = []
    for j in ie2:
        if not j.nombrado:
            continue
        destino, criterio = hacia_ie3.get(j.indice, (None, ""))
        salida.append(Equivalencia(
            ie1=hacia_ie1.get(j.indice),
            ie2=j.indice,
            ie3=destino,
            nombre=j.identidad.nombre,
            posicion=j.identidad.posicion,
            elemento=j.identidad.elemento,
            cara=j.identidad.cara,
            criterio_ie3=criterio,
            jugable_ie2=j.crecimiento.jugable,
            jugable_ie3=destino is not None and ie3[destino].crecimiento.jugable,
        ))
    return salida


COLUMNAS = ("ie1", "ie2", "ie3", "nombre", "posicion", "elemento", "cara", "criterio_ie3", "jugable_ie2", "jugable_ie3")


def a_filas(equivalencias: list[Equivalencia]) -> list[list[str]]:
    def celda(v) -> str:
        if v is None:
            return ""
        if isinstance(v, bool):
            return "1" if v else "0"
        return str(v)
    return [[celda(getattr(e, c)) for c in COLUMNAS] for e in equivalencias]
