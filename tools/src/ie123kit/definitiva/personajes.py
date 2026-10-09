"""Jugadores nuevos a partir de fichas existentes (primer caso: Aiden Froste).

Un jugador nuevo ocupa una ficha **libre**: sin nombre propio (los huecos reservados llevan «未定»,
«sin decidir»), con estadísticas a cero y sin aparecer en ningún equipo de ``team.pkb``. Se le copia la
ficha de identidad y la de crecimiento de un jugador parecido y se le pone su nombre. El ``unitstat`` del
IE3 se copia ofuscado tal cual: la ofuscación no depende de la posición.

Aiden Froste: en el IE2, la ficha 1513 es Shawn en modo delantero, la personalidad de Atsuya (Ventisca
Eterna, cara 0x0921); en el IE3 es la 1169. Ver ``docs/definitiva/HALLAZGOS_NDS.md`` §4.

Pendiente aparte: cómo se ficha (``ScoutData``, mapa de contactos o evento) y su descripción propia.
"""
from __future__ import annotations

import struct
from collections.abc import Callable

from ie123kit.definitiva.fichas import Perfil, leer_crecimiento, leer_identidades

EQUIPO_PASO = 320
EQUIPO_JUGADORES = 64
EQUIPO_MAX_JUGADORES = 16
RESERVADO = "未定".encode("shift_jis")


def jugadores_en_equipos(team_pkb: bytes) -> set[int]:
    """Números de jugador que aparecen en alguna plantilla de ``team.pkb`` (16 × 8 B desde +64)."""
    usados = set()
    for base in range(0, len(team_pkb) - EQUIPO_PASO + 1, EQUIPO_PASO):
        for k in range(EQUIPO_MAX_JUGADORES):
            jugador = struct.unpack_from("<H", team_pkb, base + EQUIPO_JUGADORES + 8 * k)[0]
            if jugador:
                usados.add(jugador)
    return usados


def fichas_libres(unitbase: bytes, unitstat: bytes, team_pkb: bytes, perfil: Perfil) -> list[int]:
    """Fichas reservadas («未定»), con crecimiento a cero y fuera de todo equipo."""
    usados = jugadores_en_equipos(team_pkb)
    crecimiento = leer_crecimiento(unitstat, perfil)
    libres = []
    for indice in range(len(unitbase) // perfil.base_paso):
        nombre = unitbase[indice * perfil.base_paso:indice * perfil.base_paso + 32].split(b"\0")[0]
        if indice and nombre == RESERVADO and indice not in usados and crecimiento[indice].pe == (0, 0, 0):
            libres.append(indice)
    return libres


def clonar_ficha(
    unitbase: bytes,
    unitstat: bytes,
    perfil: Perfil,
    origen: int,
    destino: int,
    nombre: str,
    nombre_corto: str,
    codificar: Callable[[str], bytes],
) -> tuple[bytes, bytes]:
    """Copia la ficha ``origen`` en ``destino`` con otro nombre. ``codificar`` es el de la plataforma.

    Los nombres no se truncan: si no caben con su terminador (32 B y 16 B), es un error.
    """
    base = bytearray(unitbase)
    stat = bytearray(unitstat)
    total = len(unitbase) // perfil.base_paso
    for indice in (origen, destino):
        if not 0 <= indice < total:
            raise IndexError(f"ficha {indice} fuera de la tabla ({total})")
    if origen == destino:
        raise ValueError("origen y destino son la misma ficha")

    b0, b1 = origen * perfil.base_paso, destino * perfil.base_paso
    base[b1:b1 + perfil.base_paso] = unitbase[b0:b0 + perfil.base_paso]
    for offset, ancho, texto in ((0, 32, nombre), (32, 16, nombre_corto)):
        cod = codificar(texto)
        if len(cod) >= ancho:
            raise ValueError(f"«{texto}» no cabe en {ancho} B con su terminador")
        base[b1 + offset:b1 + offset + ancho] = cod + bytes(ancho - len(cod))

    s0, s1 = origen * perfil.stat_paso, destino * perfil.stat_paso
    stat[s1:s1 + perfil.stat_paso] = unitstat[s0:s0 + perfil.stat_paso]

    # Comprobación: la ficha nueva lee igual que la de origen salvo el nombre.
    ids = leer_identidades(bytes(base), perfil)
    if (ids[destino].cara, ids[destino].posicion) != (ids[origen].cara, ids[origen].posicion):
        raise AssertionError("la ficha clonada no conserva la identidad de la de origen")
    return bytes(base), bytes(stat)
