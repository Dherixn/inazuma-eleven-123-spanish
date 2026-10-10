"""CLI de la edición definitiva.

    python -m ie123kit.definitiva identidades --ie1 DIR --ie2 DIR --ie3 DIR -o salida.csv
    python -m ie123kit.definitiva traspasar --logica DIR --origen ina1_save.dat --destino ina2F_save.dat \
        [--espacio-origen 1] [--modo completa|equilibrada|solo_fichajes] [--nivel N] [--memoria SD/Inazuma123Definitiva] \
        -o salida.dat
    python -m ie123kit.definitiva construir --base archive.fa_traducido -o archive.fa
    python -m ie123kit.definitiva paquete --mod .../load/mods/00040000000BB800 --plugin Definitiva-release.3gx -o dist/X
    python -m ie123kit.definitiva saltos --jp base/romfs/archive.fa --es .../romfs/archive.fa

Cada DIR es la carpeta que contiene ``unitbase.dat`` y ``unitstat.dat`` de ese juego (en la
recopilación: ``inazumaN/data_iz/logic`` dentro de archive.fa; en las ROM NDS: ``data_iz/logic/sp``
para IE1 e IE2, ``data_iz/logic`` para IE3). El plugin no lleva estas tablas: las lee de archive.fa al
ejecutarse (Norma 2: nada del juego en el repositorio ni en el binario).
"""
from __future__ import annotations

import argparse
import csv
import sys
import zlib
from collections import Counter
from pathlib import Path

from ie123kit.definitiva import aiden, datos_juego, fichas, identidades, partida, traspaso


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


JUEGO_DE_FORMATO = {"ie1.123": ("inazuma1", "3ds.ie1"), "ie2.123": ("inazuma2", "3ds.ie2"),
                    "ie3.123": ("inazuma3", "3ds.ie3")}


def _altas(args: argparse.Namespace, logica: Path, juego: str) -> set[int]:
    """Altas de la historia del juego: de ``--fa`` si se da, si no de los scripts extraídos en ``--logica``."""
    from ie123kit.definitiva import altas as altas_mod
    if args.fa:
        from ie123kit.nucleo.contenedores.fa import FaArchive
        fa = FaArchive(str(args.fa))
        return altas_mod.altas(lambda ruta: fa.read(ruta) if fa.exists(ruta) else None, juego)
    return altas_mod.altas(lambda ruta: (logica / ruta).read_bytes() if (logica / ruta).is_file() else None, juego)


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
    # Las partidas de IE3 tienen el mismo formato en las tres versiones; Ogro tiene carpeta propia.
    j_o, j_d = args.juego_origen or j_o, args.juego_destino or j_d
    carpeta_o, carpeta_d = logica / j_o / "data_iz/logic", logica / j_d / "data_iz/logic"
    jug_o = _cargar(carpeta_o, fichas.PERFILES[p_o])
    jug_d = _cargar(carpeta_d, fichas.PERFILES[p_d])
    datos_o = datos_juego.cargar(carpeta_o, fichas.PERFILES[p_o])
    datos = datos_juego.cargar(carpeta_d, fichas.PERFILES[p_d])
    # La partida guarda números de jugador; la tabla por cara va de ficha a ficha.
    por_ficha = identidades.emparejar_por_cara(jug_o, jug_d)
    # La ficha que la historia del juego nuevo vuelve a fichar (Jude, en el IE3, es la 30), como el plugin.
    altas = _altas(args, logica, j_d)
    if altas:
        por_ficha = identidades.preferir_altas(por_ficha, jug_d, datos, altas)
    else:
        print("aviso: sin los eventos del juego de destino (--fa o inazumaN/data_iz/script en --logica); "
              "el traspaso no puede saber qué ficha vuelve a fichar la historia")
    mapa = aiden.ajustar_mapa(datos_juego.mapa_de_numeros(por_ficha, datos_o, datos), j_o, j_d)
    op = traspaso.Opciones(
        modo=args.modo,
        nivel_tope=args.nivel if args.modo == traspaso.EQUILIBRADA else None,
        nivel_fichaje=args.nivel if args.modo == traspaso.SOLO_FICHAJES else None,
        exp_de_nivel=datos.exp_minima,
        aprendidas=datos.aprendidas,
        clave_destino=datos.personaje,
        # A quien la historia va a unir no se le crea ficha: llega con ella (traspaso.py).
        aplazar=frozenset(altas) or None,
    )
    informe = traspaso.traspasar(origen, destino, mapa, op)
    if informe.aplazados and args.memoria:
        _esperar(Path(args.memoria), destino, informe.aplazados, datos)
    Path(args.salida).write_bytes(destino.sellar())
    nombre = lambda numero: jug_d[datos.ficha(numero)].identidad.nombre
    print(f"traspasados {len(informe.traspasados)} (a la reserva): {[nombre(b) for _, b in informe.traspasados]}")
    print(f"llegan con la historia {len(informe.aplazados)}: {[nombre(a.destino) for a in informe.aplazados]}")
    if informe.aplazados and not args.memoria:
        print("aviso: sin --memoria, los que llegan con la historia llegarán sin lo que traen (el plugin se lo "
              "devuelve con el equipo_XXXXXXXX.txt de la carpeta Inazuma123Definitiva de la SD)")
    print(f"mejorados {len(informe.mejorados)}; sin pareja {informe.sin_pareja}; sin sitio {informe.sin_sitio}")
    print(f"escrito: {args.salida}")
    return 0


