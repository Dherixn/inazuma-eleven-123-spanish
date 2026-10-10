"""Edición definitiva: retoque de la textura del aviso final de StreetPass (imagen sintética)."""

from PIL import Image

from ie123kit.definitiva import texturas_menu


def test_tapa_el_resto_con_el_degradado_de_la_columna():
    im = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    for x in range(4, 154):
        for y in range(220, 256):
            im.putpixel((x, y), (200 + x % 50, 180, 90, 255))  # degradado horizontal
    for x in range(59, 72):
        im.putpixel((x, 255), (20, 20, 20, 255))  # el arranque del carácter japonés
    assert texturas_menu.tiene_resto(im)
    limpia = texturas_menu.tapar_resto(im.copy())
    assert not texturas_menu.tiene_resto(limpia)
    assert limpia.getpixel((60, 255)) == im.getpixel((60, texturas_menu.FILA_LIMPIA))
    assert limpia.getpixel((100, 255)) == im.getpixel((100, 255))  # fuera de la caja, igual
