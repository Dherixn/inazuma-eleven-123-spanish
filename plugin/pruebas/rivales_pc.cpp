// Prueba en el PC de Nucleo/Rivales (los mismos casos que tools/tests/unidad/test_definitiva_rivales.py).
//     g++ -std=c++17 -Wall -Wextra -D_GLIBCXX_ASSERTIONS -o /tmp/rv pruebas/rivales_pc.cpp Sources/Nucleo/Rivales.cpp && /tmp/rv

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../Sources/Nucleo/Rivales.hpp"

using namespace Definitiva;

static void Partido(std::vector<uint8_t> &t, uint16_t equipo, uint8_t nivel, uint8_t conNivel = 1) {
    uint8_t r[48] = {};
    r[0] = equipo & 0xFF;
    r[1] = equipo >> 8;
    r[2] = conNivel;
    r[3] = nivel;
    r[4] = 0x2d;
    r[40] = equipo & 0xFF;
    t.insert(t.end(), r, r + 48);
}

static std::vector<uint8_t> Tabla() {
    std::vector<uint8_t> t;
    Partido(t, 0, 0);
    Partido(t, 31, 5);
    Partido(t, 70, 1);
    Partido(t, 2, 20);
    Partido(t, 10, 80);
    Partido(t, 42, 254);
    Partido(t, 5, 9, 0);
    Partido(t, 628, 70);
    return t;
}

static std::vector<uint8_t> Niveles(const std::vector<uint8_t> &t) {
    std::vector<uint8_t> n;
    for (size_t i = 0; i < t.size(); i += 48)
        n.push_back(t[i + 3]);
    return n;
}

int main() {
    const uint8_t esperado[5] = {20, 14, 25, 30, 20};
    for (int m = 0; m < 5; ++m)
        assert(NivelRival(20, m, 0, 0) == esperado[m]);
    assert(NivelRival(1, RIVALES_FACILES, 0, 0) == 1);
    assert(NivelRival(1, RIVALES_DUROS, 0, 0) == 4 && NivelRival(1, RIVALES_MUY_DUROS, 0, 0) == 7);
    assert(NivelRival(5, RIVALES_A_TU_NIVEL, 33, 0) == 33 && NivelRival(50, RIVALES_A_TU_NIVEL, 33, 0) == 50);
    assert(NivelRival(80, RIVALES_MUY_DUROS, 0, 0) == 99);
    assert(NivelRival(70, RIVALES_MUY_DUROS, 0, 30) == 70);
    assert(NivelRival(60, RIVALES_MUY_DUROS, 0, 30) == 69);
    assert(NivelRival(40, RIVALES_A_TU_NIVEL, 90, 16) == 83);

    std::vector<uint8_t> t = Tabla();
    std::vector<uint8_t> extra(1024, 0);
    extra[628] = 30;
    extra[2] = 5;
    std::vector<uint8_t> duros = t;
    AjustarPartidos(t.data(), duros.data(), static_cast<uint32_t>(t.size()), RIVALES_DUROS, 0, extra.data(), 1024);
    assert((Niveles(duros) == std::vector<uint8_t>{0, 8, 4, 25, 99, 254, 9, 70}));
    assert(MismaTablaDePartidos(duros.data(), t.data(), static_cast<uint32_t>(t.size())));
    for (size_t i = 0; i < t.size(); ++i)
        assert(i % 48 == 3 || duros[i] == t[i]);
    std::vector<uint8_t> igual = duros;
    assert(AjustarPartidos(t.data(), igual.data(), static_cast<uint32_t>(t.size()), RIVALES_ORIGINAL, 0, nullptr, 0) == 0);
    assert(igual == t);
    std::vector<uint8_t> nivel = t;
    assert(AjustarPartidos(t.data(), nivel.data(), static_cast<uint32_t>(t.size()), RIVALES_A_TU_NIVEL, 33, extra.data(),
                           1024) == 3);
    assert((Niveles(nivel) == std::vector<uint8_t>{0, 33, 33, 33, 80, 254, 9, 70}));

    std::vector<uint8_t> otra = t;
    otra[48] = 99;
    assert(!MismaTablaDePartidos(otra.data(), t.data(), static_cast<uint32_t>(t.size())));

    std::vector<uint8_t> pkb(352 * 2, 0);
    pkb[32] = 26;
    const uint16_t jugadores[3] = {2101, 2102, 0};
    const uint8_t ajustes[3] = {8, 6, 40};
    for (int k = 0; k < 3; ++k) {
        uint8_t *h = &pkb[64 + 8 * k];
        h[0] = jugadores[k] & 0xFF;
        h[1] = jugadores[k] >> 8;
        h[4] = ajustes[k];
        h[5] = 1;
    }
    pkb[352 + 32] = 16;
    pkb[352 + 64] = 1;
    uint8_t salida[64];
    ExtraDeEquipos(pkb.data(), static_cast<uint32_t>(pkb.size()), 352, salida, 64);
    assert(salida[26] == 8 && salida[16] == 0 && salida[0] == 0);

    std::puts("rivales: todo bien");
    return 0;
}
