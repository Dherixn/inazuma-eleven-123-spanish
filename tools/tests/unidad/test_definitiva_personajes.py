"""Edición definitiva: fichas libres y clonado de jugadores (sin ROM)."""

import struct

import pytest

from ie123kit.definitiva import fichas, personajes


def _tabla(perfil, filas):
    """filas: (nombre_bytes, cara, pe_max)"""
    base, stat = bytearray(), bytearray()
    for nombre, cara, pe_max in filas:
        reg = bytearray(perfil.base_paso)
        reg[0:len(nombre)] = nombre
        struct.pack_into("<H", reg, perfil.off_cara, cara)
        reg[perfil.off_posicion] = 0x80
        base += reg
        s = bytearray(perfil.stat_paso)
        if pe_max:
            struct.pack_into("<HHB", s, 0, 50, pe_max, 2)
            struct.pack_into("<HH", s, 44, 0x1F0, 1)
        stat += fichas.ofuscar(bytes(s)) if perfil.stat_ofuscado else s
    return bytes(base), bytes(stat)


def _equipos(*plantillas):
    salida = bytearray()
    for jugadores in plantillas:
        reg = bytearray(personajes.EQUIPO_PASO)
        for k, j in enumerate(jugadores):
            struct.pack_into("<H", reg, personajes.EQUIPO_JUGADORES + 8 * k, j)
        salida += reg
    return bytes(salida)


def test_jugadores_en_equipos():
    assert personajes.jugadores_en_equipos(_equipos([1, 2, 3], [3, 9])) == {1, 2, 3, 9}


def test_fichas_libres_exige_reservada_vacia_y_sin_equipo():
    p = fichas.PERFILES["nds.ie2"]
    base, stat = _tabla(p, [(b"", 0, 0), (b"Shawn Froste", 0x921, 180), (personajes.RESERVADO, 0, 0),
                            (personajes.RESERVADO, 0, 0), (personajes.RESERVADO, 0, 150)])
    assert personajes.fichas_libres(base, stat, _equipos([1, 3]), p) == [2]


@pytest.mark.parametrize("clave", ["nds.ie2", "nds.ie3"])
def test_clonar_ficha_conserva_identidad_y_crecimiento(clave):
    p = fichas.PERFILES[clave]
    base, stat = _tabla(p, [(b"Shawn Froste", 0x921, 180), (personajes.RESERVADO, 0, 0)])
    nb, ns = personajes.clonar_ficha(base, stat, p, 0, 1, "Aiden Froste", "Aiden", lambda t: t.encode("latin-1"))
    nuevo = fichas.leer_identidades(nb, p)[1]
    assert (nuevo.nombre, nuevo.nombre_corto, nuevo.cara, nuevo.posicion) == ("Aiden Froste", "Aiden", 0x921, "DL")
    original, clon = fichas.leer_crecimiento(stat, p)[0], fichas.leer_crecimiento(ns, p)[1]
    assert (clon.pe, clon.pt, clon.atributos, clon.tecnicas) == (
        original.pe, original.pt, original.atributos, original.tecnicas)
    assert nb[:p.base_paso] == base[:p.base_paso]  # el origen no se toca


def test_clonar_ficha_no_trunca_ni_se_sale():
    p = fichas.PERFILES["nds.ie2"]
    base, stat = _tabla(p, [(b"Shawn Froste", 0x921, 180), (personajes.RESERVADO, 0, 0)])
    with pytest.raises(ValueError, match="no cabe"):
        personajes.clonar_ficha(base, stat, p, 0, 1, "A" * 32, "Aiden", lambda t: t.encode())
    with pytest.raises(IndexError):
        personajes.clonar_ficha(base, stat, p, 0, 5, "Aiden Froste", "Aiden", lambda t: t.encode())
    with pytest.raises(ValueError, match="misma ficha"):
        personajes.clonar_ficha(base, stat, p, 0, 0, "Aiden Froste", "Aiden", lambda t: t.encode())
