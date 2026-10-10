// Construye en el PC las tablas que el plugin saca de archive.fa y las vuelca en texto, para compararlas
// con las de ie123kit (plugin/pruebas/comparar_tablas.py). Uso: tablas_pc archive.fa > salida.txt

#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

#include "../Sources/Nucleo/DatosDestino.hpp"
#include "../Sources/Nucleo/Eventos.hpp"
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

// Altas de la historia de un juego (el IE2 tiene los eventos de Ventisca aparte), como TablasJuego::AltasDe.
static std::vector<uint16_t> Altas(const LectorFA &fa, const char *nombre, std::initializer_list<const char *> carpetas) {
    std::vector<uint16_t> altas;
    for (const char *c : carpetas)
        AltasDeEventos(fa, c, altas);
    Normalizar(altas);
    std::printf("ALTAS %s", nombre);
    for (uint16_t n : altas)
        std::printf(" %u", n);
    std::printf("\n");
    return altas;
}

static void Pares(const char *nombre, const JuegoDatos &origen, const JuegoDatos &destino,
                  const std::vector<uint16_t> &altas) {
    for (const auto &p : PreferirAltas(EmparejarPorCara(origen, destino), destino, altas))
        std::printf("PAR %s %u %u\n", nombre, p.origen, p.destino);
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
    std::vector<uint16_t> a2 = Altas(fa, "IE2", {"inazuma2/data_iz/script", "inazuma2/data_iz_blizzard/script"});
    std::vector<uint16_t> a3 = Altas(fa, "IE3", {"inazuma3/data_iz/script"});
    std::vector<uint16_t> ao = Altas(fa, "OGRO", {"inazuma3_ogre/data_iz/script"});
    Pares("IE1_IE2", ie1, ie2, a2);
    Pares("IE2_IE3", ie2, ie3, a3);
    Pares("IE2_OGRO", ie2, ogro, ao);
    Pares("IE3_OGRO", ie3, ogro, ao);
    Volcar("IE2", ie2);
    Volcar("IE3", ie3);
    Volcar("OGRO", ogro);
    return 0;
}
