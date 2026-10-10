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

**En la recopilación cada juego guarda su archivo de 64 KB** (medido en Azahar el 2026-10-10):
``ina1_save.dat`` con tres espacios y ``ina2F_save.dat`` (Tormenta de Fuego) con uno; además
``Menu.sav`` (24 B). La cabecera de 0x40 B es común y cada espacio ocupa 0x80 B de info más el bloque de
datos. Los offsets de arriba son los del espacio 1; el espacio k se desplaza k × (0x80 + datos).

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
# Resumen de la pantalla de carga: cuántos juegan (el grupo).
OFF_NUM_GRUPO = 0x56
# Los que juegan: hasta 16 (11 en el campo y 5 en el banquillo); el resto es la reserva.
MAX_GRUPO = 16
# Ficha IE2/IE3: dorsal y sitio en la formación (0 en la reserva; el juego los pone a 0 al guardar).
OFF_DORSAL = 0x57
OFF_SITIO = 0x58
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
    num_espacios: int = 1
    # Ficha de jugador. IE2/IE3: 6 técnicas u16 en +0x38, sus niveles en +0x44 y nivel en +0x4A.
    # IE1 de la recopilación: técnicas de 1 B (como en su unitstat), sin niveles, y nivel en +0x3E
    # (localizado en RAM: el menú muestra el valor de ese byte).
    off_nivel: int = 0x4A
    tecnicas_u8: bool = False
    # Lista del grupo (u16 por número de jugador, hasta MAX_GRUPO) tras los registros; 0 = sin localizar.
    off_grupo: int = 0
    # Al entrar en el grupo, la ficha recibe también su sitio en la formación (el IE3 lo deja a 0).
    sitio_al_entrar: bool = True
    # Campos de la ficha con su sitio en el equipo que no se heredan al crear una (offset, valor): dorsal,
    # sitio, órdenes (capitán, lanzadores), los cuatro de cabecera y, en el IE3, el equipo (el prólogo es
    # de Paolo Bianchi, del Orfeo: 0x36). Sin esto, los fichados copiaban los del capitán.
    reinicio: tuple[tuple[int, int], ...] = ()

    @property
    def espacio_largo(self) -> int:
        return 0x80 + self.datos_largo

    @property
    def archivo_largo(self) -> int:
        return CABECERA_LARGO + self.num_espacios * self.espacio_largo


