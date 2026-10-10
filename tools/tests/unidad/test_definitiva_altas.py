"""Edición definitiva: altas al equipo de la historia (scripts de evento) y su uso en el traspaso, sin ROM."""

import struct

from ie123kit.definitiva import altas, identidades
from ie123kit.definitiva.datos_juego import DatosJuego
from ie123kit.definitiva.fichas import Crecimiento, Identidad
from ie123kit.nucleo.compresion import lz10


def _ssd(instrucciones):
    """SSD mínimo: cabecera de 32 B (cuenta en +12) y un registro por instrucción (opcode, argumentos)."""
    registros = b""
    for ident, (op, args) in enumerate(instrucciones):
        tipos = 4 * ((len(args) + 7) // 8)
        cuerpo = bytes(tipos) + b"".join(struct.pack("<I", a) for a in args)
        registros += struct.pack("<HHHBB", ident, 8 + len(cuerpo), op, len(args), 0) + cuerpo
    cabecera = bytearray(32)
    cabecera[:4] = b"SSD\0"
    struct.pack_into("<H", cabecera, 12, len(instrucciones))
    return bytes(cabecera) + registros


def _paquete(eventos):
    """eve.pkh + eve.pkb con cada evento comprimido en LZ10 y el centinela del final."""
    pkb, tabla = b"", b""
    for eid, ssd in eventos:
        comprimido = lz10.compress(ssd)
        tabla += struct.pack("<III", eid, len(pkb), len(comprimido))
        pkb += comprimido
    tabla += struct.pack("<III", 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)
    pkh = b"PackNum 20120101" + struct.pack("<I", 0x30 + len(tabla)) + bytes(0x30 - 20) + tabla
    return pkh, pkb


def test_altas_de_script_saca_el_primer_argumento_de_la_instruccion_de_alta():
    ssd = _ssd([(12399, [2338, 1, 1, 1]), (altas.OP_ALTA, [3170, 30, 10, 0, 1]), (16447, [3170]),
                (altas.OP_ALTA, [1, 25, 1, 0, 1])])
    assert altas.altas_de_script(ssd) == [3170, 1]


def test_altas_recorre_todos_los_eventos_del_juego():
    pkh, pkb = _paquete([(31010000, _ssd([(altas.OP_ALTA, [3170, 30, 10, 0, 1])])),
                         (32010100, _ssd([(16447, [3170]), (altas.OP_ALTA, [1, 1, 1, 0, 1])])),
                         (32030000, _ssd([(altas.OP_ALTA, [30, 20, 14, 0, 1])]))])
    archivos = {"inazuma3/data_iz/script/eve.pkh": pkh, "inazuma3/data_iz/script/eve.pkb": pkb}
    assert altas.altas(archivos.get, "inazuma3") == {3170, 1, 30}
    assert altas.altas(archivos.get, "inazuma2") == set()  # sin paquete, sin altas


def _jugador(indice, cara, posicion, jugable=True):
    pe = (50, 100 if jugable else 1, 1)
    return identidades.Jugador(Identidad(indice, "", "", cara, (cara, 0, 0, 0), posicion, "", b""),
                               Crecimiento(indice, pe, pe, (), ()))


def test_preferir_altas_usa_la_ficha_que_la_historia_vuelve_a_fichar():
    # Destino como el IE3: Jude en las fichas 1 (número 30, el de la historia) y 2 (número 1097, su cara
    # del IE2); Byron solo en la 3 (número 190, la historia no lo ficha); «？？？» en la 4 y la 5.
    nombres = [b"Mark", b"Jude", b"Jude", b"Byron", "？？？".encode("shift_jis"), "？？？".encode("shift_jis")]
    numeros = [1, 30, 1097, 190, 2481, 2482]
    destino = [_jugador(i, 0x10 + i, "MD") for i in range(len(numeros))]
    datos = DatosJuego(curvas=[], curva=[0] * 6, aprende=[()] * 6, numero=numeros, caras=[0x10 + i for i in range(6)],
                       indice_de={n: i for i, n in enumerate(numeros)}, nombres=nombres)
    por_ficha = {10: 2, 11: 3, 12: 0, 13: 5}
    salida = identidades.preferir_altas(por_ficha, destino, datos, {1, 30, 2481})
    assert salida == {10: 1, 11: 3, 12: 0, 13: 5}  # Jude a la 30; Byron, Mark y el «？？？» igual
