// Prueba en el PC de Nucleo/NombreSjis.
//     g++ -std=c++17 -Wall -D_GLIBCXX_ASSERTIONS -o /tmp/nb pruebas/nombres_pc.cpp Sources/Nucleo/NombreSjis.cpp && /tmp/nb

#include <cassert>
#include <cstdio>
#include <string>

#include "../Sources/Nucleo/NombreSjis.hpp"

using namespace Definitiva;

static std::string D(const std::string &bytes) {
    return DecodificarNombre(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size());
}

int main() {
    // «Ｓｈａｗｎ　Ｆｒｏｓｔｅ» en Shift-JIS de ancho completo.
    assert(D("\x82\x72\x82\x88\x82\x81\x82\x97\x82\x8e\x81\x40\x82\x65\x82\x92\x82\x8f\x82\x93\x82\x94\x82\x85") ==
           "Shawn Froste");
    // «CaΗΔn» con los portadores de ñ y ó.
    assert(D(std::string("Ca\x83\xa5\x83\xa2n")) == "Cañón");
    // Un kanji (glifo de dos letras del IE2) sale como «·»; el relleno de ceros corta.
    assert(D(std::string("\x82\x60\x9b\xf8\x82\x85\x82\x8e\0\0\0", 11)) == "A·en");
    assert(D("Shawn") == "Shawn");
    assert(NormalizarBusqueda("Cañón ÁNGEL") == "canon angel");
    std::puts("NombreSjis: OK");
    return 0;
}
