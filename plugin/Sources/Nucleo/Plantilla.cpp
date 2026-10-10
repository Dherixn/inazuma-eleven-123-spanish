#include "Plantilla.hpp"

namespace Definitiva {

    const GeometriaPlantilla GEO_IE1 = {0x188, 0x3E, 32, 0xAC, 0x188};
    const GeometriaPlantilla GEO_IE2 = {0xFC, 0x4A, 100, 0x70, 0xFC};
    const GeometriaPlantilla GEO_IE3 = {0x6C, 0x4A, 100, 0xCA6C, 0x98};

    namespace {
        constexpr uint32_t EXP_IMPOSIBLE = 50000000;
        constexpr uint8_t MARGEN_NIVELES = 2;

        uint16_t U16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }
        uint32_t U32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }
        void P16(uint8_t *p, uint16_t v) {
            p[0] = static_cast<uint8_t>(v);
            p[1] = static_cast<uint8_t>(v >> 8);
        }
    }

    uint32_t TopeExp(const JuegoDatos &t, uint16_t numero, uint8_t nivel) {
        if (nivel >= 99)
            return t.ExpMinima(numero, 99) + t.ExpMinima(numero, 99) / 2 + 1;
        uint8_t mas = nivel + MARGEN_NIVELES > 99 ? 99 : static_cast<uint8_t>(nivel + MARGEN_NIVELES);
        return t.ExpMinima(numero, mas);
    }

    bool RegistroValido(const uint8_t *reg, const GeometriaPlantilla &g, const JuegoDatos &t) {
        uint8_t nivel = reg[g.offNivel];
        if (nivel < 1 || nivel > 99)
            return false;
        uint16_t numero = U16(reg + 4);
        int ficha = numero ? t.Ficha(numero) : -1;
        if (ficha < 0 || !t.Jugable(static_cast<size_t>(ficha)))
            return false;
        uint32_t exp = U32(reg);
        return exp >= t.ExpMinima(numero, nivel) && exp < TopeExp(t, numero, nivel) && exp < EXP_IMPOSIBLE;
    }

    uint32_t ContarRegistros(const uint8_t *inicio, size_t disponible, const GeometriaPlantilla &g, const JuegoDatos &t) {
        uint32_t n = 0;
        uint16_t vistos[128];
        while (n < g.maxJugadores && n < 128 && (n + 1) * static_cast<size_t>(g.paso) <= disponible
               && RegistroValido(inicio + n * g.paso, g, t)) {
            uint16_t numero = U16(inicio + n * g.paso + 4);
            for (uint32_t k = 0; k < n; ++k)
                if (vistos[k] == numero)
                    return n;  // nadie está dos veces en la plantilla
            vistos[n++] = numero;
        }
        return n;
    }

    long BuscarPlantilla(const uint8_t *zona, size_t largo, const GeometriaPlantilla &g, const JuegoDatos &t,
                         uint32_t minimo) {
        size_t necesita = static_cast<size_t>(g.paso) * minimo;
        for (size_t off = 0; off + necesita <= largo; off += 4) {
            const uint8_t *p = zona + off;
            uint8_t nivel = p[g.offNivel];
            if (nivel < 1 || nivel > 99 || !(p[4] | p[5]) || p[3] > 2)  // filtro rápido: EXP < 50 millones
                continue;
            if (ContarRegistros(p, largo - off, g, t) < minimo)
                continue;
            if (off >= g.paso && RegistroValido(p - g.paso, g, t))
                continue;
            return static_cast<long>(off);
        }
        return -1;
    }

    bool LlenarPePt(uint8_t *actuales, const uint8_t *maximos) {
        bool cambio = false;
        for (int k = 0; k < 2; ++k) {
            uint16_t actual = U16(actuales + 2 * k), maximo = U16(maximos + 2 * k);
            if (actual < maximo) {
                P16(actuales + 2 * k, maximo);
                cambio = true;
            }
        }
        return cambio;
    }

    uint32_t MultiplicadorExp::Aplicar(uint16_t numero, uint32_t exp, int factor, uint32_t tope) {
        auto it = ultima_.find(numero);
        uint32_t final = exp;
        if (it != ultima_.end() && exp > it->second && factor > 1) {
            uint64_t extra = static_cast<uint64_t>(exp - it->second) * static_cast<uint64_t>(factor - 1);
            uint64_t suma = exp + extra;
            final = static_cast<uint32_t>(suma > tope ? (exp > tope ? exp : tope) : suma);
        }
        ultima_[numero] = final;
        return final;
    }

}
