"""Edición definitiva: partidas guardadas y traspaso entre juegos, con partidas sintéticas (sin ROM)."""

import struct
import zlib
from dataclasses import replace

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
    p = pt.Partida(bytes(datos), replace(formato, verificado=True))
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


def test_traspaso_ie1_a_ie2_conserva_numeracion_y_usa_plantilla_del_ie2():
    # El IE1 de la recopilación tiene fichas de 0x60 B y el IE2 de 0x70: los campos sin interpretar
    # salen de una ficha del IE2.
    ie1 = _partida("ie1.123", [(11, 30, 9000, [0x92])])
    ie2 = _partida("ie2.123", [(1, 10, 500, [])])
    informe = tr.traspasar(ie1, ie2, None, tr.Opciones(modo=tr.COMPLETA))
    assert informe.traspasados == [(11, 11)] and informe.con_plantilla == [11]
    assert len(ie2.jugadores()[1].crudo) == 0x70


def test_espacios_de_la_recopilacion():
    formato = pt.FORMATOS["ie1.123"]
    datos = bytearray(formato.archivo_largo)
    datos[0x04:0x12] = b"INAZUMA_ELEVEN"
    struct.pack_into("<I", datos, formato.espacio_largo * 2 + pt.OFF_MARCA, pt.MARCA)
    p = pt.Partida(bytes(datos), formato, espacio=2)
    assert p.ocupado and not pt.Partida(bytes(datos), formato, espacio=0).ocupado
    assert p.sumas_correctas()["info"] is False
    with pytest.raises(ValueError):
        pt.Partida(bytes(datos), formato, espacio=3)
    with pytest.raises(ValueError):
        pt.Partida(bytes(pt.FORMATOS["ie2.123"].archivo_largo), pt.FORMATOS["ie2.123"], espacio=1)


def test_modo_equilibrado_topa_nivel_y_recalcula_exp():
    curvas = [[100 * (n + 1) ** 2 for n in range(98)] for _ in range(16)]
    op = tr.Opciones(modo=tr.EQUILIBRADA, nivel_tope=25,
                     exp_de_nivel=lambda id_, nivel: tr.exp_minima(curvas, 0, nivel))
    ie2 = _partida("ie2.123", [(1, 40, 999999, []), (2, 12, 1500, [])])
    ie3 = _partida("ie3.123", [(7, 3, 300, [])])
    tr.traspasar(ie2, ie3, {1: 1, 2: 2}, op)
    _, a, b = ie3.jugadores()
    assert (a.nivel, a.exp) == (25, tr.exp_minima(curvas, 0, 25))
    # La EXP se recalcula con la curva del juego de destino aunque el nivel no cambie.
    assert (b.nivel, b.exp) == (12, tr.exp_minima(curvas, 0, 12))


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


def test_opciones_incompletas_fallan_antes_de_tocar_la_partida():
    ie2 = _partida("ie2.123", [(1, 10, 500, []), (2, 50, 90000, [])])
    ie3 = _partida("ie3.123", [(9, 3, 300, [])])
    antes = bytes(ie3.datos)
    with pytest.raises(ValueError, match="exp_de_nivel"):
        tr.traspasar(ie2, ie3, {1: 1, 2: 2}, tr.Opciones(modo=tr.EQUILIBRADA, nivel_tope=20))
    assert bytes(ie3.datos) == antes
    with pytest.raises(ValueError, match="modo desconocido"):
        tr.traspasar(ie2, ie3, {1: 1}, tr.Opciones(modo="otro"))


def test_tecnicas_aprendidas_en_el_juego_nuevo_mas_las_que_traia():
    aprende = {5: [(0x10, 1), (0x20, 15), (0x30, 40), (0x40, 100)], 1: [(0xA2, 1), (0xB6, 30)]}

    def aprendidas(id_, nivel):
        return [t for t, lv in aprende.get(id_, []) if lv < 100 and lv <= nivel]

    op = tr.Opciones(modo=tr.COMPLETA, exp_de_nivel=lambda id_, n: n * n, aprendidas=aprendidas)
    ie1 = _partida("ie1.123", [(5, 20, 0, [0x92]), (1, 35, 0, [])])
    ie2 = _partida("ie2.123", [(1, 10, 100, [0xA2])])
    tr.traspasar(ie1, ie2, {5: 5, 1: 1}, op)
    mark, nuevo = ie2.jugadores()
    # Mark ya estaba: sube a 35 y aprende la Mano Mágica del IE2 (nivel 30).
    assert (mark.nivel, mark.exp, mark.tecnicas[:2]) == (35, 35 * 35, (0xA2, 0xB6))
    # El nuevo llega con lo que sabe a nivel 20 en el IE2 y con la técnica que traía (el IE1 no tiene
    # niveles de técnica: todas empiezan en el primero).
    assert nuevo.tecnicas[:3] == (0x10, 0x20, 0x92) and nuevo.niveles_tecnica[:3] == (0, 0, 0)


