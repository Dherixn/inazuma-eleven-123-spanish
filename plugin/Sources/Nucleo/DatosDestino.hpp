#pragma once

// Datos de los juegos de destino del traspaso (IE2 e IE3), generados desde sus tablas en
// DatosDestino.inc: por número de jugador, su curva de EXP, la clave de su personaje y sus técnicas
// por nivel. Gemelo de ie123kit.definitiva.datos_juego.

#include <cstddef>
#include <cstdint>

#include "Traspaso.hpp"

namespace Definitiva {

    struct FichaDestino {
        uint16_t numero;
        uint8_t curva;
        uint32_t clave;
        uint16_t tec[4];
        uint8_t nv[4];
    };

    struct JuegoDestino {
        const FichaDestino *fichas;
        size_t cantidad;
        const uint32_t (*curvas)[98];
        size_t numCurvas;

        const FichaDestino *Buscar(uint16_t numero) const;
        uint32_t ExpMinima(uint16_t numero, uint8_t nivel) const;
        int Aprendidas(uint16_t numero, uint8_t nivel, uint16_t salida[4]) const;
        uint32_t Clave(uint16_t numero) const;
        // Rellena las funciones del traspaso con este juego como destino.
        void Preparar(Opciones &op) const;
    };

    extern const JuegoDestino DESTINO_IE2;
    extern const JuegoDestino DESTINO_IE3;

}
