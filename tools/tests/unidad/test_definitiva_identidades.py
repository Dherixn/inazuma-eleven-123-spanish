"""Edición definitiva: lectura de fichas y emparejamiento de jugadores entre juegos (sin ROM)."""

import random
import struct

import pytest

from ie123kit.definitiva import fichas, identidades
from ie123kit.ie3.comun.items import desofuscar_record


def _ficha_base(perfil, nombre, cara, pos_nibble=0x8, elemento=3):
    reg = bytearray(perfil.base_paso)
    reg[0:len(nombre)] = nombre.encode("latin-1")
    struct.pack_into("<H", reg, perfil.off_cara, cara)
    reg[perfil.off_posicion] = pos_nibble << 4
    reg[perfil.off_elemento] = elemento
    return bytes(reg)


def _ficha_stat(perfil, pe_max=150, tecnicas=((10, 1), (20, 100))):
    reg = bytearray(perfil.stat_paso)
    struct.pack_into("<HHB", reg, 0, 30, pe_max, 2)
    struct.pack_into("<HHB", reg, 8, 40, 120, 1)
    for k in range(7):
        reg[16 + 4 * k:19 + 4 * k] = bytes((10 + k, 60 + k, k % 5 + 1))
    for k, (tec, nivel) in enumerate(tecnicas):
        struct.pack_into("<H", reg, 44 + 4 * k, tec)
        if perfil.nivel_u8:
            reg[46 + 4 * k] = nivel
        else:
            struct.pack_into("<H", reg, 46 + 4 * k, nivel)
    return fichas.ofuscar(bytes(reg)) if perfil.stat_ofuscado else bytes(reg)


def _jugadores(perfil, filas):
    base = b"".join(_ficha_base(perfil, *f[:2], *f[2:4]) for f in filas)
    stat = b"".join(_ficha_stat(perfil, pe_max=(f[4] if len(f) > 4 else 150)) for f in filas)
    return identidades.jugadores(fichas.leer_identidades(base, perfil), fichas.leer_crecimiento(stat, perfil))


def test_desofuscar_es_reversible():
    rnd = random.Random(7)
    for tam in (44, 72, 104):
        crudo = bytes(rnd.randrange(256) for _ in range(tam))
        assert fichas.ofuscar(fichas.desofuscar(crudo)) == crudo
        assert fichas.desofuscar(fichas.ofuscar(crudo)) == crudo


def test_desofuscar_coincide_con_item_dat_del_ie3():
    rnd = random.Random(11)
    crudo = bytes(rnd.randrange(256) for _ in range(44))
    assert fichas.desofuscar(crudo) == desofuscar_record(crudo)


@pytest.mark.parametrize("clave", ["nds.ie1", "nds.ie3"])
def test_leer_fichas(clave):
    perfil = fichas.PERFILES[clave]
    [j] = _jugadores(perfil, [("Mark Evans", 1, 0x2, 4)])
    assert j.identidad.nombre == "Mark Evans"
    assert j.identidad.cara == 1
    assert j.identidad.posicion == "PO"
    assert j.identidad.elemento == "montaña"
    assert j.crecimiento.pe == (30, 150, 2)
    assert j.crecimiento.atributos[0] == (10, 60, 1)
    assert j.crecimiento.tecnicas[:2] == ((10, 1), (20, 100))
    assert j.crecimiento.jugable


def test_relleno_japones_no_tiene_nombre():
    perfil = fichas.PERFILES["nds.ie1"]
    reg = bytearray(_ficha_base(perfil, "", 5))
    reg[0:6] = "円堂".encode("shift_jis") + b"\0\0"
    assert fichas.leer_identidades(bytes(reg), perfil)[0].nombre == ""


def test_tamano_incorrecto_falla():
    with pytest.raises(ValueError):
        fichas.leer_identidades(b"\0" * 95, fichas.PERFILES["nds.ie1"])


def test_emparejar_por_nombre_y_cara_y_por_cara_sola():
    ie2 = _jugadores(fichas.PERFILES["nds.ie2"], [
        ("Mark Evans", 1, 0x2, 4),
        ("Grent", 900, 0x4, 1),      # alias alienígena: el IE3 le da su nombre real
        ("Shawn Froste", 921, 0x8, 1),
    ])
    ie3 = _jugadores(fichas.PERFILES["nds.ie3"], [
        ("Shawn Froste", 919, 0x4, 1),
        ("Grant Cook", 900, 0x4, 1),
        ("Mark Evans", 1, 0x2, 4),
        ("Shawn Froste", 921, 0x8, 1),
    ])
    pares = identidades.emparejar(ie2, ie3)
    assert pares[0] == (2, identidades.NOMBRE_Y_CARA)
    assert pares[1] == (1, identidades.CARA)
    assert pares[2] == (3, identidades.NOMBRE_Y_CARA)


def test_emparejar_desempata_por_posicion_y_marca_lo_ambiguo():
    ie2 = _jugadores(fichas.PERFILES["nds.ie2"], [("Axel Blaze", 10, 0x8, 3)])
    ie3 = _jugadores(fichas.PERFILES["nds.ie3"], [("Axel Blaze", 10, 0x6, 3), ("Axel Blaze", 10, 0x8, 3)])
    assert identidades.emparejar(ie2, ie3)[0] == (1, identidades.NOMBRE_Y_CARA)
    ie3_iguales = _jugadores(fichas.PERFILES["nds.ie3"], [("Axel Blaze", 10, 0x8, 3), ("Axel Blaze", 10, 0x8, 3)])
    assert identidades.emparejar(ie2, ie3_iguales)[0] == (0, identidades.AMBIGUO)


def test_mismo_indice_acepta_nombre_o_cara():
    ie1 = _jugadores(fichas.PERFILES["nds.ie1"], [("Mark Evans", 1), ("T. Fielding", 7), ("", 0)])
    ie2 = _jugadores(fichas.PERFILES["nds.ie2"], [("Mark Evans", 1), ("Tammy Fielding", 7), ("Shawn Froste", 921)])
    assert identidades.mismo_indice(ie1, ie2) == {0: 0, 1: 1}


def test_tabla_encadena_los_tres_juegos():
    ie1 = _jugadores(fichas.PERFILES["nds.ie1"], [("Mark Evans", 1, 0x2, 4)])
    ie2 = _jugadores(fichas.PERFILES["nds.ie2"], [("Mark Evans", 1, 0x2, 4), ("Hombre de negro", 3, 0x8, 1, 1)])
    ie3 = _jugadores(fichas.PERFILES["nds.ie3"], [("Otro", 2), ("Mark Evans", 1, 0x2, 4)])
    tabla = identidades.tabla(ie1, ie2, ie3)
    assert [(e.ie1, e.ie2, e.ie3) for e in tabla] == [(0, 0, 1), (None, 1, None)]
    assert tabla[0].jugable_ie3 and not tabla[1].jugable_ie2
    assert identidades.a_filas(tabla)[1][:3] == ["", "1", ""]
