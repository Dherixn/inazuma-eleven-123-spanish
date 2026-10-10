"""Objetos que la traducción dejó en japonés (IE3 y Ogro), empezando por los del contenido de StreetPass y Wi-Fi.

«Contenido de StreetPass y Wi-Fi» (plugin, Fichajes especiales) desbloquea los objetos que en DS se repartían por
Wi-Fi (``premium_item.dat``): manuales, uniformes y tarjetas de partidos. Sus nombres y descripciones seguían en
japonés, igual que los de otros objetos (llaves, botas de marca, manuales sueltos). Aquí van en español con las
convenciones de la traducción: «Tarjeta X» para las de partidos, «Equipo X» para los uniformes, el nombre traducido
de la técnica para los manuales (``command.STR``) y, en la descripción, «Elemento − PT − jugadores − tipo.» y
«Cuaderno de técnica…». Se dejan las reservas sin uso («システム予約», «(仮)») y el manual de una técnica sin nombre
ni en japonés (413).

``item.dat``: registros de 44 B ofuscados uno a uno (``fichas.desofuscar``); el nombre, 29 B al principio (media
anchura con los portadores de los acentos, como los demás); en +42, el índice ×32 de la descripción en
``item.STR`` (ancho completo). Las descripciones nuevas no caben en el sitio de las japonesas: van al final de
``item.STR`` y el índice apunta allí (las viejas se quedan sin usar). Solo es texto, con las codificaciones de la
traducción (tipografía v20 intacta). Cada cambio va con el nombre japonés que se espera encontrar: si la traducción
ya lo ha cambiado, no se toca y avisa.
"""
from __future__ import annotations

import struct
from collections.abc import Callable

from ie123kit.definitiva import fichas
from ie123kit.nucleo.texto.sjis_portador import GREEK, es_encode

JUEGOS = ("inazuma3", "inazuma3_ogre")
PASO = 44
NOMBRE = 29
OFF_DESCRIPCION = 42
RANURA = 32

TIRO, OFENSIVA, PORTERO = "Cuaderno de técnica de tiro.", "Cuaderno de técnica ofensiva.", "Cuaderno de técnica de portero."
OGRO = ("inazuma3_ogre",)
SOLO_IE3 = ("inazuma3",)
# objeto: (nombre japonés que se espera, nombre, descripción o None si ya está traducida[, juegos donde está])
CAMBIOS = {
    40: ("ウラゼウスチケット", "Tarjeta Ultra Zeus", "¡Tarjeta extra del Ultra Zeus!\n¿Cambiará la cadena de partidos?"),
    44: ("ジェネシスチケット", "Tarjeta Génesis", "¡Tarjeta extra del Génesis!\n¿Cambiará la cadena de partidos?"),
    47: ("やみのチケット", "Tarjeta de las sombras",
         "Una tarjeta envuelta en un aura oscura.\n¿Cambiará la cadena de partidos?"),
    115: ("カオスウェア", "Equipo Caos", "Mensajeros del caos.\nEl uniforme del equipo Caos."),
    521: ("ジェミニウェア", "Equipo Géminis", "Encierra el poder de las estrellas.\nEl uniforme de Tormenta de Géminis."),
    522: ("イプシロンウェア", "Equipo Épsilon", "Desborda energía.\nEl uniforme exclusivo del Épsilon."),
    523: ("ジェネシスウェア", "Equipo Génesis", "La prueba de los guerreros más fuertes.\nEl uniforme exclusivo del Génesis."),
    703: ("デスゾーン２", "Triángulo letal 2", f"Bosque − 61 PT − 3J.\n{TIRO}"),
    704: ("ジ・アース", "La Tierra", f"Montaña − 64 PT − 3J.\n{TIRO}"),
    740: ("ネオ・ギャラクシー", "Neo tiro galáctico", f"Fuego − 64 PT − 3J.\n{TIRO}"),
    746: ("グングニル", "Lanza de Odín", f"Aire − 66 PT − 1J − Tiro largo.\n{TIRO}"),
    771: ("トリプルダッシュ", "Sprint triple", f"Montaña − 57 PT − 3J.\n{OFENSIVA}"),
    822: ("ロックウォールダム", "Presa", "Bosque − 53 PT − 2J − Bloqueo.\nCuad. técnica defensiva. Jugadores grandes."),
    839: ("ドリルスマッシャー", "Destrozataladros", f"Fuego − 50 PT − 1J − Despeje.\n{PORTERO}"),
    852: ("プロキオンネット", "Constelación estelar", f"Aire − 52 PT − 1J − Atajo.\n{PORTERO}"),
    50: ("ワールドチケット", "Tarjeta Mundial", "¡Reúne a los rivales de todo el mundo!\n¿Cambiará la cadena de partidos?"),
    77: ("サッカー部室の鍵", "Llave del club de fútbol", "Abre el cuartito del club de fútbol."),
    78: ("体育館の鍵", "Llave del gimnasio", None, OGRO),
    79: ("駄菓子屋の鍵", "Llave de las chucherías",
         "Abre la puerta de la zona del gimnasio.\n¡Por aquí se llega a la tienda de chucherías!"),
    80: ("研究所の鍵", "Llave del laboratorio", None, OGRO),
    84: ("カノン家の鍵", "Llave de casa de Canon", None, OGRO),
    85: ("地下道の鍵", "Llave del túnel", None, OGRO),
    86: ("でんしょうの鍵", "Llave de la leyenda", None, OGRO),
    87: ("さらなるしょうこ", "Más pruebas", None),
    88: ("あくじのしょうこ", "Pruebas de sus fechorías", None),
    168: ("プレデターＸ", "Predator X", "Botas con una potencia de tiro\nabrumadora. Tiro +16."),
    170: ("Ｆ５０\u3000アディゼロ", "F50 adiZero", "Botas tan rápidas que rasgan\nel campo. Rapidez +16."),
    236: ("ブレイブショット", "Disparo valiente", f"Montaña − 76 PT − 1J − Tiro largo.\n{TIRO}"),
    237: ("カオスブレイク", "Remate caótico", f"Bosque − 67 PT − 3J.\n{TIRO}", SOLO_IE3),
    252: ("ゴッドキャノン", "Cañón celestial", f"Montaña − 55 PT − 1J − Cadena de tiros.\n{TIRO}"),
    276: ("ビッグスパイダー", "Araña gigante", f"Bosque − 47 PT − 1J − Atajo.\n{PORTERO}"),
    337: ("ドッペルゲンガー", "Doppelganger", None),
    642: ("警備兵グローブ", "Guantes de guardia", "Los guantes de los guardias."),
    755: ("てんくうおとし", "Descenso estelar", f"Fuego − 55 PT − 1J − Cadena de tiros.\n{TIRO}"),
    968: ("ロココウェア", "Equipo Rococo", "El uniforme de Rococo.", SOLO_IE3),
}


