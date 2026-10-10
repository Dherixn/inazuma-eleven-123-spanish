"""Altas al equipo que hace la historia de cada juego, leídas de sus scripts de evento.

Cada juego guarda sus eventos en ``eve.pkh`` (índice «PackNum») + ``eve.pkb`` (un SSD comprimido en LZ10 por
evento). La instrucción ``OP_ALTA`` une un jugador al equipo con su número, nivel, dorsal...: en el prólogo
del IE3, ``[3170, 30, 10, 0, 1]`` trae a Paolo Bianchi con nivel 30 y el dorsal 10.

Sirve para que el traspaso use la ficha que la historia del juego siguiente vuelve a fichar (en el IE3, Jude
es la 30 y no la 1097 de su cara del IE2) y no la duplique. Gemelo de ``plugin/Sources/Nucleo/Eventos.cpp``.
"""
from __future__ import annotations

from collections.abc import Callable

from ie123kit.nucleo.compresion.lz10 import decompress
from ie123kit.nucleo.eventos.instrucciones import instrucciones
from ie123kit.nucleo.eventos.packnum import parse_index

OP_ALTA = 16446
SIN_EVENTO = 0xFFFFFFFF
# El IE2 tiene los eventos de Ventisca aparte; Fuego Explosivo usa los de Rayo Celeste.
CARPETAS = {
    "inazuma1": ("inazuma1/data_iz/script",),
    "inazuma2": ("inazuma2/data_iz/script", "inazuma2/data_iz_blizzard/script"),
    "inazuma3": ("inazuma3/data_iz/script",),
    "inazuma3_ogre": ("inazuma3_ogre/data_iz/script",),
}


def altas_de_script(ssd: bytes) -> list[int]:
    """Números que une al equipo un script SSD ya descomprimido."""
    return [args[0][1] & 0xFFFF for op, args in instrucciones(ssd).values() if op == OP_ALTA and args]


def altas(leer: Callable[[str], bytes | None], juego: str) -> set[int]:
    """Altas de todos los eventos del juego. ``leer(ruta)`` devuelve el archivo o None si no está."""
    salida: set[int] = set()
    for carpeta in CARPETAS[juego]:
        pkh, pkb = leer(f"{carpeta}/eve.pkh"), leer(f"{carpeta}/eve.pkb")
        if pkh is None or pkb is None:
            continue
        for eid, off, medida in parse_index(pkh):
            if eid == SIN_EVENTO or not medida or off + medida > len(pkb):
                continue
            salida.update(altas_de_script(decompress(pkb[off:off + medida])))
    return salida
