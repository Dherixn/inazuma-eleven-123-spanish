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
from pathlib import Path

from ie123kit.nucleo.contenedores.fa import FaArchive, reemplazar_entrada

LIMITE = 2**31 - 1


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
