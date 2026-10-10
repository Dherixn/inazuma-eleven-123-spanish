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


def test_aplicar_aiden_dos_veces_es_un_error():
    # Pasar como entrada la carpeta que ya lleva la edición definitiva duplicaría a Aiden en el IE3.
    t2 = aiden.ie2(_tablas_ie2(), lambda s: s.encode("ascii"))
    with pytest.raises(ValueError, match="ya llevan a Aiden"):
        aiden.ie2(t2, lambda s: s.encode("ascii"))
    t3, _, _ = aiden.ie3(_tablas_ie3(), lambda s: s.encode("utf-8"))
    with pytest.raises(ValueError, match="ya llevan a Aiden"):
        aiden.ie3(t3, lambda s: s.encode("utf-8"))


def test_ie2_no_toca_el_colegio_ni_la_busqueda():
    # Si ie2 devolviera unas tablas sin ellos, construir los reescribiría vacíos.
    antes = _tablas_ie2()
    con = aiden.Tablas(antes.unitbase, antes.unitstat, antes.unitno, antes.textos, b"\x01" * 4, b"\x02" * 8)
    t = aiden.ie2(con, lambda s: s.encode("ascii"))
    assert (t.uschool, t.usearch, t.unitno) == (con.uschool, con.usearch, con.unitno)


def test_ie3_aiden_toma_la_ficha_2337_y_conserva_numero_icono_y_modelo():
    # El IE3 solo pinta en el grupo fichas nativas de jugador: Aiden toma la de Shawn en modo Atsuya.
    escuela, busqueda = b"\x01" * 40, b"\x02" * 40 * 44
    t0 = _tablas_ie3()
    antes = aiden.Tablas(t0.unitbase, t0.unitstat, t0.unitno, t0.textos, escuela, busqueda)
    t, ficha, numero = aiden.ie3(antes, lambda s: s.encode("utf-8"))
    assert (ficha, numero) == (aiden.SHAWN_DL - 2320, aiden.SHAWN_DL) and t.numeros() == antes.numeros()
    r = t.unitbase[ficha * P3.base_paso:(ficha + 1) * P3.base_paso]
    r0 = antes.unitbase[ficha * P3.base_paso:(ficha + 1) * P3.base_paso]
    assert r[:13] == b"Aiden Froste\0" and r[28:34] == b"Aiden\0"
    # Caras, modelo y lo demás, los de la 2337: el retrato de Atsuya no existe en el IE3 y sin él no sale en el
    # grupo.
    assert r[76:76 + 26] == r0[76:76 + 26]
    assert struct.unpack_from("<H", r, 102)[0] == 3
    assert t.textos[96:].rstrip(b"\0").decode() == aiden.DESCRIPCION_IE3
    plano = fichas.desofuscar(t.unitstat[ficha * P3.stat_paso:(ficha + 1) * P3.stat_paso])
    assert struct.unpack_from("<8H", plano, 44) == (0x105, 1, 0x66, 12, 0x9F, 34, 0x114, 55)
    assert (t.uschool, t.usearch) == (escuela, busqueda)  # nativos: no se tocan


def test_ajustar_mapa_lleva_a_aiden_a_su_numero_y_a_nadie_mas():
    # IE2 -> IE3: Aiden (1307) no tiene pareja por cara; un Shawn delantero que cayera en la 2337 va a la 2338.
    mapa = {1: 1, 2337: 2337, 2338: 2338}
    assert aiden.ajustar_mapa(mapa, "inazuma2", "inazuma3") == {1: 1, 2337: 2338, 2338: 2338, 1307: 2337}
    # IE3 -> Ogro: Aiden es la 2337 en los dos.
    assert aiden.ajustar_mapa({2337: 2337, 1: 1}, "inazuma3", "inazuma3_ogre") == {2337: 2337, 1: 1}
    # IE1 -> IE2: en el IE1 no hay Aiden y en el IE2 la 2337 es de Shawn.
    assert aiden.ajustar_mapa({2337: 2337}, "inazuma1", "inazuma2") == {2337: 2337}
