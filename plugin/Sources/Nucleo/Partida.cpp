#include "Partida.hpp"

#include <cstring>

namespace Definitiva {

    // IE1: 32 fichas de 0x60 B desde 0x79C, tres espacios (medido en Azahar); solo se lee.
    namespace {
        // Medido en las partidas (2026-10-10), ver tools/src/ie123kit/definitiva/partida.py.
        const Reinicio REINICIO_IE2[] = {{0x57, 0}, {0x58, 0}, {0x5B, 0x10}, {0x5D, 0}};
        const Reinicio REINICIO_IE3[] = {{0x4B, 1}, {0x57, 0}, {0x58, 0}, {0x5A, 0}, {0x5B, 0x10}};
    }

    const FormatoPartida FORMATO_IE1 = {"ie1.123", 0x54C0, 0x79C, 0x60, false, 32, 3, 0x3E, true, 0, true, nullptr, 0};
    const FormatoPartida FORMATO_IE2 = {"ie2.123", 0x7DC0, 0x1268, 0x70, true, MAX_JUGADORES, 1, 0x4A, false,
                                        0x1268 + MAX_JUGADORES * 0x70 + 0x40, true, REINICIO_IE2, 4};
    const FormatoPartida FORMATO_IE3 = {"ie3.123", 0x5940, 0x11B0, 0x6C, true, MAX_JUGADORES, 1, 0x4A, false,
                                        0x11B0 + MAX_JUGADORES * 0x6C + 0x5E, false, REINICIO_IE3, 5};

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

    JugadorPartida LeerJugador(const uint8_t *reg, const FormatoPartida &formato) {
        JugadorPartida j;
        j.exp = U32(reg + 0x00);
        j.id = U16(reg + 0x04);
        for (uint32_t k = 0; k < NUM_TECNICAS; ++k) {
            j.tecnicas[k] = formato.tecnicasU8 ? reg[0x38 + k] : U16(reg + 0x38 + 2 * k);
            j.nivelesTecnica[k] = formato.tecnicasU8 ? 0 : reg[0x44 + k];
        }
        j.nivel = reg[formato.offNivel];
        j.crudo.assign(reg, reg + formato.jugadorPaso);
        return j;
    }

    std::vector<uint8_t> EscribirJugador(const JugadorPartida &j, const FormatoPartida &formato) {
        std::vector<uint8_t> reg = j.crudo;
        PonU32(&reg[0x00], j.exp);
        PonU16(&reg[0x04], j.id);
        for (uint32_t k = 0; k < NUM_TECNICAS; ++k) {
            if (formato.tecnicasU8) {
                reg[0x38 + k] = static_cast<uint8_t>(j.tecnicas[k] & 0xFF);
            } else {
                PonU16(&reg[0x38 + 2 * k], j.tecnicas[k]);
                reg[0x44 + k] = j.nivelesTecnica[k];
            }
        }
        reg[formato.offNivel] = j.nivel;
        return reg;
    }

    bool Partida::Cargar(std::vector<uint8_t> datos, const FormatoPartida *formato, uint32_t espacio) {
        if (!formato || espacio >= formato->numEspacios)
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
        return LeerJugador(&datos_[OffJugador(indice)], *formato_);
    }

