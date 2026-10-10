"""Fichas de jugador de los tres juegos: ``unitbase.dat`` (identidad) y ``unitstat.dat`` (crecimiento).

Medido en las ROM NDS en castellano (detalle en ``docs/definitiva/FICHAS.md``):

- IE1 e IE2 comparten numeración: 2400 fichas de 96 B en ``unitbase`` y de 80 B en ``unitstat``.
- IE3 renumera: 2582 fichas de 104 B y 72 B. Su ``unitstat`` va ofuscado con la misma
  transformación que ``item.dat`` (``ie3/comun/items.py``, consumidor 0x17DA4C del CRO).
- El bloque de identidad de 32 B (cara, gráficos, posición, elemento) tiene la misma forma en
  los tres juegos y la cara se conserva de un juego a otro: es la llave del traspaso.

Solo funciones puras: ni E/S ni efectos al importar.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass

from ie123kit.nucleo.texto.nds_latin import dec_es

# Orden habitual de los atributos en la saga (Tiro, Físico, Control, Defensa, Rapidez, Aguante,
# Valor). El orden de los campos está medido; la etiqueta de cada uno aún no se ha visto en pantalla.
ATRIBUTOS = ("tiro", "fisico", "control", "defensa", "rapidez", "aguante", "valor")
# Nibble alto del byte de posición.
POSICIONES = {0x2: "PO", 0x4: "DF", 0x6: "CC", 0x8: "DL"}
# Deducido de Mark (montaña), Axel (fuego), Nathan (viento) y Kevin (bosque).
ELEMENTOS = {1: "viento", 2: "bosque", 3: "fuego", 4: "montaña"}
# Nivel con que unitstat marca una técnica que no se aprende subiendo de nivel (historia o manual).
NIVEL_NO_POR_NIVEL = 100


@dataclass(frozen=True)
class Perfil:
    """Geometría de las tablas de jugador de un juego en una plataforma."""

    nombre: str
    base_paso: int
    # Offsets absolutos dentro de la ficha de unitbase. El IE3 empieza el bloque 4 B más tarde y
    # le quita 4 B de ceros a mitad, así que la posición y el elemento se mueven 8 B, no 12.
    off_identidad: int
    off_cara: int
    off_posicion: int
    off_elemento: int
    stat_paso: int
    stat_ofuscado: bool
    nivel_u8: bool
    # El IE1 de la recopilación guarda cada técnica en 2 B (número u8 y nivel u8); el resto, en 4.
    tecnica_u8: bool = False
    # Nombre completo de la ficha (recopilación: +32 en IE1/IE2, +0 en IE3). Es el mismo en las
    # variantes de un personaje con otra cara (Jude del Royal y del Raimon en el IE2).
    off_nombre_completo: int = 32
    ancho_nombre_completo: int = 32


_IE12 = {"base_paso": 96, "off_identidad": 64, "off_cara": 66, "off_posicion": 85, "off_elemento": 90,
         "stat_paso": 80, "stat_ofuscado": False, "nivel_u8": False}
_IE3 = {"base_paso": 104, "off_identidad": 76, "off_cara": 78, "off_posicion": 93, "off_elemento": 98,
        "stat_paso": 72, "stat_ofuscado": True, "nivel_u8": True}
PERFILES = {
    "nds.ie1": Perfil("nds.ie1", **_IE12),
    "nds.ie2": Perfil("nds.ie2", **_IE12),
    "nds.ie3": Perfil("nds.ie3", **_IE3),
    # Recopilación de 3DS (medido 2026-10-10): el IE1 viene del port europeo y tiene unitstat de 64 B;
    # IE2 e IE3 tienen las mismas tablas que en DS, pero con la numeración japonesa.
    "3ds.ie1": Perfil("3ds.ie1", **{**_IE12, "stat_paso": 64, "tecnica_u8": True}),
    "3ds.ie2": Perfil("3ds.ie2", **_IE12),
    "3ds.ie3": Perfil("3ds.ie3", **{**_IE3, "off_nombre_completo": 0, "ancho_nombre_completo": 28}),
}


@dataclass(frozen=True)
class Identidad:
    """Ficha de ``unitbase``: nombre y bloque de identidad."""

    indice: int
    nombre: str
    nombre_corto: str
    cara: int
    graficos: tuple[int, int, int, int]
    posicion: str
    elemento: str
    bloque: bytes


@dataclass(frozen=True)
class Crecimiento:
    """Ficha de ``unitstat``: PE, PT, atributos (base, máximo, curva) y técnicas por nivel."""

    indice: int
    pe: tuple[int, int, int]
    pt: tuple[int, int, int]
    atributos: tuple[tuple[int, int, int], ...]
    tecnicas: tuple[tuple[int, int], ...]

    @property
    def jugable(self) -> bool:
        """Las fichas de NPC y de relleno llevan PE máximo 1 y todos los atributos a 1."""
        return self.pe[1] > 1


def _ror2(valor: int) -> int:
    return ((valor >> 2) | ((valor & 3) << 6)) & 0xFF


def _rol2(valor: int) -> int:
    return ((valor << 2) | (valor >> 6)) & 0xFF


def _intercambiar_extremos(datos: bytearray, ancho: int) -> None:
    for inicio in range(0, len(datos) - ancho + 1, ancho):
        fin = inicio + ancho - 1
        datos[inicio], datos[fin] = datos[fin], datos[inicio]


def desofuscar(registro: bytes) -> bytes:
    """Transformación del IE3 (la de ``item.dat``) para un registro de cualquier tamaño."""
    salida = bytearray(_ror2(b ^ 0xAD) for b in registro)
    for ancho in (3, 5, 7, 2):
        _intercambiar_extremos(salida, ancho)
    return bytes(salida)


def ofuscar(registro: bytes) -> bytes:
    """Inversa literal de :func:`desofuscar`."""
    salida = bytearray(registro)
    for ancho in (2, 7, 5, 3):
        _intercambiar_extremos(salida, ancho)
    return bytes(_rol2(b) ^ 0xAD for b in salida)


def decodificar_sjis(campo: bytes) -> str:
    """Nombre de una ficha de la recopilación (Shift-JIS, con los glifos de la traducción)."""
    return campo.split(b"\0")[0].decode("shift_jis", "replace").strip()


def decodificar_latin(campo: bytes) -> str:
    """Nombre de una ficha NDS en castellano; los rellenos en Shift-JIS salen como cadena vacía.

    Las fichas que un juego reserva para el siguiente conservan el nombre japonés. Se reconocen
    por los bytes de cabeza Shift-JIS, que la codificación Latin del NDS no usa (sus acentos van
    de 0xA2 a 0xDF).
    """
    crudo = campo.split(b"\0")[0]
    if any(0x81 <= b <= 0x9F or 0xE0 <= b <= 0xFC for b in crudo):
        return ""
    return dec_es(crudo).strip()


def leer_curvas_exp(lvup_exp: bytes) -> list[list[int]]:
    """``lvup_exp.dat``: 16 curvas × 98 u32 con la EXP acumulada para los niveles 2…99."""
    if len(lvup_exp) != 16 * 98 * 4:
        raise ValueError(f"lvup_exp.dat de {len(lvup_exp)} B; se esperaban {16 * 98 * 4}")
    valores = struct.unpack(f"<{16 * 98}I", lvup_exp)
    return [list(valores[c * 98:(c + 1) * 98]) for c in range(16)]


def leer_identidades(unitbase: bytes, perfil: Perfil, decodificar=decodificar_latin) -> list[Identidad]:
    if len(unitbase) % perfil.base_paso:
        raise ValueError(f"unitbase de {len(unitbase)} B no es múltiplo de {perfil.base_paso} ({perfil.nombre})")
    salida = []
    for indice in range(len(unitbase) // perfil.base_paso):
        reg = unitbase[indice * perfil.base_paso:(indice + 1) * perfil.base_paso]
        salida.append(Identidad(
            indice=indice,
            nombre=decodificar(reg[0:32]),
            nombre_corto=decodificar(reg[32:48]),
            cara=struct.unpack_from("<H", reg, perfil.off_cara)[0],
            graficos=struct.unpack_from("<4H", reg, perfil.off_cara),
            posicion=POSICIONES.get(reg[perfil.off_posicion] >> 4, ""),
            elemento=ELEMENTOS.get(reg[perfil.off_elemento], ""),
            bloque=bytes(reg[perfil.off_identidad:]),
        ))
    return salida


def leer_crecimiento(unitstat: bytes, perfil: Perfil) -> list[Crecimiento]:
    if len(unitstat) % perfil.stat_paso:
        raise ValueError(f"unitstat de {len(unitstat)} B no es múltiplo de {perfil.stat_paso} ({perfil.nombre})")
    salida = []
    for indice in range(len(unitstat) // perfil.stat_paso):
        reg = unitstat[indice * perfil.stat_paso:(indice + 1) * perfil.stat_paso]
        if perfil.stat_ofuscado:
            reg = desofuscar(reg)
        tecnicas = []
        for k in range(4):
            if perfil.tecnica_u8:
                tecnicas.append((reg[44 + 2 * k], reg[45 + 2 * k]))
                continue
            tec = struct.unpack_from("<H", reg, 44 + 4 * k)[0]
            nivel = reg[46 + 4 * k] if perfil.nivel_u8 else struct.unpack_from("<H", reg, 46 + 4 * k)[0]
            tecnicas.append((tec, nivel))
        salida.append(Crecimiento(
            indice=indice,
            pe=(struct.unpack_from("<H", reg, 0)[0], struct.unpack_from("<H", reg, 2)[0], reg[4]),
            pt=(struct.unpack_from("<H", reg, 8)[0], struct.unpack_from("<H", reg, 10)[0], reg[12]),
            atributos=tuple((reg[16 + 4 * k], reg[17 + 4 * k], reg[18 + 4 * k]) for k in range(7)),
            tecnicas=tuple(tecnicas),
        ))
    return salida
