"""Aiden Froste jugable en el IE2, el IE3 y Ogro (#6), con lo que ya trae el juego.

IE2: la ficha 1060 (número 1307) es el Atsuya de los recuerdos. Tiene retrato propio (cara 0x51B, pelo
naranja) pero no se puede jugar. Se le dan la curva de EXP, el modelo 3D, la posición (DL) y el elemento
(Viento) del Shawn delantero (número 2337), y su crecimiento con las técnicas de Aiden. La cara se queda la
suya: las de Shawn (0x919, 0x921, 0x922) van atadas a su cambio de forma, y una ficha con ellas sale en el
juego como Shawn defensa con nivel 0 (probado en Azahar).

IE3 y Ogro: no hay Atsuya. Se usa la ficha libre que va justo detrás de la última con número, se le copia el
Shawn delantero y se le ponen la cara 0x51B, su nombre y el número siguiente al último: la tabla de números va
en orden estricto y sin huecos hasta ahí. El traspaso IE2 → IE3 lo empareja por la cara, que es solo suya.

Lo que es diseño nuestro (técnicas, retoque de estadísticas, textos) está en las constantes de arriba. Los
números de técnica son los mismos en el IE2 y el IE3.
"""
from __future__ import annotations

import struct
from collections.abc import Callable
from dataclasses import dataclass

from ie123kit.definitiva import fichas

NUMERO_IE2 = 1307
SHAWN = (2329, 2337, 2338)  # defensa, delantero (modo Atsuya) y delantero completo; igual en IE2 e IE3
SHAWN_DL = 2337
CARA = 0x051B
FICHA_IE2 = 1060

# Ventisca eterna, Entrada huracán, Chut congelante y Aullido de lobo (la técnica de los dos hermanos).
TECNICAS = ((0x105, 1), (0x066, 12), (0x09F, 34), (0x114, 55))
AULLIDO_DE_LOBO = 0x114
# Más tiro y velocidad, menos defensa y control que Shawn: un delantero que solo piensa en marcar.
# (índice de la estadística en el orden del juego: tiro, físico, control, defensa, rapidez, aguante, valor)
RETOQUE = {0: +4, 2: -2, 3: -6, 4: +2}

NOMBRE = "Aiden Froste"
NOMBRE_CORTO = "Aiden"
LECTURA = "ふぶき　あつや"
# Descripciones en ancho completo, como las del juego. La caja del IE2 da 18 casillas por línea (sus textos
# juntan letras de dos en dos con glifos propios de la traducción, que no se tocan: tipografía v20).
DESCRIPCION_IE2 = "Hermano de Shawn y\nsu otra mitad."
DESCRIPCION_IE3 = "El hermano pequeño de Shawn, su otra mitad.\nSolo vive para marcar goles."
CASILLAS_IE2 = 18

# Bloque de identidad del IE2 (unitbase + 64) y del IE3 (+ 76): desplazamientos dentro del bloque.
_CURVA, _CARAS, _MODELO = 0, (2, 4, 10), 8
_POS_IE2, _ELEM_IE2 = 21, 26
_STAT_INICIO, _STAT_NUM, _STAT_PASO = 16, 7, 4
_TEC_INICIO = 44


