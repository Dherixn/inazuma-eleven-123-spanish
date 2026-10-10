#include "Texto.hpp"

namespace Definitiva {

    namespace {
        size_t Caracteres(const std::string &s) {
            size_t n = 0;
            for (unsigned char c : s)
                if ((c & 0xC0) != 0x80)
                    ++n;
            return n;
        }
    }

    std::string Ajustar(const std::string &texto, size_t ancho) {
        std::string salida;
        size_t ini = 0;
        while (ini <= texto.size()) {
            size_t fin = texto.find('\n', ini);
            std::string parrafo = texto.substr(ini, fin == std::string::npos ? std::string::npos : fin - ini);
            std::string linea;
            size_t largo = 0, pos = 0;
            while (pos < parrafo.size()) {
                size_t esp = parrafo.find(' ', pos);
                std::string palabra = parrafo.substr(pos, esp == std::string::npos ? std::string::npos : esp - pos);
                size_t n = Caracteres(palabra);
                if (largo && largo + 1 + n > ancho) {
                    salida += linea + '\n';
                    linea.clear();
                    largo = 0;
                }
                if (largo) {
                    linea += ' ';
                    ++largo;
                }
                linea += palabra;
                largo += n;
                if (esp == std::string::npos)
                    break;
                pos = esp + 1;
            }
            salida += linea;
            if (fin == std::string::npos)
                break;
            salida += '\n';
            ini = fin + 1;
        }
        return salida;
    }

}
