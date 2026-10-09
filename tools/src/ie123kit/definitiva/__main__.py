"""CLI de la edición definitiva.

    python -m ie123kit.definitiva identidades --ie1 DIR --ie2 DIR --ie3 DIR -o salida.csv

Cada DIR es la carpeta que contiene ``unitbase.dat`` y ``unitstat.dat`` de ese juego
(en las ROM NDS: ``data_iz/logic/sp`` para IE1 e IE2, ``data_iz/logic`` para IE3).
"""
from __future__ import annotations

import argparse
import csv
import sys
from collections import Counter
from pathlib import Path

from ie123kit.definitiva import fichas, identidades


def _cargar(carpeta: Path, perfil: fichas.Perfil) -> list[identidades.Jugador]:
    base = fichas.leer_identidades((carpeta / "unitbase.dat").read_bytes(), perfil)
    stat = fichas.leer_crecimiento((carpeta / "unitstat.dat").read_bytes(), perfil)
    return identidades.jugadores(base, stat)


def _identidades(args: argparse.Namespace) -> int:
    ie1 = _cargar(Path(args.ie1), fichas.PERFILES[f"{args.plataforma}.ie1"])
    ie2 = _cargar(Path(args.ie2), fichas.PERFILES[f"{args.plataforma}.ie2"])
    ie3 = _cargar(Path(args.ie3), fichas.PERFILES[f"{args.plataforma}.ie3"])
    tabla = identidades.tabla(ie1, ie2, ie3)
    salida = Path(args.salida)
    salida.parent.mkdir(parents=True, exist_ok=True)
    with salida.open("w", newline="", encoding="utf-8") as fh:
        escritor = csv.writer(fh)
        escritor.writerow(identidades.COLUMNAS)
        escritor.writerows(identidades.a_filas(tabla))
    jugables = [e for e in tabla if e.jugable_ie2]
    print(f"{len(tabla)} fichas con nombre en IE2; {len(jugables)} jugables")
    print(f"  con pareja en IE1: {sum(e.ie1 is not None for e in jugables)}")
    print(f"  con pareja en IE3: {sum(e.ie3 is not None for e in jugables)}")
    for criterio, n in Counter(e.criterio_ie3 or "sin pareja" for e in jugables).most_common():
        print(f"    {criterio}: {n}")
    print(f"escrito: {salida}")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="python -m ie123kit.definitiva")
    sub = parser.add_subparsers(dest="orden", required=True)
    p = sub.add_parser("identidades", help="tabla de equivalencias de jugadores IE1/IE2/IE3")
    p.add_argument("--ie1", required=True)
    p.add_argument("--ie2", required=True)
    p.add_argument("--ie3", required=True)
    p.add_argument("--plataforma", default="nds", choices=("nds",))
    p.add_argument("-o", "--salida", required=True)
    p.set_defaults(func=_identidades)
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
