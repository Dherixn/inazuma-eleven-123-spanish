// Ejecuta en el PC el mismo traspaso que el plugin, para compararlo con la versión de Python.
//
//   traspaso_pc ORIGEN DESTINO SALIDA MODO [TOPE|FICHAJE] [MAPA]
//
// MODO: completa | equilibrada | solo_fichajes.  MAPA: lista:A=B,C=D
// Datos de destino de prueba (iguales en comparar.py): EXP = 100·nivel² + número; técnicas por nivel
// = (n%400+1, nv 1), (n%300+101, nv 10), (n%200+201, nv 30), (n%100+301, nv 100); clave = número / 3.
// Imprime el informe en una línea por apartado y escribe la partida de destino sellada.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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

static uint32_t ExpPrueba(uint16_t numero, uint8_t nivel, void *) { return 100u * nivel * nivel + numero; }

static int AprendidasPrueba(uint16_t n, uint8_t nivel, uint16_t salida[4], void *) {
    const uint16_t tec[4] = {static_cast<uint16_t>(n % 400 + 1), static_cast<uint16_t>(n % 300 + 101),
                             static_cast<uint16_t>(n % 200 + 201), static_cast<uint16_t>(n % 100 + 301)};
    const uint8_t nv[4] = {1, 10, 30, 100};
    int k = 0;
    for (int i = 0; i < 4; ++i)
        if (nv[i] < 100 && nv[i] <= nivel)
            salida[k++] = tec[i];
    return k;
}

static uint32_t ClavePrueba(uint16_t numero, void *) { return numero / 3; }

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
    std::vector<uint8_t> b = Leer(argv[2]), a = std::string(argv[1]) == "-" ? b : Leer(argv[1]);
    Partida origen, destino;
    if (!origen.Cargar(a, DetectarFormato(a.data(), a.size())) || !destino.Cargar(b, DetectarFormato(b.data(), b.size()))) {
        std::fprintf(stderr, "partida no reconocida\n");
        return 2;
    }
    Opciones op;
    op.expDeNivel = ExpPrueba;
    op.aprendidas = AprendidasPrueba;
    op.clave = ClavePrueba;
    std::string modo = argv[4];
    int nivel = argc > 5 ? std::atoi(argv[5]) : -1;
    if (modo == "completa") op.modo = Modo::Completa;
    else if (modo == "equilibrada") { op.modo = Modo::Equilibrada; op.nivelTope = nivel; }
    else if (modo == "solo_fichajes") { op.modo = Modo::SoloFichajes; op.nivelFichaje = nivel; }
    else if (modo == "fichar") {
        // traspaso_pc - DESTINO SALIDA fichar NIVEL NUMERO: fichaje especial sobre DESTINO.
        static const char *NOMBRES[] = {"hecho", "ya_esta", "sin_sitio", "sin_plantilla", "opciones"};
        Fichaje r = Fichar(destino, static_cast<uint16_t>(std::atoi(argv[6])), static_cast<uint8_t>(nivel), op);
        std::printf("fichaje %s\n", NOMBRES[static_cast<int>(r)]);
        if (destino.Sellar() != Error::Ninguno) return 1;
        FILE *f = std::fopen(argv[3], "wb");
        std::fwrite(destino.Datos().data(), 1, destino.Datos().size(), f);
        std::fclose(f);
        return 0;
    }
    else { std::fprintf(stderr, "modo desconocido\n"); return 2; }

    std::vector<Par> lista;
    Mapa mapa;
    std::string nombreMapa = argc > 6 ? argv[6] : "";
    if (nombreMapa.rfind("lista:", 0) == 0) {
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
    Lista("al_grupo", inf.alGrupo);
    FILE *f = std::fopen(argv[3], "wb");
    std::fwrite(destino.Datos().data(), 1, destino.Datos().size(), f);
    std::fclose(f);
    return 0;
}
