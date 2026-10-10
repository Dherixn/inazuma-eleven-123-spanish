#include "LectorFA.hpp"

#include <cstring>

namespace Definitiva {

    namespace {
        uint32_t U32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }
        uint16_t U16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }
        constexpr uint32_t ENTRADA_DIR = 24;
        constexpr uint32_t ENTRADA_ARCHIVO = 16;
    }

    bool LectorFA::Abrir(LeerBytes leer, void *contexto) {
        leer_ = leer;
        contexto_ = contexto;
        uint8_t cab[0x48];
        if (!leer_(contexto_, 0, sizeof(cab), cab) || std::memcmp(cab, "B123", 4) != 0)
            return false;
        offDirs_ = U32(cab + 0x04);
        offArchivos_ = U32(cab + 0x0C);
        offNombres_ = U32(cab + 0x10);
        offDatos_ = U32(cab + 0x14);
        numDirs_ = U16(cab + 0x18);
        if (offNombres_ <= offArchivos_ || offDatos_ <= offNombres_ || offDatos_ - offNombres_ > 0x200000)
            return false;
        dirs_.resize(numDirs_ * ENTRADA_DIR);
        nombres_.resize(offDatos_ - offNombres_);
        return leer_(contexto_, offDirs_, static_cast<uint32_t>(dirs_.size()), dirs_.data())
               && leer_(contexto_, offNombres_, static_cast<uint32_t>(nombres_.size()), nombres_.data());
    }

    std::string LectorFA::Nombre(uint32_t offset) const {
        if (offset >= nombres_.size())
            return std::string();
        const char *p = reinterpret_cast<const char *>(&nombres_[offset]);
        return std::string(p, strnlen(p, nombres_.size() - offset));
    }

    bool LectorFA::Leer(const std::string &ruta, std::vector<uint8_t> &salida) const {
        uint64_t offset;
        uint32_t largo;
        if (!Localizar(ruta, &offset, &largo))
            return false;
        salida.resize(largo);
        return LeerTramo(offset, largo, salida.data());
    }

    bool LectorFA::LeerTramo(uint64_t offset, uint32_t largo, uint8_t *salida) const {
        return leer_ && leer_(contexto_, offset, largo, salida);
    }

    bool LectorFA::Localizar(const std::string &ruta, uint64_t *offset, uint32_t *largo) const {
        size_t corte = ruta.rfind('/');
        std::string carpeta = corte == std::string::npos ? std::string() : ruta.substr(0, corte + 1);
        std::string archivo = corte == std::string::npos ? ruta : ruta.substr(corte + 1);
        for (uint32_t d = 0; d < numDirs_; ++d) {
            const uint8_t *e = &dirs_[d * ENTRADA_DIR];
            if (Nombre(U32(e + 0x14)) != carpeta)
                continue;
            uint16_t cuantos = U16(e + 0x04);
            uint32_t baseNombres = U32(e + 0x08), primero = U32(e + 0x0C);
            std::vector<uint8_t> entradas(cuantos * ENTRADA_ARCHIVO);
            if (!leer_(contexto_, offArchivos_ + primero * ENTRADA_ARCHIVO, static_cast<uint32_t>(entradas.size()),
                       entradas.data()))
                return false;
            for (uint16_t k = 0; k < cuantos; ++k) {
                const uint8_t *f = &entradas[k * ENTRADA_ARCHIVO];
                if (Nombre(baseNombres + U32(f + 0x04)) != archivo)
                    continue;
                *offset = static_cast<uint64_t>(offDatos_) + U32(f + 0x08);
                *largo = U32(f + 0x0C);
                return true;
            }
            return false;
        }
        return false;
    }

}
