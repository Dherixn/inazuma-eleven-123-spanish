// Prueba en el PC de Nucleo/Plantilla: con tablas sintéticas siempre y, si se le pasan, con un volcado real de
// memoria (en local; Norma 2: ni el volcado ni archive.fa se suben).
//     plantilla_pc                                    -> casos sintéticos
//     plantilla_pc archive.fa JUEGO volcado.bin BASE  -> busca la plantilla en el volcado (BASE en hexadecimal)
// JUEGO: inazuma1 | inazuma2 | inazuma3 | inazuma3_ogre.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../Sources/Nucleo/DatosDestino.hpp"
#include "../Sources/Nucleo/LectorFA.hpp"
#include "../Sources/Nucleo/Plantilla.hpp"

using namespace Definitiva;

static void P16(uint8_t *p, uint16_t v) { p[0] = v & 0xFF; p[1] = v >> 8; }
static void P32(uint8_t *p, uint32_t v) { for (int k = 0; k < 4; ++k) p[k] = (v >> (8 * k)) & 0xFF; }

// Juego sintético: 50 fichas, números 1..50, todas jugables, curva única 100*n^2.
static JuegoDatos Sintetico() {
    const PerfilJuego &p = PERFIL_IE2;
    const size_t n = 50;
    std::vector<uint8_t> base(n * p.basePaso), stat(n * p.statPaso), no(n * 2), lvup(98 * 4);
    for (size_t i = 0; i < n; ++i) {
        P16(&no[i * 2], static_cast<uint16_t>(i + 1));
        P16(&stat[i * p.statPaso + 2], 100);  // jugable
        P16(&base[i * p.basePaso + p.offCara], static_cast<uint16_t>(i + 1));
    }
    for (uint32_t v = 0; v < 98; ++v)
        P32(&lvup[v * 4], 100 * (v + 2) * (v + 2));
    JuegoDatos d;
    bool ok = d.Construir(p, base, stat, lvup, no);
    assert(ok);
    return d;
}

static void Sinteticos() {
    JuegoDatos t = Sintetico();
    const GeometriaPlantilla &g = GEO_IE2;
    std::vector<uint8_t> zona(0x4000, 0xEE);
    const size_t inicio = 0x400;
    for (uint32_t i = 0; i < 6; ++i) {
        uint8_t *r = &zona[inicio + i * g.paso];
        std::memset(r, 0, g.paso);
        uint8_t nivel = static_cast<uint8_t>(10 + i);
        P32(r, 100u * nivel * nivel + 5);
        P16(r + 4, static_cast<uint16_t>(3 + i));
        r[g.offNivel] = nivel;
    }
    std::memset(&zona[inicio + 6 * g.paso], 0, g.paso);  // fin de la plantilla
    long off = BuscarPlantilla(zona.data(), zona.size(), g, t);
    assert(off == static_cast<long>(inicio));
    assert(ContarRegistros(&zona[inicio], zona.size() - inicio, g, t) == 6);
    // Uno con EXP muy por encima de su nivel (otro truco, una prueba): la búsqueda estricta se para ahí, pero
    // con la plantilla ya encontrada se sigue contando a los de detrás.
    P32(&zona[inicio + 3 * g.paso], 900000);
    assert(ContarRegistros(&zona[inicio], zona.size() - inicio, g, t) == 3);
    assert(ContarRegistros(&zona[inicio], zona.size() - inicio, g, t, false) == 6);
    P32(&zona[inicio + 3 * g.paso], 100u * 13 * 13 + 5);
    // EXP por debajo de la de su nivel: no vale (así no se confunde con basura).
    P32(&zona[inicio + 2 * g.paso], 10);
    assert(ContarRegistros(&zona[inicio], zona.size() - inicio, g, t) == 2);
    assert(ContarRegistros(&zona[inicio], zona.size() - inicio, g, t, false) == 2);
    assert(BuscarPlantilla(zona.data(), zona.size(), g, t) == -1);
    P32(&zona[inicio + 2 * g.paso], 100u * 12 * 12 + 5);
    // IE2: detrás de la plantilla, las fichas de los rivales del partido (byte de rival a 1): no son tuyos.
    assert(g.offRival == 0x56);
    zona[inicio + 4 * g.paso + g.offRival] = 1;
    assert(ContarRegistros(&zona[inicio], zona.size() - inicio, g, t) == 4);
    assert(ContarRegistros(&zona[inicio], zona.size() - inicio, g, t, false) == 4);
    assert(BuscarPlantilla(zona.data(), zona.size(), g, t) == static_cast<long>(inicio));
    zona[inicio + 4 * g.paso + g.offRival] = 0;
    assert(ContarRegistros(&zona[inicio], zona.size() - inicio, g, t) == 6);

    uint8_t actuales[4], maximos[4];
    P16(actuales, 37); P16(actuales + 2, 41); P16(maximos, 75); P16(maximos + 2, 85);
    assert(LlenarPePt(actuales, maximos) && actuales[0] == 75 && actuales[2] == 85);
    assert(!LlenarPePt(actuales, maximos));
    P16(actuales, 90);  // nunca se baja
    assert(LlenarPePt(actuales, maximos) == false && actuales[0] == 90);

    MultiplicadorExp m;
    assert(m.Aplicar(7, 1000, 2, 999999) == 1000);   // primera vez: solo se recuerda
    assert(m.Aplicar(7, 1300, 2, 999999) == 1600);   // ganó 300: +300
    assert(m.Aplicar(7, 1600, 3, 999999) == 1600);   // sin cambios
    assert(m.Aplicar(7, 1700, 3, 1800) == 1800);     // con tope
    assert(m.Aplicar(8, 50, 1, 999999) == 50 && m.Aplicar(8, 80, 1, 999999) == 80);  // x1 no toca
    std::puts("Plantilla (sintéticos): OK");
}

