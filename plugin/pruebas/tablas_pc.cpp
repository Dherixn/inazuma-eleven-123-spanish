// Construye en el PC las tablas que el plugin saca de archive.fa y las vuelca en texto, para compararlas
// con las de ie123kit (plugin/pruebas/comparar_tablas.py). Uso: tablas_pc archive.fa > salida.txt

#include <cstdio>
#include <string>
#include <vector>

#include "../Sources/Nucleo/DatosDestino.hpp"
#include "../Sources/Nucleo/LectorFA.hpp"

using namespace Definitiva;

static bool LeerArchivo(void *ctx, uint64_t off, uint32_t n, uint8_t *out) {
    FILE *f = static_cast<FILE *>(ctx);
    return fseeko(f, static_cast<off_t>(off), SEEK_SET) == 0 && fread(out, 1, n, f) == n;
}

static bool Cargar(const LectorFA &fa, const char *juego, const PerfilJuego &perfil, JuegoDatos &datos) {
    std::vector<uint8_t> b, s, l, n;
    std::string base = std::string(juego) + "/data_iz/logic/";
    return fa.Leer(base + "unitbase.dat", b) && fa.Leer(base + "unitstat.dat", s) && fa.Leer(base + "lvup_exp.dat", l)
           && fa.Leer(base + "unitno.dat", n) && datos.Construir(perfil, b, s, l, n);
}

static void Volcar(const char *nombre, const JuegoDatos &d) {
    std::vector<uint16_t> vistos;
    for (size_t i = 0; i < d.Fichas(); ++i) {
        uint16_t num = d.Numero(i);
        if (d.Ficha(num) != static_cast<int>(i))
            continue;
        uint16_t tec[4] = {};
        int k = d.Aprendidas(num, 99, tec);
        std::printf("FICHA %s %u %u %u %08X %d", nombre, num, static_cast<unsigned>(i), d.ExpMinima(num, 50),
                    d.Clave(num), k);
        for (int j = 0; j < k; ++j)
            std::printf(" %u", tec[j]);
        std::printf("\n");
    }
}

int main(int argc, char **argv) {
    FILE *f = argc > 1 ? std::fopen(argv[1], "rb") : nullptr;
    LectorFA fa;
    if (!f || !fa.Abrir(LeerArchivo, f)) {
        std::fprintf(stderr, "no puedo abrir archive.fa\n");
        return 2;
    }
    JuegoDatos ie1, ie2, ie3, ogro;
    if (!Cargar(fa, "inazuma1", PERFIL_IE1, ie1) || !Cargar(fa, "inazuma2", PERFIL_IE2, ie2)
        || !Cargar(fa, "inazuma3", PERFIL_IE3, ie3) || !Cargar(fa, "inazuma3_ogre", PERFIL_IE3, ogro)) {
        std::fprintf(stderr, "faltan tablas\n");
        return 2;
    }
    for (const auto &p : EmparejarPorCara(ie1, ie2))
        std::printf("PAR IE1_IE2 %u %u\n", p.origen, p.destino);
    for (const auto &p : EmparejarPorCara(ie2, ie3))
        std::printf("PAR IE2_IE3 %u %u\n", p.origen, p.destino);
    for (const auto &p : EmparejarPorCara(ie2, ogro))
        std::printf("PAR IE2_OGRO %u %u\n", p.origen, p.destino);
    Volcar("IE2", ie2);
    Volcar("IE3", ie3);
    Volcar("OGRO", ogro);
    return 0;
}