    Error Partida::EscribirReg(uint32_t indice, const JugadorPartida &j) {
        std::vector<uint8_t> reg = EscribirJugador(j, *formato_);
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

    void NormalizarFichaNueva(std::vector<uint8_t> &crudo, const FormatoPartida &formato) {
        for (uint32_t k = 0; k < formato.numReinicio; ++k)
            if (formato.reinicio[k].offset < crudo.size())
                crudo[formato.reinicio[k].offset] = formato.reinicio[k].valor;
    }

    bool Partida::MeterEnGrupo(uint32_t indice) {
        if (!formato_->offGrupo || indice >= NumJugadores())
            return false;
        const uint32_t lista = base_ + formato_->offGrupo;
        SembrarGrupo();
        const uint32_t oJugador = OffJugador(indice);
        const uint16_t numero = U16(&datos_[oJugador + 4]);
        int hueco = -1;
        uint32_t miembros = 0;
        for (uint32_t k = 0; k < MAX_GRUPO; ++k) {
            uint16_t n = U16(&datos_[lista + 2 * k]);
            if (n == numero)
                return true;
            if (n)
                ++miembros;
            else if (hueco < 0)
                hueco = static_cast<int>(k);
        }
        if (hueco < 0)
            return false;
        bool usado[256] = {};
        for (uint32_t k = 0; k < NumJugadores(); ++k) {
            uint32_t o = OffJugador(k);
            uint16_t n = U16(&datos_[o + 4]);
            for (uint32_t m = 0; n && m < MAX_GRUPO; ++m)
                if (U16(&datos_[lista + 2 * m]) == n) {
                    usado[datos_[o + OFF_DORSAL]] = true;
                    break;
                }
        }
        uint32_t dorsal = 1;
        while (dorsal < 255 && usado[dorsal])
            ++dorsal;
        ++miembros;
        PonU16(&datos_[lista + 2 * static_cast<uint32_t>(hueco)], numero);
        datos_[oJugador + OFF_DORSAL] = static_cast<uint8_t>(dorsal);
        if (formato_->sitioAlEntrar)
            datos_[oJugador + OFF_SITIO] = static_cast<uint8_t>(miembros);
        datos_[base_ + OFF_NUM_GRUPO] = static_cast<uint8_t>(miembros);
        return true;
    }

    void Partida::SembrarGrupo() {
        const uint32_t lista = base_ + formato_->offGrupo;
        for (uint32_t k = 0; k < MAX_GRUPO; ++k)
            if (U16(&datos_[lista + 2 * k]))
                return;
        // Lista vacía con grupo (IE3 tras el prólogo): el grupo son los primeros registros. Se escriben antes de
        // tocarla para no sacar a nadie.
        uint32_t cuantos = datos_[base_ + OFF_NUM_GRUPO];
        if (cuantos > MAX_GRUPO)
            cuantos = MAX_GRUPO;
        if (cuantos > NumJugadores())
            cuantos = NumJugadores();
        for (uint32_t k = 0; k < cuantos; ++k)
            PonU16(&datos_[lista + 2 * k], U16(&datos_[OffJugador(k) + 4]));
    }

    std::vector<uint16_t> Partida::Grupo() const {
        std::vector<uint16_t> salida;
        if (!formato_->offGrupo)
            return salida;
        for (uint32_t k = 0; k < MAX_GRUPO; ++k)
            if (uint16_t n = U16(&datos_[base_ + formato_->offGrupo + 2 * k]))
                salida.push_back(n);
        if (salida.empty()) {
            uint32_t cuantos = datos_[base_ + OFF_NUM_GRUPO];
            for (uint32_t k = 0; k < cuantos && k < MAX_GRUPO && k < NumJugadores(); ++k)
                salida.push_back(U16(&datos_[OffJugador(k) + 4]));
        }
        return salida;
    }

    bool Partida::SacarDelGrupo(uint32_t indice) {
        if (!formato_->offGrupo || indice >= NumJugadores())
            return false;
        SembrarGrupo();
        const uint32_t lista = base_ + formato_->offGrupo;
        const uint32_t oJugador = OffJugador(indice);
        const uint16_t numero = U16(&datos_[oJugador + 4]);
        uint32_t k = 0;
        while (k < MAX_GRUPO && U16(&datos_[lista + 2 * k]) != numero)
            ++k;
        if (k == MAX_GRUPO)
            return false;
        for (; k + 1 < MAX_GRUPO; ++k)
            PonU16(&datos_[lista + 2 * k], U16(&datos_[lista + 2 * (k + 1)]));
        PonU16(&datos_[lista + 2 * (MAX_GRUPO - 1)], 0);
        uint32_t miembros = 0;
        for (uint32_t m = 0; m < MAX_GRUPO; ++m)
            if (U16(&datos_[lista + 2 * m]))
                ++miembros;
        datos_[oJugador + OFF_DORSAL] = 0;
        datos_[oJugador + OFF_SITIO] = 0;
        datos_[base_ + OFF_NUM_GRUPO] = static_cast<uint8_t>(miembros);
        return true;
    }

    bool Partida::EnGrupo(uint16_t numero) const {
        if (!formato_->offGrupo || !numero)
            return false;
        for (uint32_t k = 0; k < MAX_GRUPO; ++k)
            if (U16(&datos_[base_ + formato_->offGrupo + 2 * k]) == numero)
                return true;
        return false;
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
