"""Retoques de texturas de la traducción que se ven al usar la edición definitiva.

- ``menu/extra.arc``, ``ie99_extra_pas_open_mes_b02``: el aviso final del desbloqueo de StreetPass («Has
  desbloqueado todos los personajes y objetos», que la edición definitiva hace aparecer con «Contenido de
  StreetPass y Wi-Fi»). El panel se dibuja con dos trozos de esta textura; el pequeño, abajo (x 4-153, y 220-256),
  conservaba en sus últimas filas el arranque de un carácter del texto japonés, que en el juego sale como una «t»
  suelta a la derecha de «todos». Se tapa con el degradado limpio de la misma columna, unas filas más arriba.

Solo píxeles de fondo: el texto y la tipografía de la traducción no se tocan. Si la textura ya no tiene el resto
(la traducción lo arregla), no cambia nada.
"""
from __future__ import annotations

import tempfile
from pathlib import Path

from PIL import Image

from ie123kit.nucleo.contenedores.fa import FaArchive
from ie123kit.nucleo.graficos import texturas

ARC = "menu/extra.arc"
TEXTURA = "ie99_extra_pas_open_mes_b02.tga"
# Caja del resto (x0, y0, x1, y1, ambos incluidos) y fila limpia de la que se copia.
CAJA = (55, 250, 76, 255)
FILA_LIMPIA = 248


def tiene_resto(im: Image.Image) -> bool:
    px = im.load()
    x0, y0, x1, y1 = CAJA
    return any(px[x, y][3] and sum(px[x, y][:3]) / 3 < 120 for y in range(y0, y1 + 1) for x in range(x0, x1 + 1))


def tapar_resto(im: Image.Image) -> Image.Image:
    px = im.load()
    x0, y0, x1, y1 = CAJA
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            px[x, y] = px[x, FILA_LIMPIA]
    return im


def corregir(arc: FaArchive) -> dict[str, bytes]:
    """{ruta: bytes} de los .arc retocados (vacío si no hace falta)."""
    if not tiene_resto(texturas.load_texture(arc, ARC, TEXTURA)):
        return {}
    with tempfile.TemporaryDirectory() as tmp:
        texturas.apply_plan(arc, {ARC: {TEXTURA: tapar_resto}}, tmp, rewrap="keep")
        return {ARC: (Path(tmp) / ARC).read_bytes()}
