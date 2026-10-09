// Ejecuta en el PC el mismo traspaso que el plugin, para compararlo con la versión de Python.
//
//   traspaso_pc ORIGEN DESTINO SALIDA MODO [TOPE|FICHAJE] [MAPA]
//
// MODO: completa | equilibrada | solo_fichajes.  MAPA: ie1-ie2 | ie2-ie3 | lista:A=B,C=D
// La EXP de un nivel, en las pruebas, es 100 * nivel² (sin curvas reales).
// Imprime el informe en una línea por apartado y escribe la partida de destino sellada.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../Sources/Nucleo/Equivalencias.hpp"
#include "../Sources/Nucleo/Partida.hpp"
#include "../Sources/Nucleo/Traspaso.hpp"

using namespace Definitiva;

static std::vector<uint8_t> Leer(const char *ruta) {
    std::vector<uint8_t> datos;
    FILE *f = std::fopen(ruta, "rb");
    if (!f) { std::perror(ruta); std::exit(2); }
    int c;
    while ((c = std::fgetc(f)) != EOF) datos.push_back(static_cast<uint8_t>(c));
    std::fclose(f);
    return datos;
}

static uint32_t ExpCuadratica(uint16_t, uint8_t nivel, void *) { return 100u * nivel * nivel; }

static void Lista(const char *nombre, const std::vector<Par> &v) {
    std::printf("%s", nombre);
    for (const Par &p : v) std::printf(" %u>%u", p.origen, p.destino);
    std::printf("\n");
}

static void Lista(const char *nombre, const std::vector<uint16_t> &v) {
    std::printf("%s", nombre);
    for (uint16_t x : v) std::printf(" %u", x);
    std::printf("\n");
}

int main(int argc, char **argv) {
    if (argc < 5) {
        std::fprintf(stderr, "uso: traspaso_pc ORIGEN DESTINO SALIDA MODO [NIVEL] [MAPA]\n");
        return 2;
    }
    std::vector<uint8_t> a = Leer(argv[1]), b = Leer(argv[2]);
    Partida origen, destino;
    if (!origen.Cargar(a, DetectarFormato(a.data(), a.size())) || !destino.Cargar(b, DetectarFormato(b.data(), b.size()))) {
        std::fprintf(stderr, "partida no reconocida\n");
        return 2;
    }
    Opciones op;
    op.expDeNivel = ExpCuadratica;
    std::string modo = argv[4];
    int nivel = argc > 5 ? std::atoi(argv[5]) : -1;
    if (modo == "completa") op.modo = Modo::Completa;
    else if (modo == "equilibrada") { op.modo = Modo::Equilibrada; op.nivelTope = nivel; }
    else if (modo == "solo_fichajes") { op.modo = Modo::SoloFichajes; op.nivelFichaje = nivel; }
    else { std::fprintf(stderr, "modo desconocido\n"); return 2; }

    std::vector<Par> lista;
    Mapa mapa;
    std::string nombreMapa = argc > 6 ? argv[6] : "ie2-ie3";
    if (nombreMapa == "ie1-ie2") mapa = MAPA_IE1_IE2;
    else if (nombreMapa == "ie2-ie3") mapa = MAPA_IE2_IE3;
    else if (nombreMapa.rfind("lista:", 0) == 0) {
        std::string resto = nombreMapa.substr(6);
        size_t pos = 0;
        while (pos < resto.size()) {
            size_t coma = resto.find(',', pos);
            std::string par = resto.substr(pos, coma == std::string::npos ? std::string::npos : coma - pos);
            size_t igual = par.find('=');
            lista.push_back({static_cast<uint16_t>(std::atoi(par.substr(0, igual).c_str())),
                             static_cast<uint16_t>(std::atoi(par.substr(igual + 1).c_str()))});
            if (coma == std::string::npos) break;
            pos = coma + 1;
        }
        std::sort(lista.begin(), lista.end(), [](const Par &x, const Par &y) { return x.origen < y.origen; });
        mapa = {lista.data(), lista.size()};
    }

    Informe inf = Traspasar(origen, destino, mapa, op);
    if (inf.error != Error::Ninguno) {
        std::printf("error %s\n", DescribirError(inf.error));
        return 1;
    }
    Error e = destino.Sellar();
    if (e != Error::Ninguno) {
        std::printf("error %s\n", DescribirError(e));
        return 1;
    }
    Lista("traspasados", inf.traspasados);
    Lista("mejorados", inf.mejorados);
    Lista("sin_pareja", inf.sinPareja);
    Lista("sin_sitio", inf.sinSitio);
    Lista("sin_plantilla", inf.sinPlantilla);
    Lista("con_plantilla", inf.conPlantilla);
    FILE *f = std::fopen(argv[3], "wb");
    std::fwrite(destino.Datos().data(), 1, destino.Datos().size(), f);
    std::fclose(f);
    return 0;
}
