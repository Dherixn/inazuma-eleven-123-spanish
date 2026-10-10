// Prueba en el PC del grupo de Nucleo/Partida (los mismos casos que test_definitiva_traspaso.py).
//     g++ -std=c++17 -Wall -Wextra -D_GLIBCXX_ASSERTIONS -o /tmp/gr pruebas/grupo_pc.cpp Sources/Nucleo/Partida.cpp && /tmp/gr

#include <cassert>
#include <cstdio>
#include <vector>

#include "../Sources/Nucleo/Partida.hpp"

using namespace Definitiva;

static void P16(std::vector<uint8_t> &v, uint32_t o, uint16_t x) { v[o] = x & 0xFF; v[o + 1] = x >> 8; }

// Partida con esos números de jugador; `grupo` en la lista (con dorsales 1, 2...) y el resumen.
static Partida Crear(const FormatoPartida &f, const std::vector<uint16_t> &numeros, const std::vector<uint16_t> &grupo,
                     uint8_t resumen) {
    std::vector<uint8_t> d(DATOS_INICIO + f.datosLargo + 0x40, 0);
    d[OFF_NUM_JUGADORES] = static_cast<uint8_t>(numeros.size());
    d[OFF_NUM_GRUPO] = resumen;
    for (size_t k = 0; k < numeros.size(); ++k)
        P16(d, f.jugadoresInicio + static_cast<uint32_t>(k) * f.jugadorPaso + 4, numeros[k]);
    for (size_t k = 0; k < grupo.size(); ++k) {
        P16(d, f.offGrupo + 2 * static_cast<uint32_t>(k), grupo[k]);
        d[f.jugadoresInicio + static_cast<uint32_t>(k) * f.jugadorPaso + OFF_DORSAL] = static_cast<uint8_t>(k + 1);
    }
    Partida p;
    bool ok = p.Cargar(d, &f);
    assert(ok);
    return p;
}

static uint8_t Byte(const Partida &p, uint32_t indice, uint32_t campo) {
    return p.Datos()[p.Formato()->jugadoresInicio + indice * p.Formato()->jugadorPaso + campo];
}

int main() {
    // Sacar del grupo: los de detrás suben, dorsal y sitio a 0, resumen al día.
    Partida p = Crear(FORMATO_IE2, {1, 2, 3, 4}, {1, 2, 3, 4}, 4);
    assert(p.SacarDelGrupo(1));
    assert((p.Grupo() == std::vector<uint16_t>{1, 3, 4}));
    assert(Byte(p, 1, OFF_DORSAL) == 0 && Byte(p, 1, OFF_SITIO) == 0 && p.Datos()[OFF_NUM_GRUPO] == 3);
    assert(!p.SacarDelGrupo(1));

    // IE3 tras el prólogo: lista vacía y resumen 1 (Mark); meter y sacar sin perder a nadie.
    Partida q = Crear(FORMATO_IE3, {1, 30}, {}, 1);
    assert((q.Grupo() == std::vector<uint16_t>{1}));
    assert(q.MeterEnGrupo(1) && (q.Grupo() == std::vector<uint16_t>{1, 30}));
    assert(Byte(q, 1, OFF_SITIO) == 0);  // el IE3 no da sitio al entrar
    assert(q.SacarDelGrupo(0) && (q.Grupo() == std::vector<uint16_t>{30}));

    // Sin grupo localizado (IE1): nada.
    Partida r = Crear(FORMATO_IE1, {1}, {}, 1);
    assert(r.Grupo().empty() && !r.SacarDelGrupo(0) && !r.MeterEnGrupo(0));
    std::puts("Grupo: OK");
    return 0;
}
