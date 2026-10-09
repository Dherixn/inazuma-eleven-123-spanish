"""Edición definitiva: partidas guardadas y traspaso entre juegos, con partidas sintéticas (sin ROM)."""

import struct
import zlib

import pytest

from ie123kit.definitiva import partida as pt
from ie123kit.definitiva import traspaso as tr
from ie123kit.definitiva.fichas import leer_curvas_exp


def _partida(clave, jugadores, version=None):
    formato = pt.FORMATOS[clave]
    datos = bytearray(pt.DATOS_INICIO + formato.datos_largo + 0x40)
    version = version or {"ie1.123": "INAZUMA_ELEVEN", "ie2.123": "INAZUMA_ELEVEN2",
                          "ie3.123": "INAZUMA_ELEVEN3"}[clave]
    datos[0x04:0x04 + len(version)] = version.encode()
    struct.pack_into("<I", datos, pt.OFF_MARCA, pt.MARCA)
    struct.pack_into("<I", datos, 0, zlib.crc32(datos[0x04:0x40]))
    p = pt.Partida(bytes(datos), pt.FormatoPartida(formato.nombre, formato.datos_largo, formato.jugadores_inicio,
                                                   formato.jugador_paso, verificado=True))
    for (id_, nivel, exp, tecnicas) in jugadores:
        crudo = bytearray(formato.jugador_paso)
        crudo[0x10:0x12] = b"\xAA\xBB"  # campo sin interpretar que debe sobrevivir
        tec = tuple(tecnicas) + (0,) * (pt.NUM_TECNICAS - len(tecnicas))
        p.anadir_jugador(pt.JugadorPartida(exp=exp, id=id_, tecnicas=tec, niveles_tecnica=(1,) * 6,
                                           nivel=nivel, crudo=bytes(crudo)))
    p.sellar()
    return p


def test_sellar_deja_las_sumas_correctas_y_validar_las_comprueba():
    p = _partida("ie2.123", [(1, 30, 9000, [0xA2])])
    p.validar()
    assert all(p.sumas_correctas().values())
    p.datos[pt.DATOS_INICIO] ^= 1
    with pytest.raises(ValueError, match="datos"):
        p.validar()


def test_la_suma_info_incluye_la_de_datos():
    p = _partida("ie3.123", [])
    p.datos[pt.DATOS_INICIO + 5] = 7
    struct.pack_into("<I", p.datos, pt.OFF_CRC_DATOS, zlib.crc32(p.datos[pt.DATOS_INICIO:pt.DATOS_INICIO + 0x5940]))
    assert not p.sumas_correctas()["info"]
    p.sellar()
    assert all(p.sumas_correctas().values())


def test_jugadores_ida_y_vuelta():
    p = _partida("ie2.123", [(1, 30, 9000, [0xA2, 0xB6]), (10, 25, 7000, [0x65])])
    a, b = p.jugadores()
    assert (a.id, a.nivel, a.exp, a.tecnicas[:2]) == (1, 30, 9000, (0xA2, 0xB6))
    assert pt.leer_jugador(pt.escribir_jugador(b)) == b
    assert b.crudo[0x10:0x12] == b"\xAA\xBB"


def test_formato_no_verificado_no_se_escribe():
    datos = bytes(pt.DATOS_INICIO + pt.FORMATOS["ie1.123"].datos_largo)
    with pytest.raises(ValueError, match="no está verificado"):
        pt.Partida(datos, pt.FORMATOS["ie1.123"]).sellar()


@pytest.mark.parametrize(("version", "clave"), [("INAZUMA_ELEVEN3", "ie3.123"), ("INAZUMA_ELEVEN2", "ie2.123"),
                                                ("INAZUMA_ELEVEN", "ie1.123"), ("OTRO_JUEGO", None)])
def test_detectar_formato(version, clave):
    datos = bytearray(0x40)
    datos[0x04:0x04 + len(version)] = version.encode()
    formato = pt.detectar_formato(bytes(datos))
    assert (formato.nombre if formato else None) == clave


