"""CLI de la edición definitiva.

    python -m ie123kit.definitiva identidades --ie1 DIR --ie2 DIR --ie3 DIR -o salida.csv
    python -m ie123kit.definitiva cabecera --ie1 DIR --ie2 DIR --ie3 DIR -o plugin/Sources/Nucleo/Equivalencias.inc
    python -m ie123kit.definitiva traspasar --logica DIR --origen ina1_save.dat --destino ina2F_save.dat \
        [--espacio-origen 1] [--modo completa|equilibrada|solo_fichajes] [--nivel N] -o salida.dat

Cada DIR es la carpeta que contiene ``unitbase.dat`` y ``unitstat.dat`` de ese juego (en la
recopilación: ``inazumaN/data_iz/logic`` dentro de archive.fa; en las ROM NDS: ``data_iz/logic/sp``
para IE1 e IE2, ``data_iz/logic`` para IE3). La cabecera del plugin se genera con la de 3DS.
"""
from __future__ import annotations

import argparse
import csv
import sys
from collections import Counter
from pathlib import Path

from ie123kit.definitiva import datos_juego, fichas, identidades, partida, traspaso


def _cargar(carpeta: Path, perfil: fichas.Perfil) -> list[identidades.Jugador]:
    decodificar = fichas.decodificar_sjis if perfil.nombre.startswith("3ds") else fichas.decodificar_latin
    base = fichas.leer_identidades((carpeta / "unitbase.dat").read_bytes(), perfil, decodificar)
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
    if args.plataforma == "3ds":
        ie1_ie2 = identidades.emparejar_por_cara(ie1, ie2)
        ie2_ie3 = identidades.emparejar_por_cara(ie2, ie3)
    else:
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


JUEGO_DE_FORMATO = {"ie1.123": ("inazuma1", "3ds.ie1"), "ie2.123": ("inazuma2", "3ds.ie2"),
                    "ie3.123": ("inazuma3", "3ds.ie3")}


def _traspasar(args: argparse.Namespace) -> int:
    """Traspaso de partida con las tablas de 3DS (``--logica``: carpeta con inazumaN/data_iz/logic)."""
    logica = Path(args.logica)
    bruto_o, bruto_d = Path(args.origen).read_bytes(), Path(args.destino).read_bytes()
    fmt_o, fmt_d = partida.detectar_formato(bruto_o), partida.detectar_formato(bruto_d)
    if not fmt_o or not fmt_d:
        raise SystemExit("alguna de las partidas no es de la recopilación")
    origen = partida.Partida(bruto_o, fmt_o, args.espacio_origen - 1)
    destino = partida.Partida(bruto_d, fmt_d, args.espacio_destino - 1)
    origen.validar()
    destino.validar()
    (j_o, p_o), (j_d, p_d) = JUEGO_DE_FORMATO[fmt_o.nombre], JUEGO_DE_FORMATO[fmt_d.nombre]
    carpeta_o, carpeta_d = logica / j_o / "data_iz/logic", logica / j_d / "data_iz/logic"
    jug_o = _cargar(carpeta_o, fichas.PERFILES[p_o])
    jug_d = _cargar(carpeta_d, fichas.PERFILES[p_d])
    datos_o = datos_juego.cargar(carpeta_o, fichas.PERFILES[p_o])
    datos = datos_juego.cargar(carpeta_d, fichas.PERFILES[p_d])
    # La partida guarda números de jugador; la tabla por cara va de ficha a ficha.
    mapa = datos_juego.mapa_de_numeros(identidades.emparejar_por_cara(jug_o, jug_d), datos_o, datos)
    op = traspaso.Opciones(
        modo=args.modo,
        nivel_tope=args.nivel if args.modo == traspaso.EQUILIBRADA else None,
        nivel_fichaje=args.nivel if args.modo == traspaso.SOLO_FICHAJES else None,
        exp_de_nivel=datos.exp_minima,
        aprendidas=datos.aprendidas,
        clave_destino=datos.personaje,
    )
    informe = traspaso.traspasar(origen, destino, mapa, op)
    Path(args.salida).write_bytes(destino.sellar())
    nombre = lambda numero: jug_d[datos.ficha(numero)].identidad.nombre
    print(f"traspasados {len(informe.traspasados)}: {[nombre(b) for _, b in informe.traspasados]}")
    print(f"mejorados {len(informe.mejorados)}; sin pareja {informe.sin_pareja}; sin sitio {informe.sin_sitio}")
    print(f"escrito: {args.salida}")
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
    c.add_argument("--plataforma", default="3ds", choices=("nds", "3ds"))
    c.add_argument("-o", "--salida", required=True)
    c.set_defaults(func=_cabecera)
    t = sub.add_parser("traspasar", help="lleva los fichajes de una partida al juego siguiente")
    t.add_argument("--logica", required=True)
    t.add_argument("--origen", required=True)
    t.add_argument("--destino", required=True)
    t.add_argument("--espacio-origen", type=int, default=1)
    t.add_argument("--espacio-destino", type=int, default=1)
    t.add_argument("--modo", default=traspaso.COMPLETA, choices=traspaso.MODOS)
    t.add_argument("--nivel", type=int)
    t.add_argument("-o", "--salida", required=True)
    t.set_defaults(func=_traspasar)
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
