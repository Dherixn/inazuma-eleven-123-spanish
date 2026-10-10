"""Edición definitiva: la capa de datos sobre un archive.fa sintético (Norma 2: ni un byte de ROM)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "contrato"))
from fa_sintetico import escribir_fa

from ie123kit.definitiva import construir
from ie123kit.nucleo.contenedores.fa import FaArchive

FICHEROS = {
    "inazuma2/data_iz/INAZUMA.INI": b"RPG_ENCOUNT_TIME=60\r\n",
    "inazuma2/data_iz/logic/unitbase.dat": bytes(range(96)) * 3,
    "inazuma3/data_iz/INAZUMA.INI": b"RPG_ENCOUNT_TIME=60\r\n",
}


def test_cambia_solo_lo_pedido(tmp_path):
    origen = escribir_fa(tmp_path / "base.fa", FICHEROS)
    antes = origen.read_bytes()
    nuevo = b"RPG_ENCOUNT_TIME=150\r\n" + b"x" * 500
    info = construir.construir_archive(origen, tmp_path / "mod/archive.fa", {"inazuma2/data_iz/INAZUMA.INI": nuevo})
    assert origen.read_bytes() == antes
    arc = FaArchive(str(tmp_path / "mod/archive.fa"))
    assert arc.read("inazuma2/data_iz/INAZUMA.INI") == nuevo
    for ruta in ("inazuma2/data_iz/logic/unitbase.dat", "inazuma3/data_iz/INAZUMA.INI"):
        assert arc.read(ruta) == FICHEROS[ruta]
    assert info == {"tamano": (tmp_path / "mod/archive.fa").stat().st_size, "cambios": 1}


def test_rechaza_rutas_que_no_existen_y_pisar_el_origen(tmp_path):
    origen = escribir_fa(tmp_path / "base.fa", FICHEROS)
    with pytest.raises(KeyError, match="no_existe"):
        construir.construir_archive(origen, tmp_path / "otro.fa", {"inazuma2/no_existe.dat": b""})
    assert not (tmp_path / "otro.fa").exists()
    with pytest.raises(ValueError):
        construir.construir_archive(origen, origen, {})
