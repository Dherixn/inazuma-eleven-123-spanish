"""Edición definitiva: Aiden Froste sobre tablas sintéticas con la geometría de 3DS (sin ROM)."""

import struct

import pytest

from ie123kit.definitiva import aiden, fichas

P2, P3 = fichas.PERFILES["3ds.ie2"], fichas.PERFILES["3ds.ie3"]


def _stat_shawn(paso: int) -> bytes:
    s = bytearray(paso)
    for k, (minimo, maximo) in enumerate([(20, 76), (15, 69), (11, 68), (14, 64), (18, 77), (13, 50), (16, 60)]):
        s[16 + 4 * k:16 + 4 * k + 2] = bytes([minimo, maximo])
    struct.pack_into("<HHHHHHHH", s, 44, 0x105, 1, 0x66, 12, 0x145, 38, 0x9F, 60)
    struct.pack_into("<H", s, 2, 0x78)
    return bytes(s)


def _tablas_ie2() -> aiden.Tablas:
    n = 2100
    base, stat, no = bytearray(n * P2.base_paso), bytearray(n * P2.stat_paso), bytearray(n * 2)
    for i in range(n):
        struct.pack_into("<H", no, i * 2, i + 247)  # 1060 → 1307, como en el juego
    ident = P2.off_identidad
    a = aiden.FICHA_IE2 * P2.base_paso + ident
    base[a:a + 32] = bytes.fromhex("01011b051b0500031b051b05030000039001010000000000000002000000730e")
    shawn = aiden.SHAWN_DL - 247
    s = shawn * P2.base_paso + ident
    base[s:s + 32] = bytes.fromhex("0224210921090003520821090300000393010e0e008000000000010000003e12")
    stat[shawn * P2.stat_paso:(shawn + 1) * P2.stat_paso] = _stat_shawn(P2.stat_paso)
    return aiden.Tablas(bytes(base), bytes(stat), bytes(no), bytes(64))


def test_ie2_atsuya_pasa_a_ser_jugable_con_su_cara():
    antes = _tablas_ie2()
    t = aiden.ie2(antes, lambda s: s.encode("ascii"))
    b = aiden.FICHA_IE2 * P2.base_paso + P2.off_identidad
    bloque = t.unitbase[b:b + 32]
    assert bloque[0] == 0x02 and struct.unpack_from("<H", bloque, 8)[0] == 0x0852  # curva y modelo de Shawn
    assert [struct.unpack_from("<H", bloque, o)[0] for o in (2, 4, 10)] == [aiden.CARA] * 3  # su cara
    assert bloque[21] == 0x80 and bloque[26] == 0x01  # DL, Viento
    assert struct.unpack_from("<H", bloque, 30)[0] == 2  # su descripción, añadida al final de unitbase.STR
    assert t.textos[:64] == antes.textos and t.textos[64:].rstrip(b"\0") == aiden.DESCRIPCION_IE2.encode()
    assert len(t.textos) % 32 == 0
    stat = t.unitstat[aiden.FICHA_IE2 * P2.stat_paso:(aiden.FICHA_IE2 + 1) * P2.stat_paso]
    assert struct.unpack_from("<8H", stat, 44) == (0x105, 1, 0x66, 12, 0x9F, 34, 0x114, 55)
    assert (stat[17], stat[25], stat[29], stat[33]) == (80, 66, 58, 79)  # tiro, control, defensa, rapidez
    # Solo cambian la ficha de Aiden y su crecimiento.
    assert t.unitno == antes.unitno
    otra = 5 * P2.base_paso
    assert t.unitbase[:otra] == antes.unitbase[:otra]


def test_ie2_se_niega_si_la_ficha_no_es_la_de_atsuya():
    t = _tablas_ie2()
    no = bytearray(t.unitno)
    struct.pack_into("<H", no, aiden.FICHA_IE2 * 2, 1)
    with pytest.raises((KeyError, ValueError)):
        aiden.ie2(aiden.Tablas(t.unitbase, t.unitstat, bytes(no), t.textos), lambda s: s.encode("ascii"))


def _tablas_ie3() -> aiden.Tablas:
    n, ultima = 40, 30
    base, stat, no = bytearray(n * P3.base_paso), bytearray(n * P3.stat_paso), bytearray(n * 2)
    for i in range(1, ultima + 1):
        struct.pack_into("<H", no, i * 2, 2320 + i)  # 17 → 2337
    for i in range(ultima + 1, n):
        base[i * P3.base_paso:i * P3.base_paso + 4] = "未定".encode("shift_jis")
        struct.pack_into("<H", base, i * P3.base_paso + P3.base_paso - 2, 0x2B60 + i)
    shawn = aiden.SHAWN_DL - 2320
    b = shawn * P3.base_paso
    nombre = "ＳｈａｗｎＦｒｏｓｔｅ".encode("shift_jis")
    base[b:b + len(nombre)] = nombre
    base[b + 76:b + 104] = bytes.fromhex("020121092109000352082109df01010e00800000000001000000d414")
    stat[shawn * P3.stat_paso:(shawn + 1) * P3.stat_paso] = fichas.ofuscar(_stat_shawn(P3.stat_paso))
    return aiden.Tablas(bytes(base), bytes(stat), bytes(no), bytes(96))


def test_ie3_aiden_va_en_la_ficha_libre_tras_las_numeradas():
    antes = _tablas_ie3()
    t, ficha, numero = aiden.ie3(antes, lambda s: s.encode("utf-8"))
    assert (ficha, numero) == (31, 2351)
    assert t.numeros()[ficha] == numero and t.numeros()[:ficha] == antes.numeros()[:ficha]
    r = t.unitbase[ficha * P3.base_paso:(ficha + 1) * P3.base_paso]
    assert r[:13] == b"Aiden Froste\0" and r[28:34] == b"Aiden\0"
    assert [struct.unpack_from("<H", r, 76 + o)[0] for o in (2, 4, 10)] == [aiden.CARA] * 3
    assert r[76 + 17] == 0x80 and struct.unpack_from("<H", r, 102)[0] == 3
    assert t.textos[96:].rstrip(b"\0").decode() == aiden.DESCRIPCION_IE3
    plano = fichas.desofuscar(t.unitstat[ficha * P3.stat_paso:(ficha + 1) * P3.stat_paso])
    assert struct.unpack_from("<8H", plano, 44) == (0x105, 1, 0x66, 12, 0x9F, 34, 0x114, 55)


def test_ie3_sin_ficha_libre_es_un_error():
    t = _tablas_ie3()
    base = bytearray(t.unitbase)
    base[31 * P3.base_paso:31 * P3.base_paso + 4] = b"Otro"  # la que sigue a la última numerada no es «未定»
    with pytest.raises(ValueError, match="no está libre"):
        aiden.ie3(aiden.Tablas(bytes(base), t.unitstat, t.unitno, t.textos), lambda s: s.encode("ascii"))


def test_aplicar_aiden_dos_veces_es_un_error():
    # Pasar como entrada la carpeta que ya lleva la edición definitiva duplicaría a Aiden en el IE3.
    t2 = aiden.ie2(_tablas_ie2(), lambda s: s.encode("ascii"))
    with pytest.raises(ValueError, match="ya llevan a Aiden"):
        aiden.ie2(t2, lambda s: s.encode("ascii"))
    t3, _, _ = aiden.ie3(_tablas_ie3(), lambda s: s.encode("utf-8"))
    with pytest.raises(ValueError, match="ya llevan a Aiden"):
        aiden.ie3(t3, lambda s: s.encode("utf-8"))