def codificar_nombre(nombre: str) -> bytes:
    cod = es_encode(nombre, 1000)
    if len(cod) > NOMBRE - 1 or cod.decode("shift_jis") != nombre.translate(GREEK):
        raise ValueError(f"«{nombre}» no cabe en {NOMBRE - 1} B o no se codifica entero")
    return cod


def traducir(item_dat: bytes, item_str: bytes, codificar: Callable[[str], bytes],
             juego: str | None = None) -> tuple[bytes, bytes, list[str]]:
    """``item.dat`` e ``item.STR`` de ``juego`` con los objetos de ``CAMBIOS`` en español, y los que no se tocaron
    (con otro nombre). ``codificar`` es el de ancho completo de la traducción."""
    dat, texto = bytearray(item_dat), bytearray(item_str)
    texto += bytes(-len(texto) % RANURA)
    saltados = []
    for indice, (japones, nombre, descripcion, *juegos) in CAMBIOS.items():
        if juegos and juego is not None and juego not in juegos[0]:
            continue
        registro = bytearray(fichas.desofuscar(dat[indice * PASO:(indice + 1) * PASO]))
        actual = bytes(registro[:NOMBRE]).split(b"\0")[0].decode("shift_jis", "replace")
        if actual != japones:
            saltados.append(f"objeto {indice}: se esperaba «{japones}» y hay «{actual}»")
            continue
        cod = codificar_nombre(nombre)
        registro[:NOMBRE] = cod + bytes(NOMBRE - len(cod))
        if descripcion is not None:
            struct.pack_into("<H", registro, OFF_DESCRIPCION, len(texto) // RANURA)
            d = codificar(descripcion) + b"\0"
            texto += d + bytes(-len(d) % RANURA)
        dat[indice * PASO:(indice + 1) * PASO] = fichas.ofuscar(bytes(registro))
    if len(texto) // RANURA > 0xFFFF:
        raise ValueError("item.STR pasa de 65535 ranuras")
    return bytes(dat), bytes(texto), saltados