static bool LeerArchivo(void *ctx, uint64_t off, uint32_t n, uint8_t *out) {
    FILE *f = static_cast<FILE *>(ctx);
    return fseeko(f, static_cast<off_t>(off), SEEK_SET) == 0 && fread(out, 1, n, f) == n;
}

int main(int argc, char **argv) {
    Sinteticos();
    if (argc < 5)
        return 0;
    FILE *fa = std::fopen(argv[1], "rb");
    LectorFA lector;
    if (!fa || !lector.Abrir(LeerArchivo, fa)) {
        std::fprintf(stderr, "no se abre %s\n", argv[1]);
        return 2;
    }
    std::string juego = argv[2];
    const PerfilJuego &perfil = juego == "inazuma1" ? PERFIL_IE1 : juego == "inazuma2" ? PERFIL_IE2 : PERFIL_IE3;
    const GeometriaPlantilla &g = juego == "inazuma1" ? GEO_IE1 : juego == "inazuma2" ? GEO_IE2 : GEO_IE3;
    std::vector<uint8_t> b, s, l, n;
    std::string ruta = juego + "/data_iz/logic/";
    JuegoDatos t;
    if (!lector.Leer(ruta + "unitbase.dat", b) || !lector.Leer(ruta + "unitstat.dat", s) || !lector.Leer(ruta + "lvup_exp.dat", l)
        || !lector.Leer(ruta + "unitno.dat", n) || !t.Construir(perfil, b, s, l, n)) {
        std::fprintf(stderr, "tablas de %s no legibles\n", juego.c_str());
        return 2;
    }
    FILE *v = std::fopen(argv[3], "rb");
    std::vector<uint8_t> volcado;
    std::fseek(v, 0, SEEK_END);
    volcado.resize(static_cast<size_t>(std::ftell(v)));
    std::fseek(v, 0, SEEK_SET);
    if (std::fread(volcado.data(), 1, volcado.size(), v) != volcado.size())
        return 2;
    unsigned long base = std::strtoul(argv[4], nullptr, 16);
    long off = BuscarPlantilla(volcado.data(), volcado.size(), g, t);
    if (off < 0) {
        std::puts("no encontrada");
        return 1;
    }
    uint32_t cuantos = ContarRegistros(&volcado[off], volcado.size() - off, g, t);
    std::printf("plantilla en %08lX, %u jugadores\n", base + off, cuantos);
    for (uint32_t i = 0; i < cuantos; ++i) {
        const uint8_t *r = &volcado[off + i * g.paso];
        const uint8_t *pp = &volcado[off + g.offPePt + i * g.pasoPePt];
        std::printf("  %2u: número %5u nivel %2u EXP %7u  PE/PT %u %u %u %u\n", i, r[4] | r[5] << 8, r[g.offNivel],
                    r[0] | r[1] << 8 | r[2] << 16 | r[3] << 24, pp[0] | pp[1] << 8, pp[2] | pp[3] << 8, pp[4] | pp[5] << 8,
                    pp[6] | pp[7] << 8);
    }
    return 0;
}
