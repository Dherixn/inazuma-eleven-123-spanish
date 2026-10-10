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

**En la recopilación cada juego guarda un archivo con tres espacios** (``ina1_save.dat``…), medido en
Azahar el 2026-10-10: la cabecera de 0x40 B es común y cada espacio ocupa 0x80 B de info más el bloque
de datos. Los offsets de arriba son los del espacio 1; el espacio k se desplaza k × (0x80 + datos).

**El IE1 de la recopilación tiene su propia ficha de jugador**: 32 fichas de 0x60 B desde 0x79C
(medido con la ficha de Mark y el hueco hasta el nombre del equipo, en 0x139C). EXP en +0 y número de
jugador en +4 están comprobados; el resto de campos del IE1, aún no, así que su formato no se escribe.
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
NUM_ESPACIOS = 3
CABECERA_LARGO = 0x40


@dataclass(frozen=True)
class FormatoPartida:
    nombre: str
    datos_largo: int
    jugadores_inicio: int
    jugador_paso: int
    verificado: bool
    max_jugadores: int = MAX_JUGADORES

    @property
    def espacio_largo(self) -> int:
        return 0x80 + self.datos_largo

    @property
    def archivo_largo(self) -> int:
        return CABECERA_LARGO + NUM_ESPACIOS * self.espacio_largo


FORMATOS = {
    "ie1.123": FormatoPartida("ie1.123", datos_largo=0x54C0, jugadores_inicio=0x79C, jugador_paso=0x60,
                              verificado=False, max_jugadores=32),
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

    def __init__(self, datos: bytes, formato: FormatoPartida, espacio: int = 0):
        if not 0 <= espacio < NUM_ESPACIOS:
            raise ValueError(f"espacio {espacio} fuera de rango (0-{NUM_ESPACIOS - 1})")
        self.base = espacio * formato.espacio_largo
        minimo = self.base + DATOS_INICIO + formato.datos_largo
        if len(datos) < minimo:
            raise ValueError(f"partida de {len(datos)} B; el espacio {espacio + 1} de {formato.nombre} necesita {minimo}")
        self.datos = bytearray(datos)
        self.formato = formato
        self.espacio = espacio

    @property
    def ocupado(self) -> bool:
        """El espacio tiene partida (lleva la marca)."""
        return self._leer_u32(self.base + OFF_MARCA) == MARCA

    # -- comprobaciones --------------------------------------------------------------------
    @property
    def version(self) -> str:
        return self.datos[0x04:0x14].split(b"\0")[0].decode("ascii", "replace")

    def sumas_correctas(self) -> dict[str, bool]:
        b = self.base
        return {
            "cabecera": self._crc(0x04, 0x3C) == self._leer_u32(OFF_CRC_CABECERA),
            "info": self._crc(b + INFO_INICIO, INFO_LARGO) == self._leer_u32(b + OFF_CRC_INFO),
            "datos": self._crc(b + DATOS_INICIO, self.formato.datos_largo) == self._leer_u32(b + OFF_CRC_DATOS),
        }

    def validar(self) -> None:
        malas = [k for k, ok in self.sumas_correctas().items() if not ok]
        if malas:
            raise ValueError(f"sumas de control incorrectas: {', '.join(malas)}")
        if not self.ocupado:
            raise ValueError(f"el espacio {self.espacio + 1} no tiene partida (falta la marca 0x{MARCA:08X})")

    def sellar(self) -> bytes:
        """Recalcula las sumas (datos, después info) y devuelve los bytes listos para guardar."""
        if not self.formato.verificado:
            raise ValueError(f"el formato {self.formato.nombre} no está verificado: no se escribe")
        b = self.base
        struct.pack_into("<I", self.datos, b + OFF_CRC_DATOS, self._crc(b + DATOS_INICIO, self.formato.datos_largo))
        struct.pack_into("<I", self.datos, b + OFF_CRC_INFO, self._crc(b + INFO_INICIO, INFO_LARGO))
        return bytes(self.datos)

    # -- jugadores -------------------------------------------------------------------------
    @property
    def num_jugadores(self) -> int:
        return self.datos[self.base + OFF_NUM_JUGADORES]

    def _off_jugador(self, indice: int) -> int:
        return self.base + self.formato.jugadores_inicio + indice * self.formato.jugador_paso

    def jugadores(self) -> list[JugadorPartida]:
        paso = self.formato.jugador_paso
        return [leer_jugador(self.datos[o:o + paso]) for o in map(self._off_jugador, range(self.num_jugadores))]

    def poner_jugador(self, indice: int, jugador: JugadorPartida) -> None:
        if not 0 <= indice < self.num_jugadores:
            raise IndexError(f"no hay jugador {indice} (fichados: {self.num_jugadores})")
        self._escribir_reg(indice, jugador)

    def anadir_jugador(self, jugador: JugadorPartida) -> int:
        indice = self.num_jugadores
        if indice >= self.formato.max_jugadores:
            raise ValueError(f"la plantilla ya tiene {self.formato.max_jugadores} jugadores")
        self._escribir_reg(indice, jugador)
        self.datos[self.base + OFF_NUM_JUGADORES] = indice + 1
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
