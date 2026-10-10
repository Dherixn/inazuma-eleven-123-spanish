#include "Eventos.hpp"

#include <algorithm>
#include <cstring>

namespace Definitiva {

    namespace {
        uint16_t U16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }
        uint32_t U32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }

        constexpr uint32_t PKH_TABLA = 0x30, PKH_ENTRADA = 12, SIN_EVENTO = 0xFFFFFFFF;
        constexpr uint32_t SSD_CABECERA = 32, SSD_CUENTA = 12, VENTANA = 0x40000;
    }

    bool DescomprimirLz10(const uint8_t *datos, size_t largo, std::vector<uint8_t> &salida) {
        salida.clear();
        if (!largo || datos[0] != 0x10) {
            salida.assign(datos, datos + largo);
            return true;
        }
        if (largo < 4)
            return false;
        size_t tam = datos[1] | datos[2] << 8 | static_cast<size_t>(datos[3]) << 16;
        salida.reserve(tam);
        size_t p = 4;
        while (salida.size() < tam && p < largo) {
            uint8_t banderas = datos[p++];
            for (int bit = 0; bit < 8 && salida.size() < tam && p < largo; ++bit) {
                if (banderas & (0x80 >> bit)) {
                    if (p + 1 >= largo)
                        return false;
                    uint8_t b1 = datos[p], b2 = datos[p + 1];
                    p += 2;
                    size_t cuantos = static_cast<size_t>(b1 >> 4) + 3;
                    size_t atras = (static_cast<size_t>(b1 & 0xF) << 8 | b2) + 1;
                    if (atras > salida.size())
                        return false;
                    for (size_t k = 0; k < cuantos; ++k)
                        salida.push_back(salida[salida.size() - atras]);
                } else {
                    salida.push_back(datos[p++]);
                }
            }
        }
        return true;
    }

    void AltasDeScript(const uint8_t *ssd, size_t largo, std::vector<uint16_t> &altas) {
        if (largo < SSD_CABECERA)
            return;
        uint16_t cuenta = U16(ssd + SSD_CUENTA);
        size_t pos = SSD_CABECERA;
        for (uint16_t r = 0; r < cuenta && pos + 8 <= largo; ++r) {
            uint16_t medida = U16(ssd + pos + 2), op = U16(ssd + pos + 4);
            uint8_t argc = ssd[pos + 6];
            size_t tipos = 4 * ((static_cast<size_t>(argc) + 7) / 8);
            if (!medida)
                break;
            if (op == OP_ALTA && argc && pos + 8 + tipos + 4 <= largo)
                altas.push_back(static_cast<uint16_t>(U32(ssd + pos + 8 + tipos)));
            pos += medida;
        }
    }

    bool AltasDeEventos(const LectorFA &fa, const std::string &carpeta, std::vector<uint16_t> &altas) {
        std::vector<uint8_t> pkh;
        uint64_t basePkb;
        uint32_t largoPkb;
        if (!fa.Leer(carpeta + "/eve.pkh", pkh) || !fa.Localizar(carpeta + "/eve.pkb", &basePkb, &largoPkb)
            || pkh.size() < PKH_TABLA || std::memcmp(pkh.data(), "PackNum", 7) != 0)
            return false;
        struct Entrada {
            uint32_t off, medida;
        };
        std::vector<Entrada> entradas;
        for (size_t o = PKH_TABLA; o + PKH_ENTRADA <= pkh.size(); o += PKH_ENTRADA) {
            uint32_t id = U32(&pkh[o]), off = U32(&pkh[o + 4]), medida = U32(&pkh[o + 8]);
            if (id != SIN_EVENTO && medida && static_cast<uint64_t>(off) + medida <= largoPkb)
                entradas.push_back({off, medida});
        }
        std::sort(entradas.begin(), entradas.end(), [](const Entrada &a, const Entrada &b) { return a.off < b.off; });
        std::vector<uint8_t> ventana, script;
        uint32_t inicioVentana = 0;
        for (const Entrada &e : entradas) {
            if (e.off < inicioVentana || e.off + e.medida > inicioVentana + ventana.size()) {
                inicioVentana = e.off;
                uint32_t cuanto = std::min<uint32_t>(std::max(VENTANA, e.medida), largoPkb - e.off);
                ventana.resize(cuanto);
                if (!fa.LeerTramo(basePkb + e.off, cuanto, ventana.data()))
                    return false;
            }
            if (DescomprimirLz10(&ventana[e.off - inicioVentana], e.medida, script))
                AltasDeScript(script.data(), script.size(), altas);
        }
        Normalizar(altas);
        return true;
    }

    void Normalizar(std::vector<uint16_t> &numeros) {
        std::sort(numeros.begin(), numeros.end());
        numeros.erase(std::unique(numeros.begin(), numeros.end()), numeros.end());
    }

}
