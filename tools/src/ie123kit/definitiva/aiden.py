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
from dataclasses import dataclass, replace

from ie123kit.definitiva import fichas

NUMERO_IE2 = 1307
SHAWN = (2329, 2337, 2338)  # defensa, delantero (modo Atsuya) y delantero completo; igual en IE2 e IE3
SHAWN_DL = 2337
# Números de Aiden en cada juego de la recopilación (en IE3 y Ogro ocupa la ficha 2337, ver ie3) y el Shawn que
# ficha la historia del IE3 (altas.py).
NUMERO_IE3 = SHAWN_DL
SHAWN_HISTORIA_IE3 = 2338
NUMEROS = {"inazuma2": NUMERO_IE2, "inazuma3": NUMERO_IE3, "inazuma3_ogre": NUMERO_IE3}
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
# Sin comas: la de ancho completo no sale en la caja del IE3.
DESCRIPCION_IE3 = "El hermano pequeño de Shawn y su otra mitad.\nSolo vive para marcar goles."
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
    # Por ficha: colegio (1 B) y datos de búsqueda. La capa no los cambia (Aiden usa una ficha nativa), pero
    # se leen para que, si algún paso los tocara, también se escribieran.
    uschool: bytes = b""
    usearch: bytes = b""

    def numeros(self) -> list[int]:
        return [struct.unpack_from("<H", self.unitno, i * 2)[0] for i in range(len(self.unitno) // 2)]


# Aplicar la capa dos veces (sobre una carpeta que ya la lleva) duplicaría a Aiden en el IE3.
YA_APLICADA = ("estas tablas ya llevan a Aiden: usa la carpeta de la traducción, no la de la edición "
               "definitiva")


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
    if fichas.leer_crecimiento(tablas.unitstat[aiden * p.stat_paso:(aiden + 1) * p.stat_paso], p)[0].jugable:
        raise ValueError(YA_APLICADA)
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
    # replace: lo que no se toca (números, colegio, búsqueda) sigue igual y no se reescribe.
    return replace(tablas, unitbase=bytes(base), unitstat=bytes(stat), textos=textos)


def ajustar_mapa(mapa: dict[int, int], juego_origen: str, juego_destino: str) -> dict[int, int]:
    """Aiden en la tabla de números del traspaso (gemelo de ``AjustarAiden``, DatosDestino.cpp). En el IE3 no
    tiene cara propia, así que el emparejamiento por cara no lo encuentra: su número va al del destino. Y nadie
    más puede caer en la ficha de Aiden (en IE3 y Ogro es la 2337 de Shawn): un Shawn que llegara ahí va al
    que ficha la historia."""
    aiden_o, aiden_d = NUMEROS.get(juego_origen, 0), NUMEROS.get(juego_destino, 0)
    salida = dict(mapa)
    for origen, destino in mapa.items():
        if destino == aiden_d and origen != aiden_o and juego_destino != "inazuma2":
            salida[origen] = SHAWN_HISTORIA_IE3
    if aiden_o and aiden_d:
        salida[aiden_o] = aiden_d
    return salida


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
    """Aiden en el IE3 (u Ogro): toma la ficha de Shawn en modo Atsuya (``SHAWN_DL``, 2337), que la historia
    del IE3 no ficha (ficha a Shawn como 2338; ver altas.py). Devuelve las tablas, su ficha y su número.

    Una ficha nueva o reciclada (la libre de detrás de las numeradas, una de relleno) sale en la reserva pero
    el IE3 no la pinta en el grupo aunque esté en la lista: el grupo solo admite fichas nativas de jugador
    (comprobado en Azahar: ni copiando la de Shawn entera). De la 2337 se quedan el número, el modelo, el icono,
    el colegio, la búsqueda y las caras; cambian el nombre, la descripción y el crecimiento (técnicas de
    Atsuya). El retrato de Atsuya (``CARA``) no existe en el IE3: con él, el grupo tampoco pinta al jugador
    (ni sale su icono en la reserva). La llevan Nuevo Alpino y Los Impostores (y Chicharros en Familia en el
    Ogro): ahí juega Aiden. ``codificar`` es el de ancho completo, como los nombres del IE3.
    """
    p = fichas.PERFILES["3ds.ie3"]
    nombre = codificar(NOMBRE) + bytes(1)
    if any(tablas.unitbase[i:i + len(nombre)] == nombre for i in range(0, len(tablas.unitbase), p.base_paso)):
        raise ValueError(YA_APLICADA)
    ficha = _ficha_de(tablas, SHAWN_DL)
    base = bytearray(tablas.unitbase)
    b1 = ficha * p.base_paso
    textos, puntero = _anadir_texto(tablas.textos, codificar(DESCRIPCION_IE3))
    struct.pack_into("<H", base, b1 + p.base_paso - 2, puntero)
    for offset, ancho, cod in ((0, 28, codificar(NOMBRE)), (28, 16, NOMBRE_CORTO.encode("ascii")),
                               (44, 32, LECTURA.encode("shift_jis"))):
        if len(cod) >= ancho:
            raise ValueError(f"«{cod!r}» no cabe en {ancho} B con su terminador")
        base[b1 + offset:b1 + offset + ancho] = cod + bytes(ancho - len(cod))
    stat = bytearray(tablas.unitstat)
    plano = fichas.desofuscar(tablas.unitstat[ficha * p.stat_paso:(ficha + 1) * p.stat_paso])
    stat[ficha * p.stat_paso:(ficha + 1) * p.stat_paso] = fichas.ofuscar(_crecimiento(plano))
    return replace(tablas, unitbase=bytes(base), unitstat=bytes(stat), textos=textos), ficha, SHAWN_DL
