"""CLI de la edición definitiva.

    python -m ie123kit.definitiva identidades --ie1 DIR --ie2 DIR --ie3 DIR -o salida.csv
    python -m ie123kit.definitiva cabecera --ie1 DIR --ie2 DIR --ie3 DIR -o plugin/Sources/Nucleo/Equivalencias.inc

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


def _tres_juegos(args: argparse.Namespace):
    return tuple(_cargar(Path(getattr(args, j)), fichas.PERFILES[f"{args.plataforma}.{j}"]) for j in ("ie1", "ie2", "ie3"))


def _identidades(args: argparse.Namespace) -> int:
    ie1, ie2, ie3 = _tres_juegos(args)
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


def _pares_cpp(nombre: str, pares: dict[int, int]) -> str:
    filas = [f"    {{{a}, {b}}}," for a, b in sorted(pares.items())]
    return f"static const Definitiva::Par {nombre}[] = {{\n" + "\n".join(filas) + "\n};\n"


def _cabecera(args: argparse.Namespace) -> int:
    ie1, ie2, ie3 = _tres_juegos(args)
    ie1_ie2 = identidades.ie1_a_ie2(ie1, ie2)
    ie2_ie3 = {a: b for a, (b, _) in identidades.emparejar(ie2, ie3).items()}
    texto = (
        "// Generado por `python -m ie123kit.definitiva cabecera` a partir de las fichas de jugador"
        f" ({args.plataforma}). No editar a mano.\n"
        "// Pares (número en el juego de origen, número en el de destino), ordenados por origen.\n\n"
        + _pares_cpp("EQUIV_IE1_IE2", ie1_ie2) + "\n" + _pares_cpp("EQUIV_IE2_IE3", ie2_ie3)
    )
    salida = Path(args.salida)
    salida.parent.mkdir(parents=True, exist_ok=True)
    salida.write_text(texto, encoding="utf-8", newline="\n")
    print(f"IE1→IE2: {len(ie1_ie2)} pares; IE2→IE3: {len(ie2_ie3)} pares; escrito: {salida}")
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
    c = sub.add_parser("cabecera", help="tablas de equivalencias en C++ para el plugin")
    for juego in ("ie1", "ie2", "ie3"):
        c.add_argument(f"--{juego}", required=True)
    c.add_argument("--plataforma", default="nds", choices=("nds",))
    c.add_argument("-o", "--salida", required=True)
    c.set_defaults(func=_cabecera)
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
