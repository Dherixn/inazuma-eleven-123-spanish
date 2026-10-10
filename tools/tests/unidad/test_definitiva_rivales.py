"""Edición definitiva: nivel de los rivales (los mismos casos que plugin/pruebas/rivales_pc.cpp)."""

import struct

from ie123kit.definitiva import rivales as rv


def partido(equipo: int, nivel: int, con_nivel: int = 1) -> bytes:
    r = bytearray(48)
    struct.pack_into("<HBB", r, 0, equipo, con_nivel, nivel)
    r[4:8] = b"\x2d\x00\x00\x00"
    r[40] = equipo & 0xFF  # algo distinto en cada registro
    return bytes(r)


def tabla() -> bytes:
    return b"".join([partido(0, 0), partido(31, 5), partido(70, 1), partido(2, 20), partido(10, 80),
                     partido(42, 254), partido(5, 9, con_nivel=0), partido(628, 70)])


def test_niveles_por_modo():
    assert [rv.nivel_rival(20, m) for m in range(5)] == [20, 14, 25, 30, 20]
    assert rv.nivel_rival(1, rv.FACILES) == 1
    assert rv.nivel_rival(1, rv.DUROS) == 4 and rv.nivel_rival(1, rv.MUY_DUROS) == 7
    assert rv.nivel_rival(5, rv.A_TU_NIVEL, 33) == 33 and rv.nivel_rival(50, rv.A_TU_NIVEL, 33) == 50
    assert rv.nivel_rival(80, rv.MUY_DUROS) == 99
    # Con el ajuste de sus jugadores nadie pasa de 99, pero lo original no baja.
    assert rv.nivel_rival(70, rv.MUY_DUROS, extra=30) == 70
    assert rv.nivel_rival(60, rv.MUY_DUROS, extra=30) == 69
    assert rv.nivel_rival(40, rv.A_TU_NIVEL, 90, extra=16) == 83


def test_ajustar_solo_toca_niveles():
    t = tabla()
    extra = {628: 30, 2: 5}
    duros = rv.ajustar_partidos(t, rv.DUROS, 0, extra)
    niveles = [duros[i * 48 + 3] for i in range(len(t) // 48)]
    assert niveles == [0, 8, 4, 25, 99, 254, 9, 70]
    assert rv.misma_tabla(duros, t)
    assert all(duros[i] == t[i] for i in range(len(t)) if i % 48 != 3)
    assert rv.ajustar_partidos(t, rv.ORIGINAL) == t
    a_tu_nivel = rv.ajustar_partidos(t, rv.A_TU_NIVEL, 33, extra)
    assert [a_tu_nivel[i * 48 + 3] for i in range(len(t) // 48)] == [0, 33, 33, 33, 80, 254, 9, 70]


def test_misma_tabla_detecta_otra_cosa():
    t = tabla()
    otra = bytearray(t)
    otra[48] = 99  # equipo distinto
    assert not rv.misma_tabla(bytes(otra), t)
    assert not rv.misma_tabla(t[:-1], t)


def test_extra_de_equipos():
    pkb = bytearray(352 * 2)
    struct.pack_into("<H", pkb, 32, 26)
    for k, (jugador, ajuste) in enumerate([(2101, 8), (2102, 6), (0, 40)]):
        struct.pack_into("<HHBB", pkb, 64 + 8 * k, jugador, 0x0101 * (k + 1), ajuste, 1)
    struct.pack_into("<H", pkb, 352 + 32, 16)
    struct.pack_into("<HHBB", pkb, 352 + 64, 1, 0x0101, 0, 1)
    assert rv.extra_de_equipos(bytes(pkb), 352) == {26: 8, 16: 0}
