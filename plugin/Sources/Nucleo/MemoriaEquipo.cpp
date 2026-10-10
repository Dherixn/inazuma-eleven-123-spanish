#include "MemoriaEquipo.hpp"

#include <algorithm>
#include <cstdio>

namespace Definitiva {

    namespace {

        // Solo dígitos de esa base (10 o 16); vacío, basura o fuera de rango = 0. Como _entero en Python.
        uint32_t Entero(const std::string &texto, int base, uint32_t maximo) {
            if (texto.empty() || texto.size() > 10)
                return 0;
            uint64_t valor = 0;
            for (char c : texto) {
                int d = c >= '0' && c <= '9' ? c - '0'
                        : base == 16 && c >= 'a' && c <= 'f' ? c - 'a' + 10
                        : base == 16 && c >= 'A' && c <= 'F' ? c - 'A' + 10
                                                               : -1;
                if (d < 0)
                    return 0;
                valor = valor * static_cast<uint64_t>(base) + static_cast<uint64_t>(d);
            }
            return valor <= maximo ? static_cast<uint32_t>(valor) : 0;
        }

        std::vector<std::string> Partir(const std::string &texto, char separador) {
            std::vector<std::string> trozos;
            size_t ini = 0;
            while (true) {
                size_t fin = texto.find(separador, ini);
                trozos.push_back(texto.substr(ini, fin == std::string::npos ? std::string::npos : fin - ini));
                if (fin == std::string::npos)
                    return trozos;
                ini = fin + 1;
            }
        }

        bool PonerTecnica(std::vector<TecnicaRecordada> &lista, uint16_t id, uint8_t nivel) {
            for (TecnicaRecordada &t : lista) {
                if (t.id != id)
                    continue;
                if (nivel <= t.nivel)
                    return false;
                t.nivel = nivel;
                return true;
            }
            if (lista.size() >= MemoriaEquipo::MAX_POR_JUGADOR)
                lista.erase(lista.begin());  // se olvida la más antigua
            lista.push_back({id, nivel});
            return true;
        }

    }

    bool MemoriaEquipo::Apuntar(uint32_t clave, uint16_t numero, uint8_t nivel, uint32_t exp, const uint16_t *tecnicas,
                                const uint8_t *niveles, size_t cuantas) {
        Recuerdo &r = mem_[clave];
        bool cambio = false;
        for (size_t k = 0; k < cuantas; ++k)
            if (tecnicas[k])
                cambio |= PonerTecnica(r.tecnicas, tecnicas[k], niveles ? niveles[k] : 0);
        if (numero && numero != r.numero) {
            r.numero = numero;
            cambio = true;
        }
        if (nivel > r.nivel || (nivel == r.nivel && exp > r.exp)) {
            r.nivel = nivel;
            r.exp = exp;
            cambio = true;
        }
        if (r.pendiente) {
            r.pendiente = false;
            cambio = true;
        }
        if (r.tecnicas.empty() && !r.numero)
            mem_.erase(clave);
        return cambio;
    }

    void MemoriaEquipo::Esperar(uint32_t clave, const Recuerdo &r) {
        Recuerdo &dest = mem_[clave];
        dest = r;
        dest.pendiente = true;
        if (dest.tecnicas.size() > MAX_POR_JUGADOR)
            dest.tecnicas.erase(dest.tecnicas.begin(), dest.tecnicas.end() - MAX_POR_JUGADOR);
    }

    std::vector<uint16_t> MemoriaEquipo::Perdidas(uint32_t clave, const uint16_t *tecnicas, size_t cuantas) const {
        std::vector<uint16_t> salida;
        const Recuerdo *r = Buscar(clave);
        if (!r)
            return salida;
        for (const TecnicaRecordada &t : r->tecnicas)
            if (std::find(tecnicas, tecnicas + cuantas, t.id) == tecnicas + cuantas)
                salida.push_back(t.id);
        return salida;
    }

    const Recuerdo *MemoriaEquipo::Buscar(uint32_t clave) const {
        auto it = mem_.find(clave);
        return it == mem_.end() ? nullptr : &it->second;
    }

