"""``data_iz/INAZUMA.INI``: ajustes de juego en texto (Shift-JIS, CRLF) que el motor lee al arrancar.

Se trabaja sobre bytes: las claves y los valores son ASCII, y los comentarios en japonés se conservan tal
cual. Solo se cambian claves que ya existen; una clave que no está es un error, porque el motor no lee
claves que no conoce.

Las ayudas se eligen en el plugin, que cambia la tabla del INI en memoria al momento (plugin/Sources/
Ayudas.cpp; ahí está cuándo lee el juego cada clave). ``AYUDAS`` repite los mismos niveles para fijarlos en
el propio INI del mod, para quien juegue sin plugin.

Lo que dice el código de los tres módulos (ina_main1/2/3ogre.cro): RPG_ENCOUNT_TIME se relee cada vez que
se reinicia el contador de encuentros; RPG_MOVESPEED, al entrar en el modo campo; CPU_USE_SPECIAL_PERCENT,
ITEM_SELL_PERCENT y OFFSIDE_EFFECTIVE, una vez al cargar el juego. SPECIAL_SKILL_SPEND_TP y FUSEI_ITEM_NO
no los lee ningún módulo (restos de la versión de DS): cambiarlos no hace nada.
"""
from __future__ import annotations

import re
from dataclasses import dataclass

_LINEA = re.compile(rb"^(?P<clave>[A-Z0-9_]+)(?P<sep>\s*=\s*)(?P<valor>[^\s;]*)(?P<resto>.*)$")


def leer(ini: bytes) -> dict[str, str]:
    """Claves con su valor (las comentadas con ``;`` no cuentan)."""
    salida = {}
    for linea in ini.split(b"\r\n"):
        if m := _LINEA.match(linea):
            salida[m["clave"].decode("ascii")] = m["valor"].decode("ascii")
    return salida


def cambiar(ini: bytes, valores: dict[str, str | int]) -> bytes:
    """Devuelve el INI con esos valores; el resto de bytes, idéntico."""
    pendientes = {k: str(v).encode("ascii") for k, v in valores.items()}
    lineas = ini.split(b"\r\n")
    for i, linea in enumerate(lineas):
        m = _LINEA.match(linea)
        if m and (clave := m["clave"].decode("ascii")) in pendientes:
            lineas[i] = m["clave"] + m["sep"] + pendientes.pop(clave) + m["resto"]
    if pendientes:
        raise KeyError(f"claves que no están en el INI: {', '.join(sorted(pendientes))}")
    return b"\r\n".join(lineas)


@dataclass(frozen=True)
class Ayuda:
    nombre: str
    descripcion: str
    niveles: dict[str, dict[str, int]]


AYUDAS = {
    "encuentros": Ayuda(
        "Encuentros aleatorios",
        "RPG_ENCOUNT_TIME: base del contador de pasos entre encuentros (×30); más alto, menos encuentros.",
        {"normal": {"RPG_ENCOUNT_TIME": 60}, "menos": {"RPG_ENCOUNT_TIME": 150},
         "muy_pocos": {"RPG_ENCOUNT_TIME": 400}, "ninguno": {"RPG_ENCOUNT_TIME": 99999},
         "mas": {"RPG_ENCOUNT_TIME": 25}},
    ),
    "velocidad": Ayuda(
        "Velocidad al andar",
        "RPG_MOVESPEED: velocidad del personaje por el mapa (con 5555 se sale del escenario).",
        {"normal": {"RPG_MOVESPEED": 320}, "rapida": {"RPG_MOVESPEED": 448}, "muy_rapida": {"RPG_MOVESPEED": 576}},
    ),
    "rival": Ayuda(
        "Supertécnicas del rival",
        "CPU_USE_SPECIAL_PERCENT: probabilidad de que la CPU use supertécnica (el juego la limita a 0..100).",
        {"normal": {"CPU_USE_SPECIAL_PERCENT": 40}, "pocas": {"CPU_USE_SPECIAL_PERCENT": 15},
         "muchas": {"CPU_USE_SPECIAL_PERCENT": 70}, "siempre": {"CPU_USE_SPECIAL_PERCENT": 100}},
    ),
    "venta": Ayuda(
        "Precio al vender objetos",
        "ITEM_SELL_PERCENT: porcentaje del precio que pagan las tiendas.",
        {"normal": {"ITEM_SELL_PERCENT": 50}, "completo": {"ITEM_SELL_PERCENT": 100}},
    ),
    "fueradejuego": Ayuda(
        "Fuera de juego",
        "OFFSIDE_EFFECTIVE: 1 se pita, 0 no.",
        {"se_pita": {"OFFSIDE_EFFECTIVE": 1}, "no_se_pita": {"OFFSIDE_EFFECTIVE": 0}},
    ),
}


def aplicar_ayudas(ini: bytes, elegidas: dict[str, str]) -> bytes:
    """``elegidas``: ayuda → nivel (p. ej. ``{"encuentros": "menos"}``)."""
    valores: dict[str, int] = {}
    for ayuda, nivel in elegidas.items():
        if ayuda not in AYUDAS:
            raise KeyError(f"ayuda desconocida: {ayuda} (hay: {', '.join(AYUDAS)})")
        if nivel not in AYUDAS[ayuda].niveles:
            raise KeyError(f"{ayuda}: nivel desconocido {nivel} (hay: {', '.join(AYUDAS[ayuda].niveles)})")
        valores.update(AYUDAS[ayuda].niveles[nivel])
    return cambiar(ini, valores)