def test_ficha_del_ie1_tecnicas_de_un_byte_y_nivel_en_0x3e():
    formato = pt.FORMATOS["ie1.123"]
    reg = bytearray(formato.jugador_paso)
    reg[0x04] = 7
    reg[0x38:0x3B] = bytes([0x92, 0xA2, 0x05])
    reg[0x3E] = 23
    j = pt.leer_jugador(bytes(reg), formato)
    assert (j.id, j.nivel, j.tecnicas[:3], j.niveles_tecnica) == (7, 23, (0x92, 0xA2, 5), (0,) * 6)
    assert pt.escribir_jugador(j, formato) == bytes(reg)


def test_fusion_por_personaje_no_duplica_a_quien_tiene_varias_fichas():
    personaje = {10: "Axel Blaze", 1097: "Axel Blaze", 11: "Kevin Dragonfly"}
    op = tr.Opciones(modo=tr.COMPLETA, exp_de_nivel=lambda id_, n: 10 * n, clave_destino=personaje.get)
    ie1 = _partida("ie1.123", [(10, 30, 0, []), (11, 20, 0, [])])
    ie2 = _partida("ie2.123", [(1097, 5, 50, [0x65])])
    informe = tr.traspasar(ie1, ie2, {10: 10, 11: 11}, op)
    assert informe.mejorados == [(10, 10)] and informe.traspasados == [(11, 11)]
    axel, kevin = ie2.jugadores()
    # Axel sigue siendo la ficha 1097 del IE2, ahora a nivel 30.
    assert (axel.id, axel.nivel, axel.exp) == (1097, 30, 300) and kevin.id == 11


