// Prueba en el PC de Nucleo/Texto (ajuste de líneas por palabras).
//     g++ -std=c++17 -Wall -D_GLIBCXX_ASSERTIONS -o /tmp/tx pruebas/texto_pc.cpp Sources/Nucleo/Texto.cpp && /tmp/tx

#include <cassert>
#include <cstdio>

#include "../Sources/Nucleo/Texto.hpp"

using namespace Definitiva;

int main() {
    assert(Ajustar("uno dos tres", 7) == "uno dos\ntres");
    assert(Ajustar("uno dos tres", 100) == "uno dos tres");
    assert(Ajustar("ñandú ñandú", 5) == "ñandú\nñandú");  // cuenta caracteres, no bytes
    assert(Ajustar("palabralarguísima corta", 5) == "palabralarguísima\ncorta");
    assert(Ajustar("a b\n\nc d", 3) == "a b\n\nc d");
    assert(Ajustar("", 10).empty());
    std::puts("Texto: OK");
    return 0;
}
