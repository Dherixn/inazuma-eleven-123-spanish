"""Edición definitiva: edición de INAZUMA.INI conservando comentarios, Shift-JIS y CRLF (sin ROM)."""

import pytest

from ie123kit.definitiva import ini

COMENTARIO = "; ＲＰＧでのエンカウント発生判定間隔".encode("shift_jis")
INI = b"\r\n".join([
    b"; INAZUMA.INI",
    COMENTARIO,
    b"RPG_ENCOUNT_TIME=60",
    b"; RPG_MOVEACCEL_DOWN=10\tcomentada",
    b"RPG_MOVESPEED=320",
    b"FUSEI_ITEM_NO = 0",
    b"CPU_USE_SPECIAL_PERCENT=40",
    b"SPECIAL_SKILL_SPEND_TP=1",
    b"IZ_TYPE=0",
    b"",
])


def test_leer_ignora_comentarios():
    valores = ini.leer(INI)
    assert valores["RPG_ENCOUNT_TIME"] == "60"
    assert valores["FUSEI_ITEM_NO"] == "0"
    assert "RPG_MOVEACCEL_DOWN" not in valores


def test_cambiar_solo_toca_los_valores():
    nuevo = ini.cambiar(INI, {"RPG_ENCOUNT_TIME": 240, "FUSEI_ITEM_NO": 5})
    assert ini.leer(nuevo)["RPG_ENCOUNT_TIME"] == "240"
    assert b"FUSEI_ITEM_NO = 5" in nuevo
    assert COMENTARIO in nuevo and nuevo.endswith(b"\r\n")
    assert nuevo.replace(b"240", b"60").replace(b"= 5", b"= 0") == INI


def test_clave_que_no_existe_es_un_error():
    with pytest.raises(KeyError, match="NO_EXISTE"):
        ini.cambiar(INI, {"NO_EXISTE": 1})


def test_aplicar_ayudas():
    nuevo = ini.aplicar_ayudas(INI, {"encuentros": "pocos", "pt": "infinitos", "cpu": "dificil"})
    valores = ini.leer(nuevo)
    assert (valores["RPG_ENCOUNT_TIME"], valores["SPECIAL_SKILL_SPEND_TP"], valores["CPU_USE_SPECIAL_PERCENT"]) == (
        "240", "0", "60")
    assert ini.aplicar_ayudas(INI, {"encuentros": "normal", "velocidad": "normal"}) == INI
    with pytest.raises(KeyError):
        ini.aplicar_ayudas(INI, {"encuentros": "ninguno"})