def _op_fichaje(**cambios):
    return tr.Opciones(exp_de_nivel=lambda n, nv: 100 * nv * nv, aprendidas=lambda n, nv: [0x105, 0x66][: nv // 12 + 1],
                       **cambios)


def test_fichar_anade_con_nivel_exp_y_tecnicas_y_plantilla_de_la_partida():
    p = _partida("ie2.123", [(2337, 40, 12345, [0x105, 0x114])])
    shawn = p.jugadores()[0]
    k = tr.fichar(p, 1307, 40, _op_fichaje(), plantilla=shawn.crudo)
    assert k == 1 and p.num_jugadores == 2
    aiden = p.jugadores()[1]
    assert (aiden.id, aiden.nivel, aiden.exp) == (1307, 40, 160000)
    assert aiden.tecnicas[:2] == (0x105, 0x66) and aiden.tecnicas[2:] == (0,) * 4
    assert aiden.crudo[0x10:0x12] == b"\xAA\xBB"  # los campos sin interpretar, de la plantilla
    p.sellar()
    p.validar()


def test_fichar_no_duplica_ni_pasa_del_maximo():
    p = _partida("ie2.123", [(2337, 40, 0, []), (1307, 20, 0, [])])
    with pytest.raises(tr.FichajeError, match="ya_esta"):
        tr.fichar(p, 1307, 40, _op_fichaje())
    with pytest.raises(tr.FichajeError, match="ya_esta"):  # la clave manda (variantes de un mismo personaje)
        tr.fichar(p, 999, 40, _op_fichaje(clave_destino=lambda n: "aiden" if n in (999, 1307) else n))
    lleno = _partida("ie1.123", [(i, 1, 0, []) for i in range(1, 33)])
    with pytest.raises(tr.FichajeError, match="sin_sitio"):
        tr.fichar(lleno, 1307, 10, _op_fichaje())
    with pytest.raises(tr.FichajeError, match="opciones"):
        tr.fichar(p, 5, 10, tr.Opciones())


def _con_grupo(p, numeros, dorsales):
    """Pone la lista del grupo y los dorsales como los deja el juego (IE2)."""
    for k, n in enumerate((list(numeros) + [0] * pt.MAX_GRUPO)[:pt.MAX_GRUPO]):
        struct.pack_into("<H", p.datos, p.base + p.formato.off_grupo + 2 * k, n)
    for i, d in enumerate(dorsales):
        p.datos[p._off_jugador(i) + pt.OFF_DORSAL] = d
    p.datos[p.base + pt.OFF_NUM_GRUPO] = len(numeros)


def test_fichar_normaliza_la_ficha_y_entra_en_el_grupo_con_sitio():
    # Como la partida de pruebas del IE2: 12 en el grupo con dorsales 1..12 y el capitán con sus órdenes.
    p = _partida("ie2.123", [(n, 10, 5000, []) for n in range(1, 13)])
    _con_grupo(p, range(1, 13), range(1, 13))
    capitan = p._off_jugador(0)
    p.datos[capitan + 0x58], p.datos[capitan + 0x5B], p.datos[capitan + 0x5D] = 1, 0x1F, 1
    op = tr.Opciones(modo=tr.SOLO_FICHAJES, nivel_fichaje=35, exp_de_nivel=lambda n, nv: 100 * nv)
    i = tr.fichar(p, 1307, 35, op)
    reg = p.datos[p._off_jugador(i):p._off_jugador(i) + p.formato.jugador_paso]
    assert p.grupo()[:13] == list(range(1, 13)) + [1307]
    assert (reg[pt.OFF_DORSAL], reg[pt.OFF_SITIO], reg[0x5B], reg[0x5D]) == (13, 13, 0x10, 0)
    assert p.datos[p.base + pt.OFF_NUM_GRUPO] == 13
    assert reg[0x10:0x12] == b"\xAA\xBB"  # lo demás del molde sigue igual


def test_con_el_grupo_lleno_el_fichado_va_a_la_reserva():
    p = _partida("ie2.123", [(n, 10, 5000, []) for n in range(1, 17)])
    _con_grupo(p, range(1, 17), range(1, 17))
    op = tr.Opciones(modo=tr.SOLO_FICHAJES, nivel_fichaje=20, exp_de_nivel=lambda n, nv: 100 * nv)
    i = tr.fichar(p, 1307, 20, op)
    reg = p.datos[p._off_jugador(i):p._off_jugador(i) + p.formato.jugador_paso]
    assert 1307 not in p.grupo() and (reg[pt.OFF_DORSAL], reg[pt.OFF_SITIO]) == (0, 0)


def test_el_dorsal_es_el_libre_mas_bajo():
    p = _partida("ie2.123", [(1, 10, 5000, []), (2, 10, 5000, []), (3, 10, 5000, [])])
    _con_grupo(p, [1, 2, 3], [1, 3, 4])
    op = tr.Opciones(modo=tr.SOLO_FICHAJES, nivel_fichaje=20, exp_de_nivel=lambda n, nv: 100 * nv)
    i = tr.fichar(p, 50, 20, op)
    assert p.datos[p._off_jugador(i) + pt.OFF_DORSAL] == 2


def test_traspaso_al_ie3_quita_al_fichado_el_equipo_y_las_ordenes_del_capitan():
    # El IE3 empieza con Paolo Bianchi (Orfeo): sus campos de equipo no deben copiarse a los que llegan.
    ie2 = _partida("ie2.123", [(1, 40, 20000, [0xA2])])
    ie3 = _partida("ie3.123", [(3170, 30, 2379, [])])
    paolo = ie3._off_jugador(0)
    for off, v in ((0x4B, 0x36), (0x57, 10), (0x58, 1), (0x5A, 1), (0x5B, 0x11)):
        ie3.datos[paolo + off] = v
    ie3.datos[ie3.base + pt.OFF_NUM_GRUPO] = 1  # Paolo en el grupo con la lista vacía, como tras el prólogo
    informe = tr.traspasar(ie2, ie3, {1: 1}, tr.Opciones(modo=tr.COMPLETA))
    reg = ie3.datos[ie3._off_jugador(1):ie3._off_jugador(1) + ie3.formato.jugador_paso]
    # Sin el equipo ni las órdenes de Paolo; entra en el grupo detrás de él, con dorsal y sin sitio (el IE3
    # lo deja a 0 al entrar).
    assert [reg[o] for o in (0x4B, 0x57, 0x58, 0x5A, 0x5B)] == [1, 1, 0, 0, 0x10]
    assert ie3.grupo()[:3] == [3170, 1, 0] and ie3.datos[ie3.base + pt.OFF_NUM_GRUPO] == 2
    assert informe.al_grupo == [1]


def test_sacar_del_grupo_sube_a_los_de_detras_y_limpia_dorsal_y_sitio():
    p = _partida("ie2.123", [(n, 10, 5000, []) for n in (1, 2, 3, 4)])
    _con_grupo(p, [1, 2, 3, 4], [1, 2, 3, 4])
    assert p.sacar_del_grupo(1)  # el número 2
    reg = p.datos[p._off_jugador(1):p._off_jugador(1) + p.formato.jugador_paso]
    assert p.grupo()[:4] == [1, 3, 4, 0] and (reg[pt.OFF_DORSAL], reg[pt.OFF_SITIO]) == (0, 0)
    assert p.datos[p.base + pt.OFF_NUM_GRUPO] == 3
    assert not p.sacar_del_grupo(1)  # ya está en la reserva


def test_grupo_efectivo_con_la_lista_vacia_son_los_primeros_registros():
    # Como el IE3 tras el prólogo: lista vacía y resumen 1 (Mark).
    p = _partida("ie3.123", [(1, 1, 0, []), (30, 18, 2000, [])])
    p.datos[p.base + pt.OFF_NUM_GRUPO] = 1
    assert p.grupo_efectivo() == [1]
    assert p.meter_en_grupo(1) and p.grupo_efectivo() == [1, 30]
    assert p.sacar_del_grupo(0) and p.grupo_efectivo() == [30]
