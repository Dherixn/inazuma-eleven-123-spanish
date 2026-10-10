#include "DatosDestino.hpp"

#include <algorithm>
#include <cstdio>
#include <map>

#include "Partida.hpp"

namespace Definitiva {

    // Medidos en la recopilación (ver tools/src/ie123kit/definitiva/fichas.py).
    const PerfilJuego PERFIL_IE1 = {96, 64, 66, 85, 32, 32, 64, false, true, false};
    const PerfilJuego PERFIL_IE2 = {96, 64, 66, 85, 32, 32, 80, false, false, false};
    const PerfilJuego PERFIL_IE3 = {104, 76, 78, 93, 0, 28, 72, true, false, true};

    namespace {
        constexpr uint8_t NIVEL_NO_POR_NIVEL = 100;
        constexpr uint32_t VALORES_POR_CURVA = 98;

        uint16_t U16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }
        uint32_t U32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }

        uint8_t Ror2(uint8_t v) { return static_cast<uint8_t>((v >> 2) | ((v & 3) << 6)); }

        // Transformación del IE3 (la de item.dat) para un registro de cualquier tamaño.
        std::vector<uint8_t> Desofuscar(const uint8_t *reg, size_t largo) {
            std::vector<uint8_t> s(largo);
            for (size_t i = 0; i < largo; ++i)
                s[i] = Ror2(static_cast<uint8_t>(reg[i] ^ 0xAD));
            for (size_t ancho : {3, 5, 7, 2})
                for (size_t ini = 0; ini + ancho <= largo; ini += ancho)
                    std::swap(s[ini], s[ini + ancho - 1]);
            return s;
        }

        uint8_t PosicionDeByte(uint8_t byte) {
            uint8_t n = byte >> 4;
            return (n == 2 || n == 4 || n == 6 || n == 8) ? n : 0;
        }

        uint32_t ExpCb(uint16_t numero, uint8_t nivel, void *ctx) {
            return static_cast<const JuegoDatos *>(ctx)->ExpMinima(numero, nivel);
        }
        int AprendidasCb(uint16_t numero, uint8_t nivel, uint16_t salida[4], void *ctx) {
            return static_cast<const JuegoDatos *>(ctx)->Aprendidas(numero, nivel, salida);
        }
        uint32_t ClaveCb(uint16_t numero, void *ctx) { return static_cast<const JuegoDatos *>(ctx)->Clave(numero); }
    }

    bool JuegoDatos::Construir(const PerfilJuego &p, const std::vector<uint8_t> &unitbase,
                               const std::vector<uint8_t> &unitstat, const std::vector<uint8_t> &lvup,
                               const std::vector<uint8_t> &unitno) {
        size_t total = unitbase.size() / p.basePaso;
        if (!total || unitstat.size() / p.statPaso != total || unitno.size() / 2 != total || !lvup.size()
            || lvup.size() % (VALORES_POR_CURVA * 4))
            return false;
        curvas_.assign(lvup.size() / (VALORES_POR_CURVA * 4), std::vector<uint32_t>(VALORES_POR_CURVA));
        for (size_t c = 0; c < curvas_.size(); ++c)
            for (uint32_t v = 0; v < VALORES_POR_CURVA; ++v)
                curvas_[c][v] = U32(&lvup[(c * VALORES_POR_CURVA + v) * 4]);
        curva_.resize(total);
        caras_.resize(total);
        posiciones_.resize(total);
        jugables_.resize(total);
        numeros_.resize(total);
        claves_.resize(total);
        tecnicas_.assign(total * 4, 0);
        niveles_.assign(total * 4, 0);
        anchoNombre_ = p.anchoNombre;
        nombres_.assign(total * anchoNombre_, 0);
        std::map<uint16_t, uint32_t> primera;
        for (size_t i = 0; i < total; ++i) {
            const uint8_t *b = unitbase.data() + i * p.basePaso;
            curva_[i] = b[p.offIdentidad];
            caras_[i] = U16(b + p.offCara);
            posiciones_[i] = PosicionDeByte(b[p.offPosicion]);
            numeros_[i] = U16(&unitno[i * 2]);
            primera.emplace(numeros_[i], static_cast<uint32_t>(i));

            // Clave del personaje: nombre completo sin espacios de ancho completo ni blancos; si queda
            // vacío, "cara:<n>" (igual que DatosJuego.personaje), y de eso su CRC32.
            std::vector<uint8_t> nombre;
            for (uint32_t k = 0; k < p.anchoNombre && b[p.offNombre + k]; ++k)
                nombre.push_back(b[p.offNombre + k]);
            std::copy(b + p.offNombre, b + p.offNombre + p.anchoNombre, nombres_.begin() + i * anchoNombre_);
            std::vector<uint8_t> limpio;
            for (size_t k = 0; k < nombre.size(); ++k) {
                if (k + 1 < nombre.size() && nombre[k] == 0x81 && nombre[k + 1] == 0x40) {
                    ++k;
                    continue;
                }
                limpio.push_back(nombre[k]);
            }
            auto blanco = [](uint8_t c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; };
            while (!limpio.empty() && blanco(limpio.back()))
                limpio.pop_back();
            size_t ini = 0;
            while (ini < limpio.size() && blanco(limpio[ini]))
                ++ini;
            limpio.erase(limpio.begin(), limpio.begin() + ini);
            if (limpio.empty()) {
                char texto[24];
                int n = std::snprintf(texto, sizeof(texto), "cara:%u", caras_[i]);
                limpio.assign(texto, texto + n);
            }
            claves_[i] = Crc32(limpio.data(), limpio.size());

            // data() + offset: &v[v.size()] es un índice fuera de rango y la libstdc++ de devkitARM aborta.
            const uint8_t *reg = unitstat.data() + i * p.statPaso;
            std::vector<uint8_t> s = p.statOfuscado ? Desofuscar(reg, p.statPaso)
                                                    : std::vector<uint8_t>(reg, reg + p.statPaso);
            jugables_[i] = U16(&s[2]) > 1;
            for (int k = 0; k < 4; ++k) {
                if (p.tecnicaU8) {
                    tecnicas_[i * 4 + k] = s[44 + 2 * k];
                    niveles_[i * 4 + k] = s[45 + 2 * k];
                } else {
                    tecnicas_[i * 4 + k] = U16(&s[44 + 4 * k]);
                    uint16_t nv = p.nivelU8 ? s[46 + 4 * k] : U16(&s[46 + 4 * k]);
                    niveles_[i * 4 + k] = static_cast<uint8_t>(nv > 255 ? 255 : nv);
                }
            }
        }
        indice_.assign(primera.begin(), primera.end());
        return true;
    }

    int JuegoDatos::Ficha(uint16_t numero) const {
        auto it = std::lower_bound(indice_.begin(), indice_.end(), numero,
                                   [](const std::pair<uint16_t, uint32_t> &e, uint16_t n) { return e.first < n; });
        return (it != indice_.end() && it->first == numero) ? static_cast<int>(it->second) : -1;
    }

    uint32_t JuegoDatos::ExpMinima(uint16_t numero, uint8_t nivel) const {
        int f = Ficha(numero);
        if (f < 0 || nivel <= 1 || curva_[f] >= curvas_.size())
            return 0;
        return curvas_[curva_[f]][(nivel > 99 ? 99 : nivel) - 2];
    }

    int JuegoDatos::Aprendidas(uint16_t numero, uint8_t nivel, uint16_t salida[4]) const {
        int f = Ficha(numero), n = 0;
        if (f < 0)
            return 0;
        for (int k = 0; k < 4; ++k) {
            uint16_t t = tecnicas_[f * 4 + k];
            uint8_t nv = niveles_[f * 4 + k];
            if (t && nv < NIVEL_NO_POR_NIVEL && nv <= nivel)
                salida[n++] = t;
        }
        return n;
    }

    uint32_t JuegoDatos::Clave(uint16_t numero) const {
        int f = Ficha(numero);
        return f < 0 ? (0x80000000u | numero) : claves_[f];
    }

    void JuegoDatos::Preparar(Opciones &op) const {
        op.expDeNivel = ExpCb;
        op.aprendidas = AprendidasCb;
        op.clave = ClaveCb;
        op.contexto = const_cast<JuegoDatos *>(this);
    }

    std::vector<Par> EmparejarPorCara(const JuegoDatos &origen, const JuegoDatos &destino) {
        std::map<uint16_t, std::vector<size_t>> porCara;
        for (size_t i = 0; i < destino.Fichas(); ++i)
            if (destino.Cara(i) && destino.Jugable(i))
                porCara[destino.Cara(i)].push_back(i);
        std::map<uint16_t, uint16_t> pares;  // número de origen → número de destino (gana el último)
        for (size_t i = 0; i < origen.Fichas(); ++i) {
            if (!origen.Cara(i) || !origen.Jugable(i))
                continue;
            auto it = porCara.find(origen.Cara(i));
            if (it == porCara.end())
                continue;
            size_t elegido = SIZE_MAX, cualquiera = SIZE_MAX;
            for (size_t c : it->second) {
                cualquiera = std::min(cualquiera, c);
                if (destino.Posicion(c) == origen.Posicion(i))
                    elegido = std::min(elegido, c);
            }
            pares[origen.Numero(i)] = destino.Numero(elegido != SIZE_MAX ? elegido : cualquiera);
        }
        std::vector<Par> salida;
        for (const auto &kv : pares)
            salida.push_back({kv.first, kv.second});
        return salida;
    }

    namespace {
        // Nombres de relleno («？？？»): varios personajes distintos lo comparten, no identifican a nadie.
        bool Provisional(const uint8_t *nombre, size_t ancho) {
            bool alguno = false;
            for (size_t k = 0; k < ancho && nombre[k];) {
                if (k + 1 < ancho && nombre[k] == 0x81 && (nombre[k + 1] == 0x48 || nombre[k + 1] == 0x40)) {
                    alguno |= nombre[k + 1] == 0x48;
                    k += 2;
                } else if (nombre[k] == '?' || nombre[k] == ' ') {
                    alguno |= nombre[k] == '?';
                    ++k;
                } else {
                    return false;
                }
            }
            return alguno;
        }
    }

    std::vector<Par> PreferirAltas(const std::vector<Par> &pares, const JuegoDatos &destino,
                                   const std::vector<uint16_t> &altas) {
        auto enAltas = [&altas](uint16_t n) { return std::binary_search(altas.begin(), altas.end(), n); };
        std::vector<size_t> candidatas;  // jugables que la historia ficha, en orden de ficha
        for (size_t i = 0; i < destino.Fichas(); ++i)
            if (destino.Jugable(i) && enAltas(destino.Numero(i)))
                candidatas.push_back(i);
        std::vector<Par> salida = pares;
        for (Par &p : salida) {
            if (enAltas(p.destino))
                continue;
            int f = destino.Ficha(p.destino);
            if (f < 0 || Provisional(destino.NombreCrudo(static_cast<size_t>(f)), destino.AnchoNombre()))
                continue;
            uint32_t clave = destino.Clave(p.destino);
            size_t misma = SIZE_MAX, cualquiera = SIZE_MAX;
            for (size_t i : candidatas) {
                if (destino.Clave(destino.Numero(i)) != clave)
                    continue;
                if (cualquiera == SIZE_MAX)
                    cualquiera = i;
                if (misma == SIZE_MAX && destino.Posicion(i) == destino.Posicion(static_cast<size_t>(f)))
                    misma = i;
            }
            size_t elegida = misma != SIZE_MAX ? misma : cualquiera;
            if (elegida != SIZE_MAX)
                p.destino = destino.Numero(elegida);
        }
        return salida;
    }

}
