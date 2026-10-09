"""``data_iz/INAZUMA.INI``: ajustes de juego en texto (Shift-JIS, CRLF) que el motor lee al arrancar.

Se trabaja sobre bytes: las claves y los valores son ASCII, y los comentarios en japonés se conservan tal
cual. Solo se cambian claves que ya existen; una clave que no está es un error, porque el motor no lee
claves que no conoce.

Las ayudas que se eligen al instalar salen de aquí (``AYUDAS``). Los valores están puestos a ojo a
partir del de fábrica y se ajustarán al probarlos en el juego (#7, #11).
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
        "RPG_ENCOUNT_TIME es el intervalo entre comprobaciones de encuentro: más alto, menos encuentros.",
        {"normal": {"RPG_ENCOUNT_TIME": 60}, "menos": {"RPG_ENCOUNT_TIME": 120}, "pocos": {"RPG_ENCOUNT_TIME": 240}},
    ),
    "velocidad": Ayuda(
        "Velocidad al andar",
        "RPG_MOVESPEED: velocidad del personaje por el mapa.",
        {"normal": {"RPG_MOVESPEED": 320}, "rapida": {"RPG_MOVESPEED": 480}},
    ),
    "cpu": Ayuda(
        "Supertécnicas de la CPU",
        "CPU_USE_SPECIAL_PERCENT: probabilidad de que el rival use supertécnica (parte del selector de dificultad).",
        {"normal": {"CPU_USE_SPECIAL_PERCENT": 40}, "dificil": {"CPU_USE_SPECIAL_PERCENT": 60},
         "extremo": {"CPU_USE_SPECIAL_PERCENT": 80}},
    ),
    "pt": Ayuda(
        "Gasto de PT",
        "SPECIAL_SKILL_SPEND_TP: si las supertécnicas gastan PT.",
        {"normal": {"SPECIAL_SKILL_SPEND_TP": 1}, "infinitos": {"SPECIAL_SKILL_SPEND_TP": 0}},
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
