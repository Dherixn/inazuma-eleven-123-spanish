"""Compara el traspaso del plugin (C++) con el de ie123kit (Python) sobre partidas sintéticas.

    python plugin/pruebas/comparar.py [--casos N]

Genera partidas de IE1, IE2 e IE3 con jugadores al azar, ejecuta los dos traspasos con las mismas
opciones y exige el mismo informe y los mismos bytes. El C++ se compila y se ejecuta en la imagen
Docker del plugin (definitiva-3gx; ver plugin/compilar.ps1). Termina con código 1 si algo difiere.
"""
from __future__ import annotations

import argparse
import random
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from dataclasses import replace
from pathlib import Path

AQUI = Path(__file__).resolve().parent
PLUGIN = AQUI.parent
sys.path.insert(0, str(PLUGIN.parent / "tools" / "src"))

from ie123kit.definitiva import partida as pt
from ie123kit.definitiva import traspaso as tr

IMAGEN = "definitiva-3gx"
MODOS = (tr.COMPLETA, tr.EQUILIBRADA, tr.SOLO_FICHAJES)


def mapa_sintetico(rnd: random.Random, cantidad: int, tope: int) -> dict[int, int]:
    """Tabla de números de jugador de prueba (las reales salen del juego al ejecutarse y no se suben)."""
    return {a: rnd.randrange(1, tope) for a in sorted(rnd.sample(range(1, tope), cantidad))}


def lista(mapa: dict[int, int]) -> str:
    return "lista:" + ",".join(f"{a}={b}" for a, b in mapa.items())


def crear_partida(clave: str, jugadores: list[tuple[int, int, int, list[int]]], rnd: random.Random) -> pt.Partida:
    formato = pt.FORMATOS[clave]
    datos = bytearray(rnd.randbytes(pt.DATOS_INICIO + formato.datos_largo + 0x40))
    version = {"ie1.123": "INAZUMA_ELEVEN", "ie2.123": "INAZUMA_ELEVEN2", "ie3.123": "INAZUMA_ELEVEN3"}[clave]
    datos[0x04:0x14] = version.encode().ljust(16, b"\0")
    struct.pack_into("<I", datos, pt.OFF_MARCA, pt.MARCA)
    struct.pack_into("<I", datos, 0, zlib.crc32(datos[0x04:0x40]))
    datos[pt.OFF_NUM_JUGADORES] = 0
    p = pt.Partida(bytes(datos), replace(formato, verificado=True))
    for id_, nivel, exp, tecnicas in jugadores:
        tec = (tecnicas + [0] * pt.NUM_TECNICAS)[: pt.NUM_TECNICAS]
        niveles = tuple(rnd.randrange(1, 6) for _ in range(pt.NUM_TECNICAS))
        p.anadir_jugador(pt.JugadorPartida(exp=exp, id=id_, tecnicas=tuple(tec), niveles_tecnica=niveles,
                                           nivel=nivel, crudo=rnd.randbytes(formato.jugador_paso)))
    if formato.off_grupo:
        # Grupo de verdad (los primeros, con huecos a 0), a veces vacío y a veces lleno.
        numeros = [j[0] for j in jugadores]
        cuantos = min(len(numeros), rnd.choice([0, 4, 12, pt.MAX_GRUPO, rnd.randrange(0, pt.MAX_GRUPO + 1)]))
        lista_grupo = (numeros[:cuantos] + [0] * pt.MAX_GRUPO)[: pt.MAX_GRUPO]
        for k, n in enumerate(lista_grupo):
            struct.pack_into("<H", p.datos, p.base + formato.off_grupo + 2 * k, n)
    p.sellar()
    return p


def jugadores_al_azar(rnd: random.Random, ids: list[int], n: int) -> list[tuple[int, int, int, list[int]]]:
    salida = []
    for id_ in rnd.sample(ids, min(n, len(ids))):
        nivel = rnd.randrange(1, 100)
        tecnicas = [rnd.choice([0, 0x92, 0xA2, 0xB6, 0x65, 0x1A6, 0x1F0, rnd.randrange(1, 512)]) for _ in range(6)]
        salida.append((id_, nivel, rnd.randrange(0, 500000), tecnicas))
    return salida


def casos(n: int, rnd: random.Random):
    ie1_ie2, ie2_ie3 = mapa_sintetico(rnd, 400, 2400), mapa_sintetico(rnd, 600, 3200)
    ids_ie2 = sorted(ie2_ie3)
    for k in range(n):
        modo = MODOS[k % 3]
        nivel = rnd.randrange(5, 60)
        if k % 5 == 4:
            # IE1 -> IE2: mismo tamaño de ficha, sin plantilla.
            origen = crear_partida("ie1.123", jugadores_al_azar(rnd, sorted(ie1_ie2), rnd.randrange(1, 33)), rnd)
            destino = crear_partida("ie2.123", jugadores_al_azar(rnd, sorted(ie1_ie2.values()), rnd.randrange(0, 30)), rnd)
            yield f"c{k}", origen, destino, modo, nivel, lista(ie1_ie2), ie1_ie2
            continue
        comunes = jugadores_al_azar(rnd, ids_ie2, rnd.randrange(1, 70))
        if k % 7 == 3:
            comunes += [(rnd.randrange(3000, 4000), 10, 999, [])]  # sin pareja
        destino_ids = [ie2_ie3[j[0]] for j in comunes[: rnd.randrange(0, 20)] if j[0] in ie2_ie3]
        destino_n = 0 if k % 11 == 6 else rnd.randrange(1, 100 if k % 9 == 8 else 40)
        destino = crear_partida("ie3.123", [(d, rnd.randrange(1, 99), rnd.randrange(0, 400000), []) for d in destino_ids]
                                + jugadores_al_azar(rnd, sorted(set(ie2_ie3.values()) - set(destino_ids)),
                                                    max(0, destino_n - len(destino_ids))), rnd)
        origen = crear_partida("ie2.123", comunes, rnd)
        yield f"c{k}", origen, destino, modo, nivel, lista(ie2_ie3), ie2_ie3


