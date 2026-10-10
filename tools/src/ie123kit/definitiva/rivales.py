"""Nivel de los rivales (gemela de ``plugin/Sources/Nucleo/Rivales.cpp``).

Cada partido (de la historia, duelo o encuentro por el mapa) es un registro de 48 B de ``logic/games.dat``: equipo
rival (u16) en +0, 1 en +2 si el partido lleva nivel y el nivel del rival en +3. Cada jugador del rival suma a ese
nivel su ajuste de ``team.pkb`` (byte +4 de su hueco). Medido en Azahar: con el +3 de un duelo del IE2 a 77, sus
cuatro rivales salen a nivel 77 (y sus atributos pasan de 4-8 a 23-79). El plugin cambia la copia que el juego tiene
en memoria; la tabla del archivo no se toca.
"""
from __future__ import annotations

import struct

PARTIDO_PASO = 48
OFF_EQUIPO, OFF_CON_NIVEL, OFF_NIVEL = 0, 2, 3
NIVEL_MAXIMO = 99

ORIGINAL, FACILES, DUROS, MUY_DUROS, A_TU_NIVEL = range(5)


def nivel_rival(original: int, modo: int, nivel_equipo: int = 0, extra: int = 0) -> int:
    """Nivel del rival para el modo; con `extra` (ajuste más alto de sus jugadores) nadie pasa de 99."""
    if modo == FACILES:
        return max(1, (original * 7 + 5) // 10)
    if modo == DUROS:
        n = max(original + 3, (original * 5 + 2) // 4)
    elif modo == MUY_DUROS:
        n = max(original + 6, (original * 3 + 1) // 2)
    elif modo == A_TU_NIVEL:
        n = max(original, nivel_equipo)
    else:
        return original
    return min(n, max(original, NIVEL_MAXIMO - extra))


def partido_con_nivel(registro: bytes) -> bool:
    equipo = struct.unpack_from("<H", registro, OFF_EQUIPO)[0]
    return equipo != 0 and registro[OFF_CON_NIVEL] == 1 and 1 <= registro[OFF_NIVEL] <= NIVEL_MAXIMO


def ajustar_partidos(datos: bytes, modo: int, nivel_equipo: int = 0, extra: dict[int, int] | None = None) -> bytes:
    """games.dat con los niveles ajustados al modo."""
    salida = bytearray(datos)
    for off in range(0, len(datos) - PARTIDO_PASO + 1, PARTIDO_PASO):
        registro = datos[off:off + PARTIDO_PASO]
        if partido_con_nivel(registro):
            equipo = struct.unpack_from("<H", registro, OFF_EQUIPO)[0]
            salida[off + OFF_NIVEL] = nivel_rival(registro[OFF_NIVEL], modo, nivel_equipo,
                                                  (extra or {}).get(equipo, 0))
    return bytes(salida)


def misma_tabla(memoria: bytes, original: bytes) -> bool:
    """¿Es `memoria` la tabla `original` salvo los niveles?"""
    return len(memoria) == len(original) and all(
        a == b for i, (a, b) in enumerate(zip(memoria, original)) if i % PARTIDO_PASO != OFF_NIVEL)


def extra_de_equipos(team_pkb: bytes, paso: int) -> dict[int, int]:
    """Ajuste más alto de cada equipo (320 B por equipo en IE1 e IE2, 352 en IE3 y Ogro)."""
    extra: dict[int, int] = {}
    for base in range(0, len(team_pkb) - paso + 1, paso):
        equipo = struct.unpack_from("<H", team_pkb, base + 32)[0]
        for k in range(16):
            jugador = struct.unpack_from("<H", team_pkb, base + 64 + 8 * k)[0]
            if jugador:
                extra[equipo] = max(extra.get(equipo, 0), team_pkb[base + 64 + 8 * k + 4])
    return extra
