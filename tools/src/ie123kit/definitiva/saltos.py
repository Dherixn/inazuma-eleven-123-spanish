"""Comprueba los saltos de los guiones de evento traducidos contra los japoneses.

Un salto (argumento de tipo 4 en una instrucción SSD: el identificador de la instrucción de destino) que no se
desplaza al insertar instrucciones deja el evento en bucle o saltándose texto (#107 de la traducción: en el sorteo
del FFI del IE3 se quedaba en bucle). La traducción solo inserta instrucciones (cajas de texto partidas), así que las
del japonés se alinean por orden y código de operación, y cada salto traducido tiene que ir a la instrucción que
corresponde a su destino japonés. En 0x301D (texto) el argumento de tipo 4 es un índice de texto, no un salto.

    python -m ie123kit.definitiva saltos --jp base/romfs/archive.fa --es mods/.../romfs/archive.fa
"""
from __future__ import annotations

import struct
from collections import Counter
from collections.abc import Callable
from dataclasses import dataclass, field

from ie123kit.nucleo.compresion.lz10 import decompress
from ie123kit.nucleo.eventos.packnum import parse_index

TIPO_DESTINO = 4
OP_TEXTO = 0x301D
# Guiones de evento de cada juego (Ventisca tiene los suyos; Fuego Explosivo usa los de Rayo Celeste).
GUIONES = ("inazuma1/data_iz/script", "inazuma2/data_iz/script", "inazuma2/data_iz_blizzard/script",
           "inazuma3/data_iz/script", "inazuma3_ogre/data_iz/script")


def instrucciones(data: bytes) -> list[tuple[int, int, list[int], list[int]]]:
    """(identificador, código, tipos, argumentos) de cada instrucción del SSD, en orden."""
    count = struct.unpack_from("<H", data, 12)[0]
    pos, lista = 32, []
    for _ in range(count):
        ident, largo, op, argc, _ = struct.unpack_from("<HHHBB", data, pos)
        tipos_largo = 4 * ((argc + 7) // 8)
        tipos = [(data[pos + 8 + a // 2] >> (4 * (a % 2))) & 15 for a in range(argc)]
        args = [struct.unpack_from("<I", data, pos + 8 + tipos_largo + 4 * a)[0] for a in range(argc)]
        lista.append((ident, op, tipos, args))
        pos += largo
    return lista


def alinear(jp, es) -> list[int] | None:
    """Posición en ``es`` de cada instrucción de ``jp`` (la traducción inserta, no quita); None si no casa."""
    mapa, j = [], 0
    for _, op, _, _ in jp:
        while j < len(es) and es[j][1] != op:
            j += 1
        if j == len(es):
            return None
        mapa.append(j)
        j += 1
    return mapa


def saltos_mal(jp_ssd: bytes, es_ssd: bytes) -> tuple[int, int] | None:
    """(saltos revisados, saltos que no van a donde deben) de un evento; None si no se puede alinear."""
    jp, es = instrucciones(jp_ssd), instrucciones(es_ssd)
    mapa = alinear(jp, es)
    if mapa is None:
        return None
    posicion = {ident: k for k, (ident, _, _, _) in enumerate(jp)}
    revisados = mal = 0
    for k, (_, op, tipos, args) in enumerate(jp):
        if op == OP_TEXTO:
            continue
        for a, (tipo, destino) in enumerate(zip(tipos, args, strict=True)):
            if tipo != TIPO_DESTINO or destino not in posicion:
                continue
            revisados += 1
            if es[mapa[k]][3][a] != es[mapa[posicion[destino]]][0]:
                mal += 1
    return revisados, mal


@dataclass
class Resultado:
    guion: str
    traducidos: int = 0
    revisados: int = 0
    mal: Counter = field(default_factory=Counter)
    sin_alinear: list[int] = field(default_factory=list)


def comprobar(leer_jp: Callable[[str], bytes | None], leer_es: Callable[[str], bytes | None],
              guion: str) -> Resultado | None:
    """Compara los eventos de una carpeta de guiones (``eve.pkh``/``eve.pkb``); None si falta en alguno."""
    archivos = []
    for leer in (leer_jp, leer_es):
        pkh, pkb = leer(f"{guion}/eve.pkh"), leer(f"{guion}/eve.pkb")
        if pkh is None or pkb is None:
            return None
        archivos.append(({e: (o, s) for e, o, s in parse_index(pkh) if e != 0xFFFFFFFF}, pkb))
    (ev_jp, pkb_jp), (ev_es, pkb_es) = archivos
    r = Resultado(guion)
    for eid, (o, s) in sorted(ev_es.items()):
        if eid not in ev_jp:
            continue
        oj, sj = ev_jp[eid]
        jp, es = decompress(pkb_jp[oj:oj + sj]), decompress(pkb_es[o:o + s])
        if jp == es:
            continue
        r.traducidos += 1
        cuenta = saltos_mal(jp, es)
        if cuenta is None:
            r.sin_alinear.append(eid)
            continue
        r.revisados += cuenta[0]
        if cuenta[1]:
            r.mal[eid] += cuenta[1]
    return r
