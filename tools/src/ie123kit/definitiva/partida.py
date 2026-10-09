"""Partidas guardadas de la recopilación: lectura, edición y sellado de las sumas de control.

Formato tomado de la documentación pública de la comunidad (Inazuma Eleven Toolbox, de SwareJonge),
reimplementado aquí. Cada juego guarda su partida con la misma cabecera:

| Offset | Contenido |
|---|---|
| 0x00 | CRC32 de 0x04–0x3F |
| 0x04 | versión (16 B, «INAZUMA_ELEVEN…») y código NDS (4 B) |
| 0x40 | CRC32 de 0x44–0xBF |
| 0x44 | CRC32 del bloque de datos, de 0xC0 en adelante (largo propio de cada juego) |
| 0x48 | marca 0x19150A04, tiempo de juego, capítulo, nivel del equipo |
| 0x57 | número de jugadores fichados (máximo 100) |
| 0xC0 | cantidades de objetos, un byte por objeto |

Al sellar se calcula primero el CRC del bloque de datos (0x44) y después el de 0x40, que lo incluye.
El formato de jugador del IE1 de la recopilación es distinto y está sin documentar; su perfil queda
marcado como no verificado y no se escribe.
"""
from __future__ import annotations

import struct
import zlib
from dataclasses import dataclass, replace

MARCA = 0x19150A04
MAX_JUGADORES = 100
OFF_CRC_CABECERA = 0x00
OFF_CRC_INFO = 0x40
OFF_CRC_DATOS = 0x44
INFO_INICIO = 0x44
INFO_LARGO = 0x7C
DATOS_INICIO = 0xC0
OFF_MARCA = 0x48
OFF_NUM_JUGADORES = 0x57
NUM_TECNICAS = 6


@dataclass(frozen=True)
class FormatoPartida:
    nombre: str
    datos_largo: int
    jugadores_inicio: int
    jugador_paso: int
    verificado: bool


FORMATOS = {
    # Sin verificar: valores comentados en el editor de la comunidad para el IE1 japonés.
    "ie1.123": FormatoPartida("ie1.123", datos_largo=0x54C0, jugadores_inicio=0x1268, jugador_paso=0x70, verificado=False),
    "ie2.123": FormatoPartida("ie2.123", datos_largo=0x7DC0, jugadores_inicio=0x1268, jugador_paso=0x70, verificado=True),
    "ie3.123": FormatoPartida("ie3.123", datos_largo=0x5940, jugadores_inicio=0x11B0, jugador_paso=0x6C, verificado=True),
}


@dataclass(frozen=True)
class JugadorPartida:
    """Ficha de un jugador fichado. ``crudo`` conserva los campos que aún no se interpretan."""

    exp: int
    id: int
    tecnicas: tuple[int, ...]
    niveles_tecnica: tuple[int, ...]
    nivel: int
    crudo: bytes

    def con(self, **cambios) -> JugadorPartida:
        return replace(self, **cambios)


def leer_jugador(reg: bytes) -> JugadorPartida:
    return JugadorPartida(
        exp=struct.unpack_from("<I", reg, 0x00)[0],
        id=struct.unpack_from("<H", reg, 0x04)[0],
        tecnicas=struct.unpack_from(f"<{NUM_TECNICAS}H", reg, 0x38),
        niveles_tecnica=tuple(reg[0x44:0x44 + NUM_TECNICAS]),
        nivel=reg[0x4A],
        crudo=bytes(reg),
    )


def escribir_jugador(jugador: JugadorPartida) -> bytes:
    reg = bytearray(jugador.crudo)
    struct.pack_into("<I", reg, 0x00, jugador.exp)
    struct.pack_into("<H", reg, 0x04, jugador.id)
    struct.pack_into(f"<{NUM_TECNICAS}H", reg, 0x38, *jugador.tecnicas)
    reg[0x44:0x44 + NUM_TECNICAS] = bytes(jugador.niveles_tecnica)
    reg[0x4A] = jugador.nivel
    return bytes(reg)


