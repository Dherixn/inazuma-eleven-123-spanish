#include "Partida.hpp"

#include <cstring>

namespace Definitiva {

    // IE1: 32 fichas de 0x60 B desde 0x79C (medido en Azahar); solo se lee.
    const FormatoPartida FORMATO_IE1 = {"ie1.123", 0x54C0, 0x79C, 0x60, false, 32};
    const FormatoPartida FORMATO_IE2 = {"ie2.123", 0x7DC0, 0x1268, 0x70, true, MAX_JUGADORES};
    const FormatoPartida FORMATO_IE3 = {"ie3.123", 0x5940, 0x11B0, 0x6C, true, MAX_JUGADORES};

    const char *DescribirError(Error e) {
        switch (e) {
            case Error::Ninguno: return "sin error";
            case Error::PartidaCorta: return "la partida es más corta de lo que pide su formato";
            case Error::FormatoNoVerificado: return "formato de partida no verificado: no se escribe";
            case Error::SumaCabecera: return "suma de control de la cabecera incorrecta";
            case Error::SumaInfo: return "suma de control de la info incorrecta";
            case Error::SumaDatos: return "suma de control de los datos incorrecta";
            case Error::SinMarca: return "falta la marca de partida";
            case Error::IndiceFueraDeRango: return "jugador fuera de rango";
            case Error::PlantillaLlena: return "la plantilla está llena";
            case Error::TamanoFicha: return "ficha de jugador de otro tamaño";
            case Error::OpcionesIncompletas: return "al modo de herencia le faltan datos";
        }
        return "error desconocido";
    }

    namespace {
        struct TablaCrc {
            uint32_t v[256];
            TablaCrc() {
                for (uint32_t i = 0; i < 256; ++i) {
                    uint32_t c = i;
                    for (int k = 0; k < 8; ++k)
                        c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
                    v[i] = c;
                }
            }
        };

        uint16_t U16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }
        uint32_t U32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }
        void PonU16(uint8_t *p, uint16_t v) { p[0] = v & 0xFF; p[1] = v >> 8; }
        void PonU32(uint8_t *p, uint32_t v) { for (int k = 0; k < 4; ++k) p[k] = (v >> (8 * k)) & 0xFF; }
    }

    uint32_t Crc32(const uint8_t *datos, size_t largo) {
        static const TablaCrc tabla;
        uint32_t c = 0xFFFFFFFFu;
        for (size_t i = 0; i < largo; ++i)
            c = tabla.v[(c ^ datos[i]) & 0xFF] ^ (c >> 8);
        return c ^ 0xFFFFFFFFu;
    }

    JugadorPartida LeerJugador(const uint8_t *reg, size_t largo) {
        JugadorPartida j;
        j.exp = U32(reg + 0x00);
        j.id = U16(reg + 0x04);
        for (uint32_t k = 0; k < NUM_TECNICAS; ++k) {
            j.tecnicas[k] = U16(reg + 0x38 + 2 * k);
            j.nivelesTecnica[k] = reg[0x44 + k];
        }
        j.nivel = reg[0x4A];
        j.crudo.assign(reg, reg + largo);
        return j;
    }

    std::vector<uint8_t> EscribirJugador(const JugadorPartida &j) {
        std::vector<uint8_t> reg = j.crudo;
        PonU32(&reg[0x00], j.exp);
        PonU16(&reg[0x04], j.id);
        for (uint32_t k = 0; k < NUM_TECNICAS; ++k) {
            PonU16(&reg[0x38 + 2 * k], j.tecnicas[k]);
            reg[0x44 + k] = j.nivelesTecnica[k];
        }
        reg[0x4A] = j.nivel;
        return reg;
    }

    bool Partida::Cargar(std::vector<uint8_t> datos, const FormatoPartida *formato, uint32_t espacio) {
        if (!formato || espacio >= NUM_ESPACIOS)
            return false;
        uint32_t base = espacio * formato->EspacioLargo();
        if (datos.size() < base + DATOS_INICIO + formato->datosLargo)
            return false;
        datos_ = std::move(datos);
        formato_ = formato;
        base_ = base;
        return true;
    }

    bool Partida::Ocupado() const { return LeerU32(base_ + OFF_MARCA) == MARCA; }

    uint32_t Partida::LeerU32(uint32_t off) const { return U32(&datos_[off]); }

    void Partida::EscribirU32(uint32_t off, uint32_t valor) { PonU32(&datos_[off], valor); }

    Error Partida::Validar() const {
        if (Crc32(&datos_[0x04], 0x3C) != LeerU32(OFF_CRC_CABECERA))
            return Error::SumaCabecera;
        if (Crc32(&datos_[base_ + INFO_INICIO], INFO_LARGO) != LeerU32(base_ + OFF_CRC_INFO))
            return Error::SumaInfo;
        if (Crc32(&datos_[base_ + DATOS_INICIO], formato_->datosLargo) != LeerU32(base_ + OFF_CRC_DATOS))
            return Error::SumaDatos;
        if (!Ocupado())
            return Error::SinMarca;
        return Error::Ninguno;
    }

    Error Partida::Sellar() {
        if (!formato_->verificado)
            return Error::FormatoNoVerificado;
        EscribirU32(base_ + OFF_CRC_DATOS, Crc32(&datos_[base_ + DATOS_INICIO], formato_->datosLargo));
        EscribirU32(base_ + OFF_CRC_INFO, Crc32(&datos_[base_ + INFO_INICIO], INFO_LARGO));
        return Error::Ninguno;
    }

    JugadorPartida Partida::Jugador(uint32_t indice) const {
        return LeerJugador(&datos_[OffJugador(indice)], formato_->jugadorPaso);
    }

    Error Partida::EscribirReg(uint32_t indice, const JugadorPartida &j) {
        std::vector<uint8_t> reg = EscribirJugador(j);
        if (reg.size() != formato_->jugadorPaso)
            return Error::TamanoFicha;
        std::memcpy(&datos_[OffJugador(indice)], reg.data(), reg.size());
        return Error::Ninguno;
    }

    Error Partida::PonerJugador(uint32_t indice, const JugadorPartida &j) {
        if (indice >= NumJugadores())
            return Error::IndiceFueraDeRango;
        return EscribirReg(indice, j);
    }

    Error Partida::AnadirJugador(const JugadorPartida &j, uint32_t *indice) {
        uint32_t n = NumJugadores();
        if (n >= formato_->maxJugadores)
            return Error::PlantillaLlena;
        Error e = EscribirReg(n, j);
        if (e != Error::Ninguno)
            return e;
        datos_[base_ + OFF_NUM_JUGADORES] = static_cast<uint8_t>(n + 1);
        if (indice)
            *indice = n;
        return Error::Ninguno;
    }

    const FormatoPartida *DetectarFormato(const uint8_t *datos, size_t largo) {
        if (largo < 0x14)
            return nullptr;
        char version[17] = {};
        std::memcpy(version, datos + 0x04, 16);
        if (!std::strstr(version, "INAZUMA_ELEVEN"))
            return nullptr;
        if (std::strchr(version, '3'))
            return &FORMATO_IE3;
        if (std::strchr(version, '2'))
            return &FORMATO_IE2;
        return &FORMATO_IE1;
    }

}
