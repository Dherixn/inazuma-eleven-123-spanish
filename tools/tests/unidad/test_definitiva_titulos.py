"""Edición definitiva: títulos de equipo del IE2 acortados a la caja (sin ROM)."""

from ie123kit.definitiva import titulos
from ie123kit.nucleo.texto.sjis_portador import es_encode


def _str(textos: dict[int, str], largo: int = 0x600) -> bytes:
    datos = bytearray(largo)
    for off, texto in textos.items():
        cod = es_encode(texto, 31)
        datos[off:off + len(cod)] = cod
    return bytes(datos)


def test_cambia_los_que_estan_y_respeta_lo_demas():
    datos = _str({0x4A0: "Equipo Victorioso", 0x0E0: "Orgullo de León", 0x460: "Equipo Raimon"})
    nuevo, saltados = titulos.corregir(datos)
    assert titulos.decodificar(nuevo[0x4A0:0x4C0]) == "Victoriosos"
    assert titulos.decodificar(nuevo[0x0E0:0x100]) == "Garra de León"
    assert nuevo[0x460:0x480] == datos[0x460:0x480]  # no está en la lista
    assert len(nuevo) == len(datos)
    # Los que no traen el texto esperado (aquí, ranuras vacías) se dejan y se avisa.
    assert len(saltados) == len(titulos.CAMBIOS) - 2


def test_no_toca_un_titulo_que_la_traduccion_haya_cambiado():
    datos = _str({0x4A0: "Equipo Ganador"})
    nuevo, saltados = titulos.corregir(datos)
    assert nuevo[0x4A0:0x4C0] == datos[0x4A0:0x4C0]
    assert any("0x4a0" in s for s in saltados)


def test_todos_los_nuevos_caben_y_se_codifican():
    for antes, despues in titulos.CAMBIOS.values():
        assert len(despues) <= titulos.CASILLAS < len(antes)
        assert titulos.decodificar(titulos.codificar(despues)) == despues