    std::string MemoriaEquipo::Serializar() const {
        char cabecera[48];
        std::snprintf(cabecera, sizeof cabecera, "#partida=%08lX\n#tiempo=%lu\n", static_cast<unsigned long>(partida),
                      static_cast<unsigned long>(tiempo));
        std::string s = cabecera;
        for (const auto &kv : mem_) {
            const Recuerdo &r = kv.second;
            s += std::to_string(kv.first) + "=";
            for (size_t k = 0; k < r.tecnicas.size(); ++k) {
                s += (k ? "," : "") + std::to_string(r.tecnicas[k].id);
                if (r.tecnicas[k].nivel)
                    s += ":" + std::to_string(r.tecnicas[k].nivel);
            }
            if (r.numero)
                s += ";n=" + std::to_string(r.numero) + ";nv=" + std::to_string(r.nivel) + ";xp=" + std::to_string(r.exp);
            if (r.pendiente)
                s += ";p=1";
            s += "\n";
        }
        return s;
    }

    void MemoriaEquipo::Cargar(const std::string &texto) {
        mem_.clear();
        partida = tiempo = 0;
        for (const std::string &linea : Partir(texto, '\n')) {
            size_t igual = linea.find('=');
            if (!linea.empty() && linea[0] == '#') {
                std::string nombre = linea.substr(1, igual == std::string::npos ? std::string::npos : igual - 1);
                std::string valor = igual == std::string::npos ? "" : linea.substr(igual + 1);
                if (nombre == "partida")
                    partida = Entero(valor, 16, 0xFFFFFFFF);
                else if (nombre == "tiempo")
                    tiempo = Entero(valor, 10, 0xFFFFFFFF);
                continue;
            }
            uint32_t clave = igual == std::string::npos ? 0 : Entero(linea.substr(0, igual), 10, 0xFFFFFFFF);
            if (!clave)
                continue;
            std::vector<std::string> campos = Partir(linea.substr(igual + 1), ';');
            Recuerdo r;
            for (const std::string &elemento : Partir(campos[0], ',')) {
                size_t dos = elemento.find(':');
                uint32_t id = Entero(elemento.substr(0, dos), 10, 0xFFFF);
                if (id && r.tecnicas.size() < MAX_POR_JUGADOR)
                    r.tecnicas.push_back({static_cast<uint16_t>(id), static_cast<uint8_t>(
                                              dos == std::string::npos ? 0 : Entero(elemento.substr(dos + 1), 10, 0xFF))});
            }
            for (size_t k = 1; k < campos.size(); ++k) {
                size_t i = campos[k].find('=');
                std::string nombre = campos[k].substr(0, i), valor = i == std::string::npos ? "" : campos[k].substr(i + 1);
                if (nombre == "n")
                    r.numero = static_cast<uint16_t>(Entero(valor, 10, 0xFFFF));
                else if (nombre == "nv")
                    r.nivel = static_cast<uint8_t>(Entero(valor, 10, 99));
                else if (nombre == "xp")
                    r.exp = Entero(valor, 10, 0xFFFFFFFF);
                else if (nombre == "p")
                    r.pendiente = valor == "1";
            }
            if (!r.tecnicas.empty() || r.numero)
                mem_[clave] = r;
        }
    }

    bool Restaurar(JugadorPartida &j, const Recuerdo &r) {
        if (!r.pendiente && r.nivel <= j.nivel)
            return false;
        bool cambio = false;
        if (r.nivel > j.nivel || (r.nivel == j.nivel && r.exp > j.exp)) {
            j.nivel = r.nivel;
            j.exp = r.exp;
            cambio = true;
        }
        for (const TecnicaRecordada &t : r.tecnicas) {
            uint16_t *fin = j.tecnicas + NUM_TECNICAS;
            uint16_t *esta = std::find(j.tecnicas, fin, t.id);
            if (esta != fin) {
                uint8_t &nivel = j.nivelesTecnica[esta - j.tecnicas];
                if (t.nivel > nivel) {
                    nivel = t.nivel;
                    cambio = true;
                }
                continue;
            }
            uint16_t *libre = std::find(j.tecnicas, fin, static_cast<uint16_t>(0));
            if (libre != fin) {
                *libre = t.id;
                j.nivelesTecnica[libre - j.tecnicas] = t.nivel;
                cambio = true;
            }
        }
        return cambio;
    }

}