@dataclass(frozen=True)
class Tablas:
    unitbase: bytes
    unitstat: bytes
    unitno: bytes
    textos: bytes = b""  # unitbase.STR: descripciones, punteros u16 ×32 al final de la ficha

    def numeros(self) -> list[int]:
        return [struct.unpack_from("<H", self.unitno, i * 2)[0] for i in range(len(self.unitno) // 2)]


def _crecimiento(stat_shawn: bytes) -> bytes:
    """Crecimiento del Shawn delantero con el retoque de Aiden y sus técnicas (registro desofuscado)."""
    s = bytearray(stat_shawn)
    for k, delta in RETOQUE.items():
        o = _STAT_INICIO + k * _STAT_PASO
        minimo, maximo = s[o], s[o + 1]
        s[o + 1] = max(minimo + 1, min(99, maximo + delta))
    for k, (tecnica, nivel) in enumerate(TECNICAS):
        struct.pack_into("<HH", s, _TEC_INICIO + 4 * k, tecnica, nivel)
    return bytes(s)


def _anadir_texto(textos: bytes, cod: bytes) -> tuple[bytes, int]:
    """Añade una descripción al final de unitbase.STR (en ranuras de 32 B) y devuelve su puntero."""
    if len(textos) % 32:
        raise ValueError("unitbase.STR no acaba en ranura de 32 B")
    puntero = len(textos) // 32
    if puntero > 0xFFFF:
        raise ValueError("unitbase.STR demasiado grande para un puntero de 16 bits")
    ranura = cod + bytes(32 - len(cod) % 32)
    return textos + ranura, puntero


def _ficha_de(tablas: Tablas, numero: int) -> int:
    numeros = tablas.numeros()
    if numero not in numeros:
        raise KeyError(f"no hay jugador con número {numero}")
    return numeros.index(numero)


def ie2(tablas: Tablas, codificar: Callable[[str], bytes]) -> Tablas:
    """La ficha 1060 pasa a ser Aiden jugable, con su descripción. ``codificar``: ancho completo."""
    p = fichas.PERFILES["3ds.ie2"]
    aiden, shawn = _ficha_de(tablas, NUMERO_IE2), _ficha_de(tablas, SHAWN_DL)
    if aiden != FICHA_IE2:
        raise ValueError(f"el número {NUMERO_IE2} está en la ficha {aiden}, no en la {FICHA_IE2}")
    base = bytearray(tablas.unitbase)
    b, s = aiden * p.base_paso + p.off_identidad, shawn * p.base_paso + p.off_identidad
    if struct.unpack_from("<H", base, b + _CARAS[0])[0] != CARA:
        raise ValueError("la ficha de Atsuya no tiene la cara esperada")
    base[b + _CURVA] = base[s + _CURVA]
    base[b + _MODELO:b + _MODELO + 2] = base[s + _MODELO:s + _MODELO + 2]
    base[b + _POS_IE2] = base[s + _POS_IE2]
    base[b + _ELEM_IE2] = base[s + _ELEM_IE2]
    # Los bytes +16 y +19 (0x90/0x00 en las fichas de recuerdos, 0x93/0x0E en las de jugador) se dejan:
    # así es como se ha probado en Azahar y sale bien en la plantilla.
    stat = bytearray(tablas.unitstat)
    o_s, o_a = shawn * p.stat_paso, aiden * p.stat_paso
    stat[o_a:o_a + p.stat_paso] = _crecimiento(tablas.unitstat[o_s:o_s + p.stat_paso])
    if any(len(linea) > CASILLAS_IE2 for linea in DESCRIPCION_IE2.split("\n")):
        raise ValueError("la descripción no cabe en la caja del IE2")
    textos, puntero = _anadir_texto(tablas.textos, codificar(DESCRIPCION_IE2))
    struct.pack_into("<H", base, aiden * p.base_paso + p.base_paso - 2, puntero)
    return Tablas(bytes(base), bytes(stat), tablas.unitno, textos)


def ficha_libre_tras_numeradas(tablas: Tablas) -> int:
    """La ficha que va justo detrás de la última con número; debe estar reservada («未定»)."""
    numeros = tablas.numeros()
    ultima = max(i for i, n in enumerate(numeros) if n)
    ficha = ultima + 1
    nombre = tablas.unitbase[ficha * 104:ficha * 104 + 28].split(b"\0")[0]
    if ficha >= len(numeros) or numeros[ficha] or nombre != "未定".encode("shift_jis"):
        raise ValueError(f"la ficha {ficha} no está libre")
    return ficha


def ie3(tablas: Tablas, codificar: Callable[[str], bytes]) -> tuple[Tablas, int, int]:
    """Aiden en una ficha libre del IE3 (u Ogro). Devuelve las tablas, su ficha y su número.

    ``codificar`` es el de ancho completo, como el nombre completo y las descripciones del IE3.
    """
    p = fichas.PERFILES["3ds.ie3"]
    shawn = _ficha_de(tablas, SHAWN_DL)
    ficha = ficha_libre_tras_numeradas(tablas)
    numero = max(tablas.numeros()) + 1
    base = bytearray(tablas.unitbase)
    b0, b1 = shawn * p.base_paso, ficha * p.base_paso
    base[b1:b1 + p.base_paso] = base[b0:b0 + p.base_paso]
    textos, puntero = _anadir_texto(tablas.textos, codificar(DESCRIPCION_IE3))
    struct.pack_into("<H", base, b1 + p.base_paso - 2, puntero)
    for offset, ancho, cod in ((0, 28, codificar(NOMBRE)), (28, 16, NOMBRE_CORTO.encode("ascii")),
                               (44, 32, LECTURA.encode("shift_jis"))):
        if len(cod) >= ancho:
            raise ValueError(f"«{cod!r}» no cabe en {ancho} B con su terminador")
        base[b1 + offset:b1 + offset + ancho] = cod + bytes(ancho - len(cod))
    for o in _CARAS:
        struct.pack_into("<H", base, b1 + p.off_identidad + o, CARA)
    stat = bytearray(tablas.unitstat)
    plano = fichas.desofuscar(tablas.unitstat[shawn * p.stat_paso:(shawn + 1) * p.stat_paso])
    stat[ficha * p.stat_paso:(ficha + 1) * p.stat_paso] = fichas.ofuscar(_crecimiento(plano))
    no = bytearray(tablas.unitno)
    struct.pack_into("<H", no, ficha * 2, numero)
    return Tablas(bytes(base), bytes(stat), bytes(no), textos), ficha, numero
