// Prueba en el PC de Nucleo/MemoriaTecnicas.
//     g++ -std=c++17 -Wall -D_GLIBCXX_ASSERTIONS -o /tmp/mt pruebas/memoria_pc.cpp Sources/Nucleo/MemoriaTecnicas.cpp && /tmp/mt

#include <cassert>
#include <cstdio>

#include "../Sources/Nucleo/MemoriaTecnicas.hpp"

using namespace Definitiva;

int main() {
    MemoriaTecnicas m;
    const uint16_t antes[6] = {0x105, 0x66, 0x9F, 0x200, 0, 0};
    assert(m.Apuntar(2337, antes, 6) && !m.Apuntar(2337, antes, 6));
    // Vuelve tras el partido contra Corea solo con las de su tabla.
    const uint16_t despues[6] = {0x105, 0x66, 0, 0, 0, 0};
    auto perdidas = m.Perdidas(2337, despues, 6);
    assert(perdidas.size() == 2 && perdidas[0] == 0x9F && perdidas[1] == 0x200);
    assert(m.Perdidas(9999, despues, 6).empty());
    // Claves de 32 bits (CRC del nombre) y jugadores desconocidos (0x80000000 | número).
    const uint32_t grande = 0xF00DCAFEu;
    m.Apuntar(grande, antes, 6);
    // Ida y vuelta por texto.
    MemoriaTecnicas copia;
    copia.Cargar(m.Serializar());
    assert(copia.Serializar() == m.Serializar() && copia.Perdidas(2337, despues, 6).size() == 2);
    copia.Cargar("basura\n=1\n7=,,3,x\n");
    assert(copia.Serializar() == "7=3\n");
    // Como mucho 16 por jugador: se olvida la más antigua.
    MemoriaTecnicas llena;
    for (uint16_t t = 1; t <= 20; ++t)
        llena.Apuntar(5, &t, 1);
    const uint16_t ninguna[1] = {0};
    auto quedan = llena.Perdidas(5, ninguna, 1);
    assert(quedan.size() == 16 && quedan.front() == 5 && quedan.back() == 20);
    std::puts("MemoriaTecnicas: OK");
    return 0;
}