FORMATOS = {
    "ie1.123": FormatoPartida("ie1.123", datos_largo=0x54C0, jugadores_inicio=0x79C, jugador_paso=0x60,
                              verificado=False, max_jugadores=32, num_espacios=3, off_nivel=0x3E,
                              tecnicas_u8=True),
    # Medido en la partida (2026-10-10): el grupo, 0x40 B detrás de los 100 registros; en la ficha, +0x5B
    # son las órdenes (0x10 | capitán y lanzadores) y +0x5D el puesto entre los cuatro de cabecera.
    "ie2.123": FormatoPartida("ie2.123", datos_largo=0x7DC0, jugadores_inicio=0x1268, jugador_paso=0x70, verificado=True,
                              off_grupo=0x1268 + MAX_JUGADORES * 0x70 + 0x40,
                              reinicio=((0x57, 0), (0x58, 0), (0x5B, 0x10), (0x5D, 0))),
    # IE3: +0x4B es el equipo (1 en los naturales). El grupo, 0x5E B detrás de los registros (localizado
    # metiendo a alguien en la lista en memoria y guardando desde el juego); al entrar, el juego le da dorsal
    # pero deja el sitio a 0.
    "ie3.123": FormatoPartida("ie3.123", datos_largo=0x5940, jugadores_inicio=0x11B0, jugador_paso=0x6C, verificado=True,
                              off_grupo=0x11B0 + MAX_JUGADORES * 0x6C + 0x5E, sitio_al_entrar=False,
                              reinicio=((0x4B, 1), (0x57, 0), (0x58, 0), (0x5A, 0), (0x5B, 0x10))),
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


def leer_jugador(reg: bytes, formato: FormatoPartida | None = None) -> JugadorPartida:
    formato = formato or FORMATOS["ie2.123"]
    if formato.tecnicas_u8:
        tecnicas, niveles = tuple(reg[0x38:0x38 + NUM_TECNICAS]), (0,) * NUM_TECNICAS
    else:
        tecnicas = struct.unpack_from(f"<{NUM_TECNICAS}H", reg, 0x38)
        niveles = tuple(reg[0x44:0x44 + NUM_TECNICAS])
    return JugadorPartida(
        exp=struct.unpack_from("<I", reg, 0x00)[0],
        id=struct.unpack_from("<H", reg, 0x04)[0],
        tecnicas=tecnicas,
        niveles_tecnica=niveles,
        nivel=reg[formato.off_nivel],
        crudo=bytes(reg),
    )


def escribir_jugador(jugador: JugadorPartida, formato: FormatoPartida | None = None) -> bytes:
    formato = formato or FORMATOS["ie2.123"]
    reg = bytearray(jugador.crudo)
    struct.pack_into("<I", reg, 0x00, jugador.exp)
    struct.pack_into("<H", reg, 0x04, jugador.id)
    if formato.tecnicas_u8:
        reg[0x38:0x38 + NUM_TECNICAS] = bytes(t & 0xFF for t in jugador.tecnicas)
    else:
        struct.pack_into(f"<{NUM_TECNICAS}H", reg, 0x38, *jugador.tecnicas)
        reg[0x44:0x44 + NUM_TECNICAS] = bytes(jugador.niveles_tecnica)
    reg[formato.off_nivel] = jugador.nivel
    return bytes(reg)


def normalizar_ficha_nueva(crudo: bytes, formato: FormatoPartida) -> bytes:
    """Quita a una ficha nueva el sitio en el equipo heredado de su molde (ver ``FormatoPartida.reinicio``)."""
    datos = bytearray(crudo)
    for off, valor in formato.reinicio:
        if off < len(datos):
            datos[off] = valor
    return bytes(datos)


class Partida:
    """Partida de un juego en memoria. Las ediciones no son válidas hasta :meth:`sellar`."""

    def __init__(self, datos: bytes, formato: FormatoPartida, espacio: int = 0):
        if not 0 <= espacio < formato.num_espacios:
            raise ValueError(f"espacio {espacio} fuera de rango (0-{formato.num_espacios - 1})")
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
        offsets = map(self._off_jugador, range(self.num_jugadores))
        return [leer_jugador(self.datos[o:o + paso], self.formato) for o in offsets]

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
        reg = escribir_jugador(jugador, self.formato)
        if len(reg) != self.formato.jugador_paso:
            raise ValueError(f"ficha de {len(reg)} B en un formato de {self.formato.jugador_paso} B")
        o = self._off_jugador(indice)
        self.datos[o:o + len(reg)] = reg

    # -- grupo (los que juegan) -------------------------------------------------------------
    def grupo(self) -> list[int]:
        """Números de la lista del grupo (0 = hueco); vacía si el formato no la tiene localizada."""
        if not self.formato.off_grupo:
            return []
        o = self.base + self.formato.off_grupo
        return [struct.unpack_from("<H", self.datos, o + 2 * k)[0] for k in range(MAX_GRUPO)]

    def _sembrar_grupo(self) -> None:
        """Lista vacía con grupo (IE3 tras el prólogo): el grupo son los primeros registros (tantos como el
        resumen). Se escriben antes de tocarla para no sacar a nadie."""
        if any(self.grupo()):
            return
        for k in range(min(self.datos[self.base + OFF_NUM_GRUPO], MAX_GRUPO, self.num_jugadores)):
            n = struct.unpack_from("<H", self.datos, self._off_jugador(k) + 4)[0]
            struct.pack_into("<H", self.datos, self.base + self.formato.off_grupo + 2 * k, n)

    def grupo_efectivo(self) -> list[int]:
        """Números del grupo tal como los ve el juego (con la lista vacía, los primeros registros)."""
        numeros = [n for n in self.grupo() if n]
        if numeros or not self.formato.off_grupo:
            return numeros
        cuantos = min(self.datos[self.base + OFF_NUM_GRUPO], MAX_GRUPO, self.num_jugadores)
        return [struct.unpack_from("<H", self.datos, self._off_jugador(k) + 4)[0] for k in range(cuantos)]

    def sacar_del_grupo(self, indice: int) -> bool:
        """Pasa al jugador a la reserva: fuera de la lista (los de detrás suben), dorsal y sitio a 0. Gemelo de
        ``Partida::SacarDelGrupo``."""
        if not self.formato.off_grupo:
            return False
        self._sembrar_grupo()
        lista = self.grupo()
        o_jugador = self._off_jugador(indice)
        numero = struct.unpack_from("<H", self.datos, o_jugador + 4)[0]
        if numero not in lista:
            return False
        k = lista.index(numero)
        lista = lista[:k] + lista[k + 1:] + [0]
        for m, n in enumerate(lista):
            struct.pack_into("<H", self.datos, self.base + self.formato.off_grupo + 2 * m, n)
        self.datos[o_jugador + OFF_DORSAL] = 0
        self.datos[o_jugador + OFF_SITIO] = 0
        self.datos[self.base + OFF_NUM_GRUPO] = sum(1 for n in lista if n)
        return True

    def meter_en_grupo(self, indice: int) -> bool:
        """Mete al jugador ``indice`` en el grupo si cabe (como un fichaje del juego con sitio): primer
        hueco de la lista, el dorsal libre más bajo y el sitio siguiente. False si no cabe o el formato no
        tiene el grupo localizado. Gemelo de ``Partida::MeterEnGrupo``."""
        if not self.formato.off_grupo:
            return False
        self._sembrar_grupo()
        lista = self.grupo()
        o_jugador = self._off_jugador(indice)
        numero = struct.unpack_from("<H", self.datos, o_jugador + 4)[0]
        if numero in lista:
            return True
        if 0 not in lista:
            return False
        en_grupo = {n for n in lista if n}
        dorsales = set()
        for k in range(self.num_jugadores):
            o = self._off_jugador(k)
            if struct.unpack_from("<H", self.datos, o + 4)[0] in en_grupo:
                dorsales.add(self.datos[o + OFF_DORSAL])
        dorsal = next(d for d in range(1, 256) if d not in dorsales)
        miembros = sum(1 for n in lista if n) + 1  # casillas ocupadas, como el C++
        struct.pack_into("<H", self.datos, self.base + self.formato.off_grupo + 2 * lista.index(0), numero)
        self.datos[o_jugador + OFF_DORSAL] = dorsal
        if self.formato.sitio_al_entrar:
            self.datos[o_jugador + OFF_SITIO] = miembros
        self.datos[self.base + OFF_NUM_GRUPO] = miembros
        return True

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
