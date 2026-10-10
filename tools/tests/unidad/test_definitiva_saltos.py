"""Edición definitiva: comprobación de saltos de los eventos (SSD sintéticos, sin ROM)."""

import struct

from ie123kit.definitiva import saltos


def _ssd(instrucciones: list[tuple[int, int, list[tuple[int, int]]]]) -> bytes:
    """SSD con esas instrucciones (identificador, código, [(tipo, valor)]); sin textos."""
    codigo = bytearray()
    for ident, op, args in instrucciones:
        tipos = bytearray(4 * ((len(args) + 7) // 8))
        for a, (tipo, _) in enumerate(args):
            tipos[a // 2] |= tipo << (4 * (a % 2))
        largo = 8 + len(tipos) + 4 * len(args)
        codigo += struct.pack("<HHHBB", ident, largo, op, len(args), 1) + tipos
        codigo += b"".join(struct.pack("<I", v) for _, v in args)
    cabecera = struct.pack("<4sIIHHIIII", b"SSD\0", 0, 32 + len(codigo), len(instrucciones), 0, len(codigo), 0, 0, 0)
    return cabecera + bytes(codigo)


JP = [(1, 0x3000, []), (2, 0x6001, [(1, 0), (4, 4)]), (3, 0x301D, [(4, 1)]), (4, 0x4002, [])]


def test_insertar_y_desplazar_el_salto_esta_bien():
    # Una caja partida en dos: se inserta un 0x301D (id 4) y lo de detrás se renumera; el salto pasa de 4 a 5.
    es = [(1, 0x3000, []), (2, 0x6001, [(1, 0), (4, 5)]), (3, 0x301D, [(4, 1)]), (4, 0x301D, [(4, 2)]),
          (5, 0x4002, [])]
    assert saltos.saltos_mal(_ssd(JP), _ssd(es)) == (1, 0)


def test_renumerar_sin_desplazar_el_salto_se_detecta():
    # El fallo de #107: el destino se queda en 4, que ahora es la caja insertada.
    es = [(1, 0x3000, []), (2, 0x6001, [(1, 0), (4, 4)]), (3, 0x301D, [(4, 1)]), (4, 0x301D, [(4, 2)]),
          (5, 0x4002, [])]
    assert saltos.saltos_mal(_ssd(JP), _ssd(es)) == (1, 1)


def test_el_texto_no_cuenta_como_salto_y_lo_que_no_alinea_se_dice():
    es = [(1, 0x3000, []), (2, 0x6001, [(1, 0), (4, 4)]), (3, 0x301D, [(4, 9)]), (4, 0x4002, [])]
    assert saltos.saltos_mal(_ssd(JP), _ssd(es)) == (1, 0)
    assert saltos.saltos_mal(_ssd(JP), _ssd([(1, 0x3000, [])])) is None