def _esperar(carpeta: Path, destino: partida.Partida, aplazados, datos) -> None:
    """Apunta en la memoria del equipo de la partida de destino a los que llegan con la historia (como el plugin:
    PlantillaEnJuego.cpp, LeerArchivo; una memoria de una partida anterior sobre el mismo archivo se aparta)."""
    from ie123kit.definitiva.memoria_equipo import MemoriaEquipo, Recuerdo

    ruta = carpeta / f"equipo_{destino.identificador:08X}.txt"
    memoria = MemoriaEquipo.cargar(ruta.read_text(encoding="utf-8")) if ruta.is_file() else MemoriaEquipo()
    if memoria.tiempo > destino.tiempo_de_juego:
        ruta.replace(ruta.with_name(ruta.name + ".anterior"))
        memoria = MemoriaEquipo()
    memoria.partida, memoria.tiempo = destino.identificador, max(memoria.tiempo, destino.tiempo_de_juego)
    for a in aplazados:
        tecnicas = [(t, n) for t, n in zip(a.tecnicas, a.niveles_tecnica, strict=True) if t]
        memoria.esperar(zlib.crc32(datos.personaje(a.destino)) & 0xFFFFFFFF,
                        Recuerdo(numero=a.destino, nivel=a.nivel, exp=a.exp, tecnicas=tecnicas))
    carpeta.mkdir(parents=True, exist_ok=True)
    ruta.write_text(memoria.serializar(), encoding="utf-8", newline="\n")
    print(f"memoria del equipo: {ruta}")


def _construir(args: argparse.Namespace) -> int:
    """archive.fa de la edición definitiva a partir del traducido (nunca del japonés)."""
    from ie123kit.definitiva import construir
    from ie123kit.nucleo.contenedores.fa import FaArchive

    base, salida = Path(args.base), Path(args.salida)
    cambios, resumen = construir.capa_definitiva(FaArchive(str(base)))
    for linea in resumen:
        print(linea)
    info = construir.construir_archive(base, salida, cambios)
    print(f"{info['cambios']} archivos cambiados; {salida} ({info['tamano']} B)")
    return 0


def _paquete(args: argparse.Namespace) -> int:
    """Carpeta de usuario de Azahar con el mod y el plugin (ver paquete.py)."""
    from ie123kit.definitiva import paquete

    for linea in paquete.montar(Path(args.mod), Path(args.plugin), Path(args.salida)):
        print(linea)
    return 0


def _saltos(args: argparse.Namespace) -> int:
    """Saltos de los eventos traducidos contra el japonés (saltos.py). Código 1 si alguno va donde no debe."""
    from ie123kit.definitiva import saltos
    from ie123kit.nucleo.contenedores.fa import FaArchive

    jp, es = FaArchive(str(args.jp)), FaArchive(str(args.es))
    fallos = 0
    for guion in saltos.GUIONES:
        r = saltos.comprobar(lambda ruta: jp.read(ruta) if jp.exists(ruta) else None,
                             lambda ruta: es.read(ruta) if es.exists(ruta) else None, guion)
        if r is None:
            print(f"{guion}: no está en los dos")
            continue
        mal = sum(r.mal.values())
        print(f"{guion}: {r.traducidos} eventos traducidos, {r.revisados} saltos, {mal} mal en {len(r.mal)} eventos, "
              f"{len(r.sin_alinear)} sin alinear")
        for eid, n in r.mal.most_common(10):
            print(f"  {eid}: {n}")
        fallos += mal + len(r.sin_alinear)
    return 1 if fallos else 0


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
    t = sub.add_parser("traspasar", help="lleva los fichajes de una partida al juego siguiente")
    t.add_argument("--logica", required=True)
    t.add_argument("--fa", help="archive.fa del que leer los eventos del juego de destino (si no están en --logica)")
    t.add_argument("--origen", required=True)
    t.add_argument("--destino", required=True)
    t.add_argument("--espacio-origen", type=int, default=1)
    t.add_argument("--espacio-destino", type=int, default=1)
    t.add_argument("--modo", default=traspaso.COMPLETA, choices=traspaso.MODOS)
    t.add_argument("--nivel", type=int)
    t.add_argument("--juego-origen", help="carpeta del juego de origen si no es la de su formato (inazuma3_ogre)")
    t.add_argument("--juego-destino", help="carpeta del juego de destino si no es la de su formato (inazuma3_ogre)")
    t.add_argument("--memoria", help="carpeta Inazuma123Definitiva de la SD: ahí apunta a los que llegan con la historia")
    t.add_argument("-o", "--salida", required=True)
    t.set_defaults(func=_traspasar)
    c = sub.add_parser("construir", help="archive.fa de la edición definitiva sobre el de la traducción")
    c.add_argument("--base", required=True, help="archive.fa YA TRADUCIDO (el de la carpeta de mods)")
    c.add_argument("-o", "--salida", required=True)
    c.set_defaults(func=_construir)
    q = sub.add_parser("paquete", help="carpeta de usuario de Azahar (Windows, Android, AYN Thor) con mod y plugin")
    q.add_argument("--mod", required=True, help="carpeta del mod de la traducción (load/mods/00040000000BB800)")
    q.add_argument("--plugin", required=True, help="Definitiva-release.3gx")
    q.add_argument("-o", "--salida", required=True)
    q.set_defaults(func=_paquete)
    s = sub.add_parser("saltos", help="comprueba los saltos de los eventos traducidos contra el japonés")
    s.add_argument("--jp", required=True, help="archive.fa japonés (romfs del juego)")
    s.add_argument("--es", required=True, help="archive.fa traducido (el de la carpeta de mods)")
    s.set_defaults(func=_saltos)
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
