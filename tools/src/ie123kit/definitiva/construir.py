"""Capa de datos de la edición definitiva: un archive.fa con los archivos del mod, encima de la traducción.

Se parte del archive.fa ya traducido (nunca del japonés: si no, el mod pisaría la traducción), se copia y
cada archivo cambiado se añade al final y se reapunta su entrada (``nucleo.contenedores.fa
.reemplazar_entrada``, la misma primitiva que usan las capas de la traducción). Lo que no cambia queda
igual byte a byte. El resultado se pone en una carpeta de mod de Azahar/Luma junto al resto de archivos
de la traducción.

Límite: el archive.fa traducido mide 1,57 GB; el plugin lo lee con desplazamientos de 32 bits con signo,
así que el resultado no debe pasar de 2 GiB.
"""
from __future__ import annotations

import shutil
from collections.abc import Callable
from pathlib import Path

from ie123kit.definitiva import aiden, objetos, titulos
from ie123kit.nucleo.contenedores.fa import FaArchive, reemplazar_entrada

LIMITE = 2**31 - 1
LOGICA = "{juego}/data_iz/logic/"


def _tablas(arc: FaArchive, juego: str) -> aiden.Tablas:
    base = LOGICA.format(juego=juego)
    leer = lambda f: arc.read(base + f) if arc.exists(base + f) else b""  # noqa: E731
    return aiden.Tablas(*(leer(f) for f in ("unitbase.dat", "unitstat.dat", "unitno.dat", "unitbase.STR",
                                            "uschool.dat", "usearch.dat")))


def _cambios_de(juego: str, antes: aiden.Tablas, despues: aiden.Tablas) -> dict[str, bytes]:
    base = LOGICA.format(juego=juego)
    nombres = {"unitbase": "unitbase.dat", "unitstat": "unitstat.dat", "unitno": "unitno.dat", "textos": "unitbase.STR",
               "uschool": "uschool.dat", "usearch": "usearch.dat"}
    return {base + archivo: getattr(despues, campo) for campo, archivo in nombres.items()
            if getattr(despues, campo) != getattr(antes, campo)}


def capa_definitiva(arc: FaArchive, codificar: Callable[[str], bytes] | None = None) -> tuple[dict[str, bytes], list[str]]:
    """Archivos que cambia la edición definitiva sobre el archive.fa traducido, y un resumen legible.

    ``codificar`` es el de ancho completo de la traducción (por defecto, ``ancho_completo.encode_fullwidth``).
    """
    if codificar is None:
        from ie123kit.nucleo.texto import ancho_completo
        codificar = ancho_completo.encode_fullwidth
    cambios: dict[str, bytes] = {}
    resumen = []
    antes = _tablas(arc, "inazuma2")
    cambios |= _cambios_de("inazuma2", antes, aiden.ie2(antes, codificar))
    resumen.append(f"inazuma2: Aiden Froste jugable (ficha {aiden.FICHA_IE2}, número {aiden.NUMERO_IE2})")
    for juego in ("inazuma3", "inazuma3_ogre"):
        antes = _tablas(arc, juego)
        despues, ficha, numero = aiden.ie3(antes, codificar)
        cambios |= _cambios_de(juego, antes, despues)
        resumen.append(f"{juego}: Aiden Froste en la ficha {ficha}, número {numero}")
    for juego in objetos.JUEGOS:
        base = f"{juego}/data_iz/logic/"
        dat, texto, saltados = objetos.traducir(arc.read(base + "item.dat"), arc.read(base + "item.STR"), codificar, juego)
        cambios[base + "item.dat"], cambios[base + "item.STR"] = dat, texto
        hechos = sum(1 for e in objetos.CAMBIOS.values() if len(e) < 4 or juego in e[3]) - len(saltados)
        resumen.append(f"{juego}: {hechos} objetos que seguían en japonés, en español")
        resumen += [f"  aviso: {s}" for s in saltados]
    rpgtitle, saltados = titulos.corregir(arc.read(titulos.RUTA))
    cambios[titulos.RUTA] = rpgtitle
    resumen.append(f"inazuma2: {len(titulos.CAMBIOS) - len(saltados)} títulos de equipo acortados a {titulos.CASILLAS} casillas")
    resumen += [f"  aviso: {s}" for s in saltados]
    return cambios, resumen


def construir_archive(origen: Path, destino: Path, cambios: dict[str, bytes]) -> dict[str, int]:
    """Copia ``origen`` en ``destino`` y sustituye en él las entradas de ``cambios`` (ruta → bytes)."""
    if origen.resolve() == destino.resolve():
        raise ValueError("origen y destino son el mismo archivo")
    arc = FaArchive(str(origen))
    faltan = [r for r in cambios if not arc.exists(r)]
    if faltan:
        raise KeyError(f"no están en archive.fa: {', '.join(faltan)}")
    destino.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(origen, destino)
    with destino.open("r+b") as fh:
        for ruta, datos in cambios.items():
            reemplazar_entrada(fh, arc, ruta, datos)
        fh.seek(0, 2)
        tam = fh.tell()
    if tam > LIMITE:
        raise ValueError(f"archive.fa resultante de {tam} B pasa de 2 GiB")
    nuevo = FaArchive(str(destino))
    for ruta, datos in cambios.items():
        if nuevo.read(ruta) != datos:
            raise AssertionError(f"{ruta}: la entrada no lee lo escrito")
    return {"tamano": tam, "cambios": len(cambios)}
