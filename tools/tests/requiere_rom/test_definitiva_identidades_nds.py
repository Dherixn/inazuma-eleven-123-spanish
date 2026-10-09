"""Edición definitiva: la tabla de equivalencias sobre las ROM NDS extraídas en work/."""


import pytest

from ie123kit.definitiva import fichas, identidades
from ie123kit.nucleo.config.raiz import find_root

pytestmark = pytest.mark.requiere_rom

CARPETAS = {
    "ie1": ("nds.ie1", "work/ie1/fuentes/nds_es/data_iz/logic/sp"),
    "ie2": ("nds.ie2", "work/ie2/tormenta_de_fuego/fuentes/nds_es/data_iz/logic/sp"),
    "ie3": ("nds.ie3", "work/ie3/amenaza_del_ogro/fuentes/nds_es/data_iz/logic"),
}


def _cargar(juego):
    perfil, ruta = CARPETAS[juego]
    carpeta = find_root() / ruta
    if not (carpeta / "unitbase.dat").is_file():
        pytest.skip(f"falta la extracción NDS de {juego} en {ruta}")
    p = fichas.PERFILES[perfil]
    return identidades.jugadores(
        fichas.leer_identidades((carpeta / "unitbase.dat").read_bytes(), p),
        fichas.leer_crecimiento((carpeta / "unitstat.dat").read_bytes(), p),
    )


def test_tabla_nds():
    ie1, ie2, ie3 = _cargar("ie1"), _cargar("ie2"), _cargar("ie3")
    assert (len(ie1), len(ie2), len(ie3)) == (2400, 2400, 2582)
    tabla = {e.ie2: e for e in identidades.tabla(ie1, ie2, ie3)}
    # El Raimon conserva el número en los tres juegos.
    for indice in range(1, 12):
        assert (tabla[indice].ie1, tabla[indice].ie3) == (indice, indice), tabla[indice]
    # Las tres fichas de Shawn Froste (defensa, delantero «Atsuya» y despierto) van a sus tres del IE3.
    assert [tabla[i].ie3 for i in (1505, 1513, 1514)] == [1161, 1169, 1170]
    jugables = [e for e in tabla.values() if e.jugable_ie2]
    assert sum(e.ie3 is not None for e in jugables) >= 1700


def test_estadisticas_ie3_desofuscadas_cuadran_con_ie2():
    ie2, ie3 = _cargar("ie2"), _cargar("ie3")
    # Mark Evans: misma curva de PE y PT, crecimiento distinto entre juegos.
    assert ie2[1].crecimiento.pe[2] == ie3[1].crecimiento.pe[2] == 5
    assert ie3[1].crecimiento.jugable
    assert all(0 < base <= maximo <= 99 for base, maximo, _ in ie3[1].crecimiento.atributos)
