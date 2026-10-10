#include "NombreSjis.hpp"

namespace Definitiva {

    namespace {
        // Portadores 0x839F..0x83AE (Α..Π) → letra española, en el orden de font_patch.PLAN.
        const char *PORTADORES[] = {"á", "é", "í", "ó", "ú", "ü", "ñ", "Á", "É", "Í", "Ó", "Ú", "Ñ", "¡", "¿", "È"};
    }

    std::string DecodificarNombre(const uint8_t *campo, size_t ancho) {
        std::string s;
        for (size_t i = 0; i < ancho && campo[i];) {
            uint8_t a = campo[i];
            bool doble = (a >= 0x81 && a <= 0x9F) || (a >= 0xE0 && a <= 0xFC);
            if (!doble) {
                s += a < 0x80 ? static_cast<char>(a) : '?';
                ++i;
                continue;
            }
            if (i + 1 >= ancho)
                break;
            unsigned codigo = static_cast<unsigned>(a) << 8 | campo[i + 1];
            i += 2;
            if (codigo >= 0x8260 && codigo <= 0x8279)
                s += static_cast<char>('A' + (codigo - 0x8260));
            else if (codigo >= 0x8281 && codigo <= 0x829A)
                s += static_cast<char>('a' + (codigo - 0x8281));
            else if (codigo >= 0x824F && codigo <= 0x8258)
                s += static_cast<char>('0' + (codigo - 0x824F));
            else if (codigo >= 0x839F && codigo <= 0x83AE)
                s += PORTADORES[codigo - 0x839F];
            else if (codigo == 0x8140)
                s += ' ';
            else if (codigo == 0x8144)
                s += '.';
            else if (codigo == 0x8143)
                s += ',';
            else if (codigo == 0x815B || codigo == 0x817C)
                s += '-';
            else
                s += "·";
        }
        while (!s.empty() && s.back() == ' ')
            s.pop_back();
        return s;
    }

    std::string NormalizarBusqueda(const std::string &utf8) {
        static const char *CON[] = {"á", "é", "í", "ó", "ú", "ü", "ñ", "Á", "É", "Í", "Ó", "Ú", "Ü", "Ñ"};
        static const char SIN[] = "aeiouunaeiouun";
        std::string s;
        for (size_t i = 0; i < utf8.size();) {
            bool hecho = false;
            for (size_t k = 0; k < sizeof(CON) / sizeof(CON[0]); ++k) {
                if (utf8.compare(i, 2, CON[k]) == 0) {
                    s += SIN[k];
                    i += 2;
                    hecho = true;
                    break;
                }
            }
            if (hecho)
                continue;
            unsigned char c = static_cast<unsigned char>(utf8[i++]);
            s += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : static_cast<char>(c);
        }
        return s;
    }

}
