"""Edición definitiva: objetos de StreetPass y Wi-Fi en español (tablas sintéticas, sin ROM)."""

import struct

import pytest

from ie123kit.definitiva import fichas, objetos


def _codificar(texto: str) -> bytes:
    return texto.encode("utf-8")


def _tablas(nombres: dict[int, str]) -> tuple[bytes, bytes]:
    dat = bytearray(1024 * objetos.PASO)
    for i in range(1024):
        registro = bytearray(objetos.PASO)
        registro[:len(nombres.get(i, "x").encode("shift_jis"))] = nombres.get(i, "x").encode("shift_jis")
        registro[29] = 1
        struct.pack_into("<H", registro, objetos.OFF_DESCRIPCION, 3)  # todas a la misma descripción vieja
        dat[i * objetos.PASO:(i + 1) * objetos.PASO] = fichas.ofuscar(bytes(registro))
    return bytes(dat), bytes(5 * 32)


def _registro(dat: bytes, i: int) -> bytes:
    return fichas.desofuscar(dat[i * objetos.PASO:(i + 1) * objetos.PASO])


def test_nombre_y_descripcion_nueva_al_final():
    dat, texto = _tablas({704: "ジ・アース", 852: "プロキオンネット"})
    nuevo_dat, nuevo_texto, saltados = objetos.traducir(dat, texto, _codificar)
    assert len(saltados) == len(objetos.CAMBIOS) - 2
    tierra, red = _registro(nuevo_dat, 704), _registro(nuevo_dat, 852)
    assert tierra[:10] == b"La Tierra\0" and tierra[29] == 1
    assert red[:29].split(b"\0")[0] == b"Constelaci\x83\xa2n estelar"  # la ó con su portador
    p = struct.unpack_from("<H", tierra, objetos.OFF_DESCRIPCION)[0]
    assert p == 5 and nuevo_texto[p * 32:].split(b"\0")[0] == objetos.CAMBIOS[704][2].encode("utf-8")
    assert nuevo_texto[:5 * 32] == texto and len(nuevo_texto) % 32 == 0
    # Lo demás, igual.
    assert _registro(nuevo_dat, 703) == _registro(dat, 703)


def test_si_la_traduccion_ya_lo_cambio_no_se_toca():
    dat, texto = _tablas({704: "La Tierra"})
    nuevo_dat, nuevo_texto, saltados = objetos.traducir(dat, texto, _codificar)
    assert nuevo_dat == dat and nuevo_texto == texto
    assert any("704" in s for s in saltados)


def test_un_nombre_que_no_cabe_es_un_error():
    with pytest.raises(ValueError):
        objetos.codificar_nombre("Un nombre larguísimo que no cabe")


def test_solo_nombre_y_solo_en_su_juego():
    dat, texto = _tablas({87: "さらなるしょうこ", 78: "体育館の鍵"})
    # En el IE3 el 78 no se mira (es del Ogro); el 87 cambia de nombre y conserva su descripción.
    nuevo_dat, nuevo_texto, saltados = objetos.traducir(dat, texto, _codificar, "inazuma3")
    assert not any(s.startswith("objeto 78:") for s in saltados)
    pruebas = _registro(nuevo_dat, 87)
    assert pruebas[:13] == b"M\x83\x9fs pruebas\0"  # la «á» con su portador
    assert struct.unpack_from("<H", pruebas, objetos.OFF_DESCRIPCION)[0] == 3 and nuevo_texto[:len(texto)] == texto
    assert _registro(nuevo_dat, 78) == _registro(dat, 78)
    ogro_dat, _, _ = objetos.traducir(dat, texto, _codificar, "inazuma3_ogre")
    assert _registro(ogro_dat, 78)[:18] == b"Llave del gimnasio"
