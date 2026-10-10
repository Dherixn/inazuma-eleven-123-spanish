// Prueba en el PC de Nucleo/Eventos (LZ10 y altas de un SSD), también con datos rotos: un evento malformado
// no debe colgar el plugin. Con datos reales lo compara pruebas/comparar_tablas.py.
//     g++ -std=c++17 -Wall -Wextra -D_GLIBCXX_ASSERTIONS -o /tmp/ev pruebas/eventos_pc.cpp Sources/Nucleo/Eventos.cpp Sources/Nucleo/LectorFA.cpp && /tmp/ev

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../Sources/Nucleo/Eventos.hpp"

using namespace Definitiva;

static void P16(std::vector<uint8_t> &v, uint16_t x) { v.push_back(x & 0xFF); v.push_back(x >> 8); }
static void P32(std::vector<uint8_t> &v, uint32_t x) { for (int k = 0; k < 4; ++k) v.push_back((x >> (8 * k)) & 0xFF); }

static std::vector<uint8_t> Ssd(const std::vector<std::pair<uint16_t, std::vector<uint32_t>>> &ins) {
    std::vector<uint8_t> v(32, 0);
    std::memcpy(v.data(), "SSD", 4);
    v[12] = static_cast<uint8_t>(ins.size());
    uint16_t id = 0;
    for (const auto &i : ins) {
        size_t tipos = 4 * ((i.second.size() + 7) / 8);
        P16(v, id++);
        P16(v, static_cast<uint16_t>(8 + tipos + 4 * i.second.size()));
        P16(v, i.first);
        v.push_back(static_cast<uint8_t>(i.second.size()));
        v.push_back(0);
        v.insert(v.end(), tipos, 0);
        for (uint32_t a : i.second)
            P32(v, a);
    }
    return v;
}

int main() {
    // LZ10: «abcabcabc» = 3 literales y una copia de 6 hacia atrás 3.
    const uint8_t lz[] = {0x10, 9, 0, 0, 0x10, 'a', 'b', 'c', 0x30, 0x02};
    std::vector<uint8_t> s;
    assert(DescomprimirLz10(lz, sizeof(lz), s) && std::string(s.begin(), s.end()) == "abcabcabc");
    // Sin cabecera 0x10: tal cual.
    const uint8_t crudo[] = {1, 2, 3};
    assert(DescomprimirLz10(crudo, 3, s) && s.size() == 3 && s[2] == 3);
    // Rotos: copia antes del principio y flujo cortado a mitad de una copia.
    const uint8_t atras[] = {0x10, 9, 0, 0, 0x80, 0x30, 0x05};
    assert(!DescomprimirLz10(atras, sizeof(atras), s));
    const uint8_t cortado[] = {0x10, 9, 0, 0, 0x10, 'a', 'b', 'c', 0x30};
    assert(!DescomprimirLz10(cortado, sizeof(cortado), s));

    // Altas: solo el primer argumento de OP_ALTA.
    std::vector<uint8_t> ssd = Ssd({{12399, {2338, 1, 1, 1}}, {OP_ALTA, {3170, 30, 10, 0, 1}}, {16447, {3170}},
                                    {OP_ALTA, {1, 25, 1, 0, 1}}});
    std::vector<uint16_t> altas;
    AltasDeScript(ssd.data(), ssd.size(), altas);
    assert(altas.size() == 2 && altas[0] == 3170 && altas[1] == 1);
    // Cortado por cualquier sitio: nunca lee fuera.
    for (size_t largo = 0; largo < ssd.size(); ++largo) {
        std::vector<uint8_t> trozo(ssd.begin(), ssd.begin() + static_cast<long>(largo));
        std::vector<uint16_t> a;
        AltasDeScript(trozo.data(), trozo.size(), a);
        assert(a.size() <= 2);
    }
    // Registro de medida 0: se para en vez de dar vueltas.
    std::vector<uint8_t> cero = Ssd({{OP_ALTA, {7}}});
    cero[34] = cero[35] = 0;
    std::vector<uint16_t> a0;
    AltasDeScript(cero.data(), cero.size(), a0);
    assert(a0.empty());
    Normalizar(altas = {30, 1, 30, 2338});
    assert((altas == std::vector<uint16_t>{1, 30, 2338}));
    std::puts("Eventos: OK");
    return 0;
}
