"""Edición definitiva: memoria del equipo (los mismos casos que plugin/pruebas/memoria_pc.cpp)."""

from ie123kit.definitiva.memoria_equipo import MemoriaEquipo, Recuerdo, restaurar
from ie123kit.definitiva.partida import JugadorPartida

TEXTO = ("#partida=BF10331F\n#tiempo=17823\n"
         "1=162:3,512;n=1;nv=60;xp=50000;p=1\n"
         "2337=261:2,102,159:1;n=2337;nv=45;xp=18192\n")


def _memoria() -> MemoriaEquipo:
    m = MemoriaEquipo(partida=0xBF10331F, tiempo=17823)
    assert m.apuntar(2337, 2337, 45, 18192, [0x105, 0x66, 0x9F, 0, 0, 0], [2, 0, 1, 0, 0, 0])
    assert not m.apuntar(2337, 2337, 45, 18192, [0x105, 0x66, 0x9F, 0, 0, 0], [2, 0, 1, 0, 0, 0])
    m.esperar(1, Recuerdo(numero=1, nivel=60, exp=50000, tecnicas=[(0xA2, 3), (0x200, 0)]))
    return m


def test_texto_ida_y_vuelta():
    assert _memoria().serializar() == TEXTO
    assert MemoriaEquipo.cargar(TEXTO).serializar() == TEXTO


def test_cargar_se_salta_lo_que_no_entiende():
    texto = "basura\n=1\n7=,,3,x:2,4:9\n#tiempo=abc\n8=;n=5;nv=120;xp=-3;p=2\n12=1,2\n"
    assert MemoriaEquipo.cargar(texto).serializar() == ("#partida=00000000\n#tiempo=0\n"
                                                        "7=3,4:9\n8=;n=5;nv=0;xp=0\n12=1,2\n")


def test_apuntar_se_queda_el_nivel_mas_alto_y_quita_el_pendiente():
    m = _memoria()
    # La historia lo vuelve a crear a nivel 30: no se olvida del 45.
    assert not m.apuntar(2337, 2337, 30, 9000, [0x105, 0, 0, 0, 0, 0], [0] * 6)
    assert (m.recuerdos[2337].nivel, m.recuerdos[2337].exp) == (45, 18192)
    assert m.apuntar(1, 1, 25, 7000, [0x10, 0, 0, 0, 0, 0], None)
    r = m.recuerdos[1]
    assert not r.pendiente and (r.nivel, r.exp) == (60, 50000) and r.tecnicas[-1] == (0x10, 0)
    assert m.perdidas(1, [0x10, 0xA2, 0, 0, 0, 0]) == [0x200]


def test_como_mucho_16_tecnicas_se_olvida_la_mas_antigua():
    m = MemoriaEquipo()
    for t in range(1, 21):
        m.apuntar(5, 5, 1, 0, [t])
    assert [t for t, _ in m.recuerdos[5].tecnicas] == list(range(5, 21))
    grande = 0xF00DCAFE  # claves de 32 bits (CRC del nombre)
    m.apuntar(grande, 9, 1, 0, [7])
    assert MemoriaEquipo.cargar(m.serializar()).recuerdos[grande].tecnicas == [(7, 0)]


def _jugador(nivel, exp, tecnicas, niveles=(0,) * 6, numero=2338):
    return JugadorPartida(exp=exp, id=numero, tecnicas=tuple(tecnicas), niveles_tecnica=tuple(niveles), nivel=nivel,
                          crudo=b"")


def test_restaurar_a_quien_la_historia_vuelve_a_crear():
    r = Recuerdo(numero=2338, nivel=48, exp=21000, tecnicas=[(0x105, 2), (0x9F, 1), (0x200, 0)])
    j = restaurar(_jugador(40, 900, (0x105, 0, 0, 0, 0, 0)), r)
    assert (j.nivel, j.exp) == (48, 21000)
    assert j.tecnicas == (0x105, 0x9F, 0x200, 0, 0, 0) and j.niveles_tecnica == (2, 1, 0, 0, 0, 0)
    assert restaurar(j, r) is None  # ya está como lo recuerda
    # Huecos llenos: no se pisa ninguna.
    lleno = _jugador(40, 900, (1, 2, 3, 4, 5, 6))
    assert restaurar(lleno, r).tecnicas == (1, 2, 3, 4, 5, 6)


def test_restaurar_al_pendiente_aunque_la_historia_lo_traiga_con_mas_nivel():
    r = Recuerdo(numero=1, nivel=20, exp=100, pendiente=True, tecnicas=[(0xA2, 1)])
    j = _jugador(25, 5000, (0x10, 0, 0, 0, 0, 0), numero=1)
    assert restaurar(j, r) == j.con(tecnicas=(0x10, 0xA2, 0, 0, 0, 0), niveles_tecnica=(0, 1, 0, 0, 0, 0))
    r.pendiente = False
    assert restaurar(j, r) is None
