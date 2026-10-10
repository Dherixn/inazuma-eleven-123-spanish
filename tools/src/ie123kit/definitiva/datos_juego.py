"""Lo que el traspaso necesita saber del juego de destino: curva de EXP y técnicas por nivel de cada jugador.

- La curva de EXP de un jugador es el primer byte de su bloque de identidad (``unitbase``): Mark lleva
  la 6 en los tres juegos. Comprobado en Azahar: con 470 de EXP a nivel 15 el IE2 muestra que le
  faltan 254, que es el umbral del nivel 16 en la curva 6 (724).
- ``lvup_exp.dat`` tiene 98 valores u32 por curva (EXP acumulada de los niveles 2…99): 16 curvas en el
  IE2 y el IE3, 56 en el IE1 de la recopilación. Las del IE2 y el IE3 **no** son iguales en 3DS, así que
  la EXP no se copia: se recalcula a partir del nivel con la curva del juego de destino.
- Las técnicas que un jugador aprende por nivel son las de su ``unitstat`` con nivel ≤ el suyo; el
  nivel 100 marca las que da la historia o un manual.
- **Personaje** (para no duplicar a nadie en la plantilla): el nombre completo de la ficha, que es el
  mismo en las variantes de un personaje con otra cara (Jude del Royal, cara 0x1E, y del Raimon, 0x449,
  en el IE2); si la ficha no tiene nombre completo, su cara.
- **La partida guarda el número de jugador, no el índice de la ficha.** ``unitno.dat`` da el número de
  cada ficha (u16 por ficha): en los primeros coinciden, pero Jude es la ficha 1024 del IE2 con el
  número 1097. Todos los métodos de :class:`DatosJuego` reciben el **número**, como en la partida.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

from ie123kit.definitiva.fichas import NIVEL_NO_POR_NIVEL, Perfil, leer_crecimiento

VALORES_POR_CURVA = 98


def leer_curvas(lvup_exp: bytes) -> list[list[int]]:
    paso = VALORES_POR_CURVA * 4
    if not lvup_exp or len(lvup_exp) % paso:
        raise ValueError(f"lvup_exp.dat de {len(lvup_exp)} B no es múltiplo de {paso}")
    valores = struct.unpack(f"<{len(lvup_exp) // 4}I", lvup_exp)
    return [list(valores[c * VALORES_POR_CURVA:(c + 1) * VALORES_POR_CURVA]) for c in range(len(lvup_exp) // paso)]


@dataclass(frozen=True)
class DatosJuego:
    """Tablas por ficha (índice) y su traducción desde el número de jugador de la partida."""

    curvas: list[list[int]]
    curva: list[int]
    aprende: list[tuple[tuple[int, int], ...]]
    numero: list[int]
    caras: list[int]
    indice_de: dict[int, int]
    nombres: list[bytes]

    def ficha(self, numero: int) -> int:
        try:
            return self.indice_de[numero]
        except KeyError:
            raise KeyError(f"el número de jugador {numero} no tiene ficha en este juego") from None

    def exp_minima(self, numero: int, nivel: int) -> int:
        if not 1 <= nivel <= 99:
            raise ValueError(f"nivel fuera de rango: {nivel}")
        return 0 if nivel == 1 else self.curvas[self.curva[self.ficha(numero)]][nivel - 2]

    def aprendidas(self, numero: int, nivel: int) -> list[int]:
        """Técnicas que el jugador ya sabe a ese nivel en este juego, en su orden."""
        aprende = self.aprende[self.ficha(numero)]
        return [t for t, lv in aprende if t and lv < NIVEL_NO_POR_NIVEL and lv <= nivel]

    def cara(self, numero: int) -> int:
        return self.caras[self.ficha(numero)]

    def personaje(self, numero: int) -> bytes:
        """Clave del personaje: nombre completo de su ficha o, si está vacío, su cara."""
        i = self.ficha(numero)
        return self.nombres[i] or f"cara:{self.caras[i]}".encode()


def leer_numeros(unitno: bytes) -> list[int]:
    return list(struct.unpack(f"<{len(unitno) // 2}H", unitno))


def construir(unitbase: bytes, unitstat: bytes, lvup_exp: bytes, unitno: bytes, perfil: Perfil) -> DatosJuego:
    total = len(unitbase) // perfil.base_paso
    curva = [unitbase[i * perfil.base_paso + perfil.off_identidad] for i in range(total)]
    caras = [struct.unpack_from("<H", unitbase, i * perfil.base_paso + perfil.off_cara)[0] for i in range(total)]
    crecimiento = leer_crecimiento(unitstat, perfil)
    numero = leer_numeros(unitno)
    o, a = perfil.off_nombre_completo, perfil.ancho_nombre_completo
    # Sin espacios de ancho completo: algunas fichas del IE1 solo llevan uno.
    nombres = [unitbase[i * perfil.base_paso + o:i * perfil.base_paso + o + a].split(b"\0")[0]
               .replace("\u3000".encode("shift_jis"), b"").strip() for i in range(total)]
    if not len(crecimiento) == len(numero) == total:
        raise ValueError(f"unitbase ({total}), unitstat ({len(crecimiento)}) y unitno ({len(numero)}) no cuadran")
    indice_de: dict[int, int] = {}
    for i, n in enumerate(numero):
        indice_de.setdefault(n, i)
    return DatosJuego(curvas=leer_curvas(lvup_exp), curva=curva, aprende=[c.tecnicas for c in crecimiento],
                      numero=numero, caras=caras, indice_de=indice_de, nombres=nombres)


def cargar(carpeta: Path, perfil: Perfil) -> DatosJuego:
    return construir(*((carpeta / n).read_bytes() for n in ("unitbase.dat", "unitstat.dat", "lvup_exp.dat",
                                                            "unitno.dat")), perfil)


def mapa_de_numeros(por_ficha: dict[int, int], origen: DatosJuego, destino: DatosJuego) -> dict[int, int]:
    """Pasa una tabla ficha → ficha (p. ej. por cara) a número de jugador → número de jugador."""
    return {origen.numero[a]: destino.numero[b] for a, b in por_ficha.items()}
