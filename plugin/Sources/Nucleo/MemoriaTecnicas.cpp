#include "MemoriaTecnicas.hpp"

#include <algorithm>
#include <cstdlib>

namespace Definitiva {

    bool MemoriaTecnicas::Apuntar(uint32_t clave, const uint16_t *tecnicas, size_t cuantas) {
        std::vector<uint16_t> &lista = mem_[clave];
        bool nueva = false;
        for (size_t k = 0; k < cuantas; ++k) {
            uint16_t t = tecnicas[k];
            if (!t || std::find(lista.begin(), lista.end(), t) != lista.end())
                continue;
            if (lista.size() >= MAX_POR_JUGADOR)
                lista.erase(lista.begin());  // se olvida la más antigua
            lista.push_back(t);
            nueva = true;
        }
        if (lista.empty())
            mem_.erase(clave);
        return nueva;
    }

    std::vector<uint16_t> MemoriaTecnicas::Perdidas(uint32_t clave, const uint16_t *tecnicas, size_t cuantas) const {
        std::vector<uint16_t> salida;
        auto it = mem_.find(clave);
        if (it == mem_.end())
            return salida;
        for (uint16_t t : it->second)
            if (std::find(tecnicas, tecnicas + cuantas, t) == tecnicas + cuantas)
                salida.push_back(t);
        return salida;
    }

    std::string MemoriaTecnicas::Serializar() const {
        std::string s;
        for (const auto &kv : mem_) {
            s += std::to_string(kv.first) + "=";
            for (size_t k = 0; k < kv.second.size(); ++k)
                s += (k ? "," : "") + std::to_string(kv.second[k]);
            s += "\n";
        }
        return s;
    }

    void MemoriaTecnicas::Cargar(const std::string &texto) {
        mem_.clear();
        size_t ini = 0;
        while (ini < texto.size()) {
            size_t fin = texto.find('\n', ini);
            std::string linea = texto.substr(ini, fin == std::string::npos ? std::string::npos : fin - ini);
            ini = fin == std::string::npos ? texto.size() : fin + 1;
            size_t igual = linea.find('=');
            if (igual == std::string::npos)
                continue;
            // strtoul: en la 3DS long es de 32 bits y las claves llegan a 0xFFFFFFFF.
            char *finClave = nullptr;
            unsigned long clave = std::strtoul(linea.c_str(), &finClave, 10);
            if (finClave != linea.c_str() + igual || !clave)
                continue;
            std::vector<uint16_t> lista;
            const char *p = linea.c_str() + igual + 1;
            while (*p && lista.size() < MAX_POR_JUGADOR) {
                if (*p == ',') {
                    ++p;
                    continue;
                }
                char *fin2 = nullptr;
                long t = std::strtol(p, &fin2, 10);
                if (fin2 == p) {  // no es un número: se salta hasta la siguiente coma
                    while (*p && *p != ',')
                        ++p;
                    continue;
                }
                if (t > 0 && t <= 0xFFFF)
                    lista.push_back(static_cast<uint16_t>(t));
                p = fin2;
            }
            if (!lista.empty())
                mem_[static_cast<uint32_t>(clave)] = lista;
        }
    }

}
