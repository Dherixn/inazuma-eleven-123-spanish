"""Compara las tablas que el plugin construye desde archive.fa (C++) con las de ie123kit (Python).

    python plugin/pruebas/comparar_tablas.py RUTA/archive.fa RUTA/carpeta_logica

``carpeta_logica`` es la extracción de las tablas (inazumaN/data_iz/logic) del MISMO archive.fa. Ninguno
de los dos se sube a git (Norma 2). Termina con código 1 si algo difiere.
"""
from __future__ import annotations

import subprocess
import sys
import zlib
from pathlib import Path

AQUI = Path(__file__).resolve().parent
PLUGIN = AQUI.parent
sys.path.insert(0, str(PLUGIN.parent / "tools" / "src"))

from ie123kit.definitiva import datos_juego, fichas, identidades


def cargar(logica: Path, juego: str, perfil: str):
    p = fichas.PERFILES[perfil]
    carpeta = logica / juego / "data_iz/logic"
    ident = fichas.leer_identidades((carpeta / "unitbase.dat").read_bytes(), p, fichas.decodificar_sjis)
    crec = fichas.leer_crecimiento((carpeta / "unitstat.dat").read_bytes(), p)
    return identidades.jugadores(ident, crec), datos_juego.cargar(carpeta, p)


def volcado_python(logica: Path) -> list[str]:
    (j1, d1), (j2, d2), (j3, d3), (jo, do) = (cargar(logica, j, p) for j, p in (
        ("inazuma1", "3ds.ie1"), ("inazuma2", "3ds.ie2"), ("inazuma3", "3ds.ie3"), ("inazuma3_ogre", "3ds.ie3")))
    lineas = []
    for nombre, (jo_, do_), (jd_, dd_) in (("IE1_IE2", (j1, d1), (j2, d2)), ("IE2_IE3", (j2, d2), (j3, d3)),
                                           ("IE2_OGRO", (j2, d2), (jo, do))):
        pares = datos_juego.mapa_de_numeros(identidades.emparejar_por_cara(jo_, jd_), do_, dd_)
        lineas += [f"PAR {nombre} {a} {b}" for a, b in sorted(pares.items())]
    for nombre, d in (("IE2", d2), ("IE3", d3), ("OGRO", do)):
        for i, num in enumerate(d.numero):
            if d.ficha(num) != i:
                continue
            tec = d.aprendidas(num, 99)
            clave = zlib.crc32(d.personaje(num)) & 0xFFFFFFFF
            lineas.append(" ".join([f"FICHA {nombre} {num} {i} {d.exp_minima(num, 50)} {clave:08X} {len(tec)}",
                                    *map(str, tec)]).rstrip())
    return lineas


def main() -> int:
    archive, logica = Path(sys.argv[1]).resolve(), Path(sys.argv[2])
    orden = ("g++ -std=c++17 -O1 -Wall -D_GLIBCXX_ASSERTIONS -o /tmp/tablas pruebas/tablas_pc.cpp Sources/Nucleo/LectorFA.cpp "
             "Sources/Nucleo/DatosDestino.cpp Sources/Nucleo/Partida.cpp && /tmp/tablas /fa/archive.fa")
    r = subprocess.run(["docker", "run", "--rm", "-v", f"{PLUGIN}:/plugin", "-v", f"{archive.parent}:/fa",
                        "definitiva-3gx", "bash", "-c", orden], capture_output=True, text=True, check=False)
    if r.returncode:
        print(r.stdout[-2000:], r.stderr[-2000:])
        return 1
    cpp = [x.rstrip() for x in r.stdout.splitlines()]
    py = volcado_python(logica)
    print(f"C++: {len(cpp)} líneas; Python: {len(py)} líneas")
    distintas = [(a, b) for a, b in zip(cpp, py, strict=False) if a != b]
    for a, b in distintas[:10]:
        print(f"  c++: {a}\n  py:  {b}")
    iguales = len(cpp) == len(py) and not distintas
    print("IGUALES" if iguales else f"{len(distintas)} distintas")
    return 0 if iguales else 1


if __name__ == "__main__":
    sys.exit(main())