def exp_prueba(numero: int, nivel: int) -> int:
    return 100 * nivel * nivel + numero


def aprendidas_prueba(n: int, nivel: int) -> list[int]:
    aprende = [(n % 400 + 1, 1), (n % 300 + 101, 10), (n % 200 + 201, 30), (n % 100 + 301, 100)]
    return [t for t, lv in aprende if lv < 100 and lv <= nivel]


def informe_python(inf: tr.Informe) -> list[str]:
    def pares(nombre, v): return " ".join([nombre] + [f"{a}>{b}" for a, b in v])
    def ids(nombre, v): return " ".join([nombre] + [str(x) for x in v])
    return [pares("traspasados", inf.traspasados), pares("mejorados", inf.mejorados),
            ids("sin_pareja", inf.sin_pareja), ids("sin_sitio", inf.sin_sitio),
            ids("sin_plantilla", inf.sin_plantilla), ids("con_plantilla", inf.con_plantilla),
            ids("al_grupo", inf.al_grupo)]


def main() -> int:
    args = argparse.ArgumentParser()
    args.add_argument("--casos", type=int, default=60)
    args.add_argument("--semilla", type=int, default=1234)
    a = args.parse_args()
    rnd = random.Random(a.semilla)
    tmp = Path(tempfile.mkdtemp(prefix="comparar_", dir=AQUI))
    try:
        esperado = {}
        ordenes = ["set -e", "g++ -std=c++17 -O1 -Wall -Wextra -D_GLIBCXX_ASSERTIONS -o /tmp/tp pruebas/traspaso_pc.cpp Sources/Nucleo/*.cpp"]
        for nombre, origen, destino, modo, nivel, mapa, tabla in casos(a.casos, rnd):
            (tmp / f"{nombre}.o").write_bytes(origen.sellar())
            (tmp / f"{nombre}.d").write_bytes(destino.sellar())
            op = tr.Opciones(modo=modo, nivel_tope=nivel if modo == tr.EQUILIBRADA else None,
                             nivel_fichaje=nivel if modo == tr.SOLO_FICHAJES else None,
                             exp_de_nivel=exp_prueba, aprendidas=aprendidas_prueba, clave_destino=lambda n: n // 3)
            # Fichaje especial sobre la misma partida de destino: a veces de alguien que ya está.
            ids_destino = [j.id for j in destino.jugadores()]
            numero = rnd.choice(ids_destino) if ids_destino and rnd.random() < 0.3 else rnd.randint(1, 3000)
            nivel_f = rnd.randint(1, 99)
            fichado = pt.Partida(destino.sellar(), destino.formato, destino.espacio)
            try:
                tr.fichar(fichado, numero, nivel_f, op)
                motivo = "hecho"
            except tr.FichajeError as e:
                motivo = e.motivo
            esperado[nombre + "_f"] = ([f"fichaje {motivo}"], fichado.sellar())
            relf = f"pruebas/{tmp.name}/{nombre}_f"
            ordenes.append(f"/tmp/tp - pruebas/{tmp.name}/{nombre}.d {relf}.s fichar {nivel_f} {numero} > {relf}.txt")

            inf = tr.traspasar(origen, destino, tabla, op)
            esperado[nombre] = (informe_python(inf), destino.sellar())
            rel = f"pruebas/{tmp.name}/{nombre}"
            ordenes.append(f"/tmp/tp {rel}.o {rel}.d {rel}.s {modo} {nivel} {mapa} > {rel}.txt")
        (tmp / "correr.sh").write_text("\n".join(ordenes) + "\n", encoding="utf-8", newline="\n")
        r = subprocess.run(["docker", "run", "--rm", "-v", f"{PLUGIN}:/plugin", IMAGEN, "bash",
                            f"pruebas/{tmp.name}/correr.sh"], capture_output=True, text=True, check=False)
        if r.returncode:
            print(r.stdout, r.stderr)
            return 1
        fallos = 0
        cobertura = {}
        for lineas, _ in esperado.values():
            for linea in lineas:
                apartado, *valores = linea.split()
                cobertura[apartado] = cobertura.get(apartado, 0) + (1 if valores else 0)
        print("casos con cada apartado no vacío:", cobertura)
        for nombre, (lineas, datos) in esperado.items():
            salida = (tmp / f"{nombre}.txt").read_text(encoding="utf-8").splitlines()
            if [s.strip() for s in salida] != lineas:
                fallos += 1
                print(f"{nombre}: informe distinto\n  py: {lineas}\n  c++: {salida}")
            elif (tmp / f"{nombre}.s").read_bytes() != datos:
                fallos += 1
                cpp = (tmp / f"{nombre}.s").read_bytes()
                primero = next(i for i, (a, b) in enumerate(zip(cpp, datos, strict=False)) if a != b and i >= 0xC0)
                print(f"{nombre}: bytes distintos desde 0x{primero:X} (c++ {cpp[primero:primero + 8].hex()} "
                      f"py {datos[primero:primero + 8].hex()})")
        print(f"{len(esperado)} casos, {fallos} diferencias")
        return 1 if fallos else 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