def test_traspaso_completo_ie2_a_ie3_con_tabla_de_equivalencias():
    ie2 = _partida("ie2.123", [(1, 40, 20000, [0xA2, 0xB6]), (1513, 35, 15000, [0x1F0]), (56, 10, 500, [])])
    ie3 = _partida("ie3.123", [(1, 20, 4000, [0xA2, 0, 0x1A6])])
    informe = tr.traspasar(ie2, ie3, {1: 1, 1513: 1169}, tr.Opciones(modo=tr.COMPLETA))
    ie3.sellar()
    ie3.validar()
    assert informe.sin_pareja == [56]
    assert informe.mejorados == [(1, 1)]
    assert informe.traspasados == [(1513, 1169)]
    assert informe.con_plantilla == [1513]
    mark, shawn = ie3.jugadores()
    # Mark ya estaba: se queda con la EXP mayor y suma la Mano Mágica en un hueco libre.
    assert (mark.nivel, mark.exp) == (40, 20000)
    assert mark.tecnicas[:3] == (0xA2, 0xB6, 0x1A6)
    assert (shawn.id, shawn.nivel, shawn.tecnicas[0]) == (1169, 35, 0x1F0)
    assert len(shawn.crudo) == 0x6C


def test_traspaso_ie1_a_ie2_conserva_numeracion_y_ficha_entera():
    ie1 = _partida("ie1.123", [(11, 30, 9000, [0x92])])
    ie2 = _partida("ie2.123", [(1, 10, 500, [])])
    informe = tr.traspasar(ie1, ie2, None, tr.Opciones(modo=tr.COMPLETA))
    assert informe.traspasados == [(11, 11)] and not informe.con_plantilla
    assert ie2.jugadores()[1].crudo[0x10:0x12] == b"\xAA\xBB"


def test_modo_equilibrado_topa_nivel_y_recalcula_exp():
    curvas = [[100 * (n + 1) ** 2 for n in range(98)] for _ in range(16)]
    op = tr.Opciones(modo=tr.EQUILIBRADA, nivel_tope=25,
                     exp_de_nivel=lambda id_, nivel: tr.exp_minima(curvas, 0, nivel))
    ie2 = _partida("ie2.123", [(1, 40, 999999, []), (2, 12, 1500, [])])
    ie3 = _partida("ie3.123", [(7, 3, 300, [])])
    tr.traspasar(ie2, ie3, {1: 1, 2: 2}, op)
    _, a, b = ie3.jugadores()
    assert (a.nivel, a.exp) == (25, tr.exp_minima(curvas, 0, 25))
    assert (b.nivel, b.exp) == (12, 1500)


def test_modo_solo_fichajes_exige_nivel_y_curva():
    ie2 = _partida("ie2.123", [(1, 40, 999999, [])])
    with pytest.raises(ValueError, match="nivel_fichaje"):
        tr.traspasar(ie2, _partida("ie3.123", []), {1: 1}, tr.Opciones(modo=tr.SOLO_FICHAJES))
    with pytest.raises(ValueError, match="exp_de_nivel"):
        tr.traspasar(ie2, _partida("ie3.123", []), {1: 1}, tr.Opciones(modo=tr.SOLO_FICHAJES, nivel_fichaje=5))


def test_destino_vacio_con_otro_formato_no_inventa_campos():
    informe = tr.traspasar(_partida("ie2.123", [(1, 5, 100, [])]), _partida("ie3.123", []), {1: 1},
                           tr.Opciones(modo=tr.COMPLETA))
    assert informe.sin_plantilla == [1] and not informe.traspasados


def test_plantilla_llena_no_desborda():
    ie2 = _partida("ie2.123", [(500, 5, 100, [])])
    ie3 = _partida("ie3.123", [(i, 5, 100, []) for i in range(1, 101)])
    informe = tr.traspasar(ie2, ie3, {500: 500}, tr.Opciones(modo=tr.COMPLETA))
    assert informe.sin_sitio == [500] and ie3.num_jugadores == 100


def test_curvas_exp():
    valores = [c * 1000 + n for c in range(16) for n in range(98)]
    curvas = leer_curvas_exp(struct.pack(f"<{len(valores)}I", *valores))
    assert tr.exp_minima(curvas, 0, 1) == 0
    assert tr.exp_minima(curvas, 2, 2) == 2000
    assert tr.exp_minima(curvas, 15, 99) == 15097
    with pytest.raises(ValueError):
        leer_curvas_exp(b"\0" * 10)
