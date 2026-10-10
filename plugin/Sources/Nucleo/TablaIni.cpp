#include "TablaIni.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Definitiva {

    namespace {
        bool ClaveEs(const uint8_t *entrada, const char *clave) {
            size_t n = std::strlen(clave);
            return n < INI_TEXTO && std::memcmp(entrada, clave, n) == 0 && entrada[n] == 0;
        }
    }

    bool EsTablaIni(const uint8_t *tabla, uint32_t entradas) {
        return entradas >= 2 && entradas <= INI_MAX_ENTRADAS && ClaveEs(tabla, "FARCLIP_LENGTH")
               && ClaveEs(tabla + INI_ENTRADA, "MDL_SCALE_CHAR");
    }

    uint32_t ContarEntradas(const uint8_t *tabla, uint32_t max) {
        uint32_t n = 0;
        for (; n < max && n < INI_MAX_ENTRADAS; ++n) {
            const uint8_t *e = tabla + n * INI_ENTRADA;
            uint32_t k = 0;
            while (k < INI_TEXTO && ((e[k] >= 'A' && e[k] <= 'Z') || (e[k] >= '0' && e[k] <= '9') || e[k] == '_'))
                ++k;
            if (k == 0 || k == INI_TEXTO || e[k] != 0)
                break;
        }
        return n;
    }

    int BuscarClave(const uint8_t *tabla, uint32_t entradas, const char *clave) {
        for (uint32_t i = 0; i < entradas && i < INI_MAX_ENTRADAS; ++i)
            if (ClaveEs(tabla + i * INI_ENTRADA, clave))
                return static_cast<int>(i);
        return -1;
    }

    int32_t LeerValor(const uint8_t *entrada) {
        char texto[INI_TEXTO + 1];
        std::memcpy(texto, entrada + INI_TEXTO, INI_TEXTO);
        texto[INI_TEXTO] = 0;
        return static_cast<int32_t>(std::atoi(texto));
    }

    bool PonerValor(uint8_t *entrada, int32_t valor) {
        char texto[INI_TEXTO] = {};
        std::snprintf(texto, sizeof(texto), "%ld", static_cast<long>(valor));
        if (std::memcmp(entrada + INI_TEXTO, texto, INI_TEXTO) == 0)
            return false;
        std::memcpy(entrada + INI_TEXTO, texto, INI_TEXTO);
        return true;
    }

    uint32_t BuscarTablas(const uint8_t *zona, size_t largo, uint32_t *salida, uint32_t max) {
        uint32_t n = 0;
        for (size_t off = 0; off + 2 * INI_ENTRADA <= largo && n < max; off += INI_ENTRADA)
            if (zona[off] == 'F' && EsTablaIni(zona + off, 2))
                salida[n++] = static_cast<uint32_t>(off);
        return n;
    }

}
