#include "Rivales.hpp"

#include <cstring>

namespace Definitiva {

    namespace {

        int Mayor(int a, int b) { return a > b ? a : b; }

        uint16_t U16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }

    }

    uint8_t NivelRival(uint8_t original, int modo, int nivelEquipo, uint8_t extra) {
        int n = original;
        switch (modo) {
        case RIVALES_FACILES:
            return static_cast<uint8_t>(Mayor(1, (original * 7 + 5) / 10));  // un 30 % menos
        case RIVALES_DUROS:
            n = Mayor(original + 3, (original * 5 + 2) / 4);  // un 25 % más; 3 niveles como poco
            break;
        case RIVALES_MUY_DUROS:
            n = Mayor(original + 6, (original * 3 + 1) / 2);  // un 50 % más; 6 niveles como poco
            break;
        case RIVALES_A_TU_NIVEL:
            n = Mayor(original, nivelEquipo);
            break;
        default:
            return original;
        }
        int tope = Mayor(original, NIVEL_MAXIMO - extra);
        return static_cast<uint8_t>(n > tope ? tope : n);
    }

    bool PartidoConNivel(const uint8_t *r) {
        return U16(r + OFF_PARTIDO_EQUIPO) != 0 && r[OFF_PARTIDO_CON_NIVEL] == 1 && r[OFF_PARTIDO_NIVEL] >= 1
               && r[OFF_PARTIDO_NIVEL] <= NIVEL_MAXIMO;
    }

    uint32_t AjustarPartidos(const uint8_t *original, uint8_t *destino, uint32_t largo, int modo, int nivelEquipo,
                             const uint8_t *extraPorEquipo, uint32_t equipos) {
        uint32_t cambiados = 0;
        for (uint32_t off = 0; off + PARTIDO_PASO <= largo; off += PARTIDO_PASO) {
            const uint8_t *r = original + off;
            uint8_t nivel = r[OFF_PARTIDO_NIVEL];
            if (PartidoConNivel(r)) {
                uint16_t equipo = U16(r + OFF_PARTIDO_EQUIPO);
                uint8_t extra = extraPorEquipo && equipo < equipos ? extraPorEquipo[equipo] : 0;
                nivel = NivelRival(nivel, modo, nivelEquipo, extra);
                cambiados += nivel != r[OFF_PARTIDO_NIVEL];
            }
            destino[off + OFF_PARTIDO_NIVEL] = nivel;
        }
        return cambiados;
    }

    bool MismaTablaDePartidos(const uint8_t *memoria, const uint8_t *original, uint32_t largo) {
        for (uint32_t i = 0; i < largo; ++i)
            if (i % PARTIDO_PASO != OFF_PARTIDO_NIVEL && memoria[i] != original[i])
                return false;
        return true;
    }

    void ExtraDeEquipos(const uint8_t *pkb, uint32_t largo, uint32_t paso, uint8_t *salida, uint32_t equipos) {
        std::memset(salida, 0, equipos);
        for (uint32_t base = 0; paso && base + paso <= largo; base += paso) {
            uint16_t equipo = U16(pkb + base + 32);
            if (equipo >= equipos)
                continue;
            for (uint32_t k = 0; k < 16; ++k) {
                const uint8_t *hueco = pkb + base + 64 + 8 * k;
                if (U16(hueco) && hueco[4] > salida[equipo])
                    salida[equipo] = hueco[4];
            }
        }
    }

}