class Partida:
    """Partida de un juego en memoria. Las ediciones no son válidas hasta :meth:`sellar`."""

    def __init__(self, datos: bytes, formato: FormatoPartida):
        minimo = DATOS_INICIO + formato.datos_largo
        if len(datos) < minimo:
            raise ValueError(f"partida de {len(datos)} B; el formato {formato.nombre} necesita {minimo}")
        self.datos = bytearray(datos)
        self.formato = formato

    # -- comprobaciones --------------------------------------------------------------------
    @property
    def version(self) -> str:
        return self.datos[0x04:0x14].split(b"\0")[0].decode("ascii", "replace")

    def sumas_correctas(self) -> dict[str, bool]:
        return {
            "cabecera": self._crc(0x04, 0x3C) == self._leer_u32(OFF_CRC_CABECERA),
            "info": self._crc(INFO_INICIO, INFO_LARGO) == self._leer_u32(OFF_CRC_INFO),
            "datos": self._crc(DATOS_INICIO, self.formato.datos_largo) == self._leer_u32(OFF_CRC_DATOS),
        }

    def validar(self) -> None:
        malas = [k for k, ok in self.sumas_correctas().items() if not ok]
        if malas:
            raise ValueError(f"sumas de control incorrectas: {', '.join(malas)}")
        if self._leer_u32(OFF_MARCA) != MARCA:
            raise ValueError(f"falta la marca 0x{MARCA:08X} en 0x{OFF_MARCA:X}")

    def sellar(self) -> bytes:
        """Recalcula las sumas (datos, después info) y devuelve los bytes listos para guardar."""
        if not self.formato.verificado:
            raise ValueError(f"el formato {self.formato.nombre} no está verificado: no se escribe")
        struct.pack_into("<I", self.datos, OFF_CRC_DATOS, self._crc(DATOS_INICIO, self.formato.datos_largo))
        struct.pack_into("<I", self.datos, OFF_CRC_INFO, self._crc(INFO_INICIO, INFO_LARGO))
        return bytes(self.datos)

    # -- jugadores -------------------------------------------------------------------------
    @property
    def num_jugadores(self) -> int:
        return self.datos[OFF_NUM_JUGADORES]

    def _off_jugador(self, indice: int) -> int:
        return self.formato.jugadores_inicio + indice * self.formato.jugador_paso

    def jugadores(self) -> list[JugadorPartida]:
        paso = self.formato.jugador_paso
        return [leer_jugador(self.datos[o:o + paso]) for o in map(self._off_jugador, range(self.num_jugadores))]

    def poner_jugador(self, indice: int, jugador: JugadorPartida) -> None:
        if not 0 <= indice < self.num_jugadores:
            raise IndexError(f"no hay jugador {indice} (fichados: {self.num_jugadores})")
        self._escribir_reg(indice, jugador)

    def anadir_jugador(self, jugador: JugadorPartida) -> int:
        indice = self.num_jugadores
        if indice >= MAX_JUGADORES:
            raise ValueError(f"la plantilla ya tiene {MAX_JUGADORES} jugadores")
        self._escribir_reg(indice, jugador)
        self.datos[OFF_NUM_JUGADORES] = indice + 1
        return indice

    def _escribir_reg(self, indice: int, jugador: JugadorPartida) -> None:
        reg = escribir_jugador(jugador)
        if len(reg) != self.formato.jugador_paso:
            raise ValueError(f"ficha de {len(reg)} B en un formato de {self.formato.jugador_paso} B")
        o = self._off_jugador(indice)
        self.datos[o:o + len(reg)] = reg

    # -- utilidades ------------------------------------------------------------------------
    def _crc(self, inicio: int, largo: int) -> int:
        return zlib.crc32(self.datos[inicio:inicio + largo]) & 0xFFFFFFFF

    def _leer_u32(self, offset: int) -> int:
        return struct.unpack_from("<I", self.datos, offset)[0]


def detectar_formato(datos: bytes) -> FormatoPartida | None:
    """Formato de la recopilación por la cadena de versión de 0x04 (el IE1 no lleva el número)."""
    version = datos[0x04:0x14].split(b"\0")[0].decode("ascii", "replace")
    if "INAZUMA_ELEVEN" not in version:
        return None
    if "3" in version:
        return FORMATOS["ie3.123"]
    if "2" in version:
        return FORMATOS["ie2.123"]
    return FORMATOS["ie1.123"]
