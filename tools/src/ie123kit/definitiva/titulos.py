"""Títulos de equipo del IE2 que no caben (#14): «Equipo Victori / oso».

La caja del título (menú del campo y ficha de la partida) pinta cada letra en su casilla y caben 14; 26 de
los 38 títulos traducidos de ``inazuma2/data_iz/logic/rpgtitle.STR`` son más largos y se parten a mitad de
palabra. Aquí se cambian por otros de 14 o menos con el mismo sentido. Solo es texto: misma codificación
(ASCII y los portadores griegos de los acentos, ``sjis_portador``), mismas ranuras de 32 B y la misma
fuente (tipografía v20 intacta). En el IE1 y el IE3 la caja es proporcional y caben todos.

Cada cambio va con el texto que se espera encontrar: si la traducción lo cambia, no se toca nada y avisa.
"""
from __future__ import annotations

from ie123kit.nucleo.texto.sjis_portador import GREEK, es_encode

RUTA = "inazuma2/data_iz/logic/rpgtitle.STR"
RANURA = 32
CASILLAS = 14

# desplazamiento: (lo que hay, lo que se pone)
CAMBIOS = {
    0x040: ("Cerrados en Banda", "Muro Defensivo"),
    0x080: ("Reyes del Aguante", "Aguante Total"),
    0x0A0: ("Goleadores de Oro", "Botas de Oro"),
    0x0E0: ("Orgullo de León", "Garra de León"),
    0x100: ("Ataque Pingüino", "Furia Pingüina"),
    0x120: ("Superarmadillos", "Los Armadillos"),
    0x160: ("Potros Enérgicos", "Potros Bravos"),
    0x180: ("Canguros Pichichis", "Canguros Gol"),
    0x1A0: ("Felinos Velocistas", "Felinos Raudos"),
    0x1C0: ("Titanes Físicos", "Titanes Duros"),
    0x1E0: ("Elegancia de Pies", "Pies de Seda"),
    0x240: ("Equipo Incansable", "Incansables"),
    0x260: ("Demonios del Gol", "Endiablados"),
    0x280: ("Equipo Saltarín", "Saltarines"),
    0x2A0: ("Equipo Intrépido", "Intrépidos"),
    0x2C0: ("Ases de los Pases", "Ases del Pase"),
    0x2E0: ("Bravos Defensores", "Defensa Brava"),
    0x320: ("Superresistentes", "Resistentes"),
    0x340: ("Arietes Imparables", "Imparables"),
    0x360: ("Demonios Raudos", "Diablos Raudos"),
    0x380: ("Equipo Pasional", "Pura Pasión"),
    0x3A0: ("Amigos del Balón", "Amigos del Gol"),
    0x3E0: ("Machos Musculitos", "Musculitos"),
    0x400: ("Amos del Gimnasio", "Amos del Gym"),
    0x420: ("Arietes Maníacos", "Arietes Locos"),
    0x480: ("Equipo Inseguro", "Inseguros"),
    0x4A0: ("Equipo Victorioso", "Victoriosos"),
}

# GREEK es una tabla de str.translate: código de la letra española → portador (un carácter griego). También
# pasa «ª» a «a» y «º» a «o»: esos no se invierten (solo los portadores, que no son ASCII).
_DESDE_PORTADOR = {ord(portador): letra for letra, portador in GREEK.items()
                   if isinstance(portador, str) and len(portador) == 1 and ord(portador) > 0x7F}


def decodificar(ranura: bytes) -> str:
    """Texto de una ranura con los acentos devueltos a su letra (inverso de ``es_encode``)."""
    texto = ranura.split(b"\0")[0].decode("shift_jis")
    return "".join(chr(_DESDE_PORTADOR.get(ord(c), ord(c))) for c in texto)


def codificar(texto: str) -> bytes:
    cod = es_encode(texto, RANURA - 1)
    if len(texto) > CASILLAS or cod.decode("shift_jis").translate(_DESDE_PORTADOR) != texto:
        raise ValueError(f"«{texto}» no cabe en {CASILLAS} casillas o no se codifica entero")
    return cod


def corregir(datos: bytes) -> tuple[bytes, list[str]]:
    """Devuelve rpgtitle.STR con los títulos acortados y la lista de los que no se tocaron (texto distinto)."""
    salida = bytearray(datos)
    saltados = []
    for off, (antes, despues) in CAMBIOS.items():
        actual = decodificar(datos[off:off + RANURA])
        if actual != antes:
            saltados.append(f"{off:#x}: se esperaba «{antes}» y hay «{actual}»")
            continue
        cod = codificar(despues)
        salida[off:off + RANURA] = cod + bytes(RANURA - len(cod))
    return bytes(salida), saltados
