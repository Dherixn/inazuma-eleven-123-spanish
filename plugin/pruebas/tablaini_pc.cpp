// Prueba en el PC de Nucleo/TablaIni con una tabla sintética como la que deja el juego en memoria.
//     g++ -std=c++17 -Wall -D_GLIBCXX_ASSERTIONS -o /tmp/ti pruebas/tablaini_pc.cpp Sources/Nucleo/TablaIni.cpp && /tmp/ti

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../Sources/Nucleo/TablaIni.hpp"

using namespace Definitiva;

static void Entrada(std::vector<uint8_t> &z, size_t off, const char *clave, const char *valor) {
    std::memcpy(&z[off], clave, std::strlen(clave));
    std::memcpy(&z[off + INI_TEXTO], valor, std::strlen(valor));
}

int main() {
    std::vector<uint8_t> zona(0x4000, 0);
    const size_t T = 0x1F40 & ~size_t(INI_ENTRADA - 1);
    Entrada(zona, T, "FARCLIP_LENGTH", "1024");
    Entrada(zona, T + 0x40, "MDL_SCALE_CHAR", "819");
    Entrada(zona, T + 0x80, "RPG_ENCOUNT_TIME", "60");
    Entrada(zona, T + 0xC0, "OFFSIDE_EFFECTIVE", "1");
    zona[T + 0x100] = 0xE5;  // basura detrás de la tabla
    // Copia desalineada (como la del analizador del juego): no cuenta.
    Entrada(zona, 0x204, "FARCLIP_LENGTH", "1024");
    Entrada(zona, 0x244, "MDL_SCALE_CHAR", "819");

    uint32_t halladas[4];
    assert(BuscarTablas(zona.data(), zona.size(), halladas, 4) == 1 && halladas[0] == T);
    uint8_t *tabla = &zona[T];
    uint32_t n = ContarEntradas(tabla, 100);
    assert(n == 4 && EsTablaIni(tabla, n));
    assert(BuscarClave(tabla, n, "RPG_ENCOUNT") == -1 && BuscarClave(tabla, n, "RPG_ENCOUNT_TIME") == 2);

    uint8_t *e = tabla + 2 * INI_ENTRADA;
    assert(LeerValor(e) == 60);
    assert(PonerValor(e, 99999) && LeerValor(e) == 99999 && !PonerValor(e, 99999));
    assert(PonerValor(e, 25) && LeerValor(e) == 25 && e[INI_TEXTO + 2] == 0 && e[INI_TEXTO + 4] == 0);
    uint8_t *f = tabla + 3 * INI_ENTRADA;
    assert(PonerValor(f, 0) && LeerValor(f) == 0 && f[INI_TEXTO] == '0' && f[INI_TEXTO + 1] == 0);
    assert(zona[T + 0x100] == 0xE5 && ContarEntradas(tabla, 3) == 3);
    std::puts("TablaIni: OK");
    return 0;
}
