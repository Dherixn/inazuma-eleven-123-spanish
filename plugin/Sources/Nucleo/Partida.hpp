#pragma once

// Partidas guardadas de la recopilación. Gemelo de tools/src/ie123kit/definitiva/partida.py:
// los dos tienen que dar los mismos bytes (plugin/pruebas/comparar.py lo comprueba).
// Sin dependencias de la 3DS: se compila también en el PC para las pruebas.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Definitiva {

    constexpr uint32_t MARCA = 0x19150A04;
    constexpr uint32_t MAX_JUGADORES = 100;
    constexpr uint32_t NUM_TECNICAS = 6;
    constexpr uint32_t OFF_CRC_CABECERA = 0x00;
    constexpr uint32_t OFF_CRC_INFO = 0x40;
    constexpr uint32_t OFF_CRC_DATOS = 0x44;
    constexpr uint32_t INFO_INICIO = 0x44;
    constexpr uint32_t INFO_LARGO = 0x7C;
    constexpr uint32_t DATOS_INICIO = 0xC0;
    constexpr uint32_t OFF_MARCA = 0x48;
    constexpr uint32_t OFF_NUM_JUGADORES = 0x57;
    constexpr uint32_t NUM_ESPACIOS = 3;
    constexpr uint32_t CABECERA_LARGO = 0x40;

    // Cada juego guarda un archivo (ina1_save.dat…) con una cabecera común y tres espacios de
    // 0x80 B de info + el bloque de datos. Los offsets son los del espacio 1.
    struct FormatoPartida {
        const char *nombre;
        uint32_t datosLargo;
        uint32_t jugadoresInicio;
        uint32_t jugadorPaso;
        bool verificado;
        uint32_t maxJugadores;
        uint32_t EspacioLargo() const { return 0x80 + datosLargo; }
        uint32_t ArchivoLargo() const { return CABECERA_LARGO + NUM_ESPACIOS * EspacioLargo(); }
    };

    extern const FormatoPartida FORMATO_IE1;
    extern const FormatoPartida FORMATO_IE2;
    extern const FormatoPartida FORMATO_IE3;

    enum class Error {
        Ninguno,
        PartidaCorta,
        FormatoNoVerificado,
        SumaCabecera,
        SumaInfo,
        SumaDatos,
        SinMarca,
        IndiceFueraDeRango,
        PlantillaLlena,
        TamanoFicha,
        OpcionesIncompletas,
    };

    const char *DescribirError(Error e);

    uint32_t Crc32(const uint8_t *datos, size_t largo);

    struct JugadorPartida {
        uint32_t exp = 0;
        uint16_t id = 0;
        uint16_t tecnicas[NUM_TECNICAS] = {};
        uint8_t nivelesTecnica[NUM_TECNICAS] = {};
        uint8_t nivel = 0;
        std::vector<uint8_t> crudo;
    };

    JugadorPartida LeerJugador(const uint8_t *reg, size_t largo);
    std::vector<uint8_t> EscribirJugador(const JugadorPartida &j);

    class Partida {
    public:
        // false si los datos son más cortos de lo que pide el formato o el espacio no existe.
        bool Cargar(std::vector<uint8_t> datos, const FormatoPartida *formato, uint32_t espacio = 0);
        bool Ocupado() const;

        Error Validar() const;
        // Recalcula las sumas (datos, después info). Error si el formato no está verificado.
        Error Sellar();

        uint8_t NumJugadores() const { return datos_[base_ + OFF_NUM_JUGADORES]; }
        JugadorPartida Jugador(uint32_t indice) const;
        Error PonerJugador(uint32_t indice, const JugadorPartida &j);
        Error AnadirJugador(const JugadorPartida &j, uint32_t *indice);

        const std::vector<uint8_t> &Datos() const { return datos_; }
        const FormatoPartida *Formato() const { return formato_; }

    private:
        uint32_t OffJugador(uint32_t indice) const {
            return base_ + formato_->jugadoresInicio + indice * formato_->jugadorPaso;
        }
        Error EscribirReg(uint32_t indice, const JugadorPartida &j);
        uint32_t LeerU32(uint32_t off) const;
        void EscribirU32(uint32_t off, uint32_t valor);

        std::vector<uint8_t> datos_;
        const FormatoPartida *formato_ = nullptr;
        uint32_t base_ = 0;
    };

    // Formato por la cadena de versión de 0x04; nullptr si no es una partida de la recopilación.
    const FormatoPartida *DetectarFormato(const uint8_t *datos, size_t largo);

}
