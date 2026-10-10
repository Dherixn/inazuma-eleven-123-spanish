#pragma once

// Lo que la historia de un juego hace con el equipo, leído de sus scripts de evento (eve.pkh + eve.pkb,
// «PackNum»: cada evento, comprimido en LZ10, es un SSD con su tabla de instrucciones). Gemelo de
// tools/src/ie123kit/definitiva/altas.py. Sin dependencias de la 3DS.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "LectorFA.hpp"

namespace Definitiva {

    // Instrucción que une un jugador al equipo: número, nivel, dorsal... (en el prólogo del IE3,
    // [3170, 30, 10, 0, 1] trae a Paolo Bianchi con nivel 30 y el dorsal 10).
    constexpr uint16_t OP_ALTA = 16446;

    // LZ10 de Nintendo (0x10 + tamaño de 24 bits); si no lleva esa cabecera, se copia tal cual (como en
    // nucleo/compresion/lz10.py). false si el flujo está cortado.
    bool DescomprimirLz10(const uint8_t *datos, size_t largo, std::vector<uint8_t> &salida);

    // Añade a `altas` el primer argumento de cada instrucción OP_ALTA del script SSD.
    void AltasDeScript(const uint8_t *ssd, size_t largo, std::vector<uint16_t> &altas);

    // Altas de todos los eventos de `carpeta` ("inazuma3/data_iz/script"); false si no hay paquete.
    // Lee eve.pkb por ventanas, sin cargarlo entero.
    bool AltasDeEventos(const LectorFA &fa, const std::string &carpeta, std::vector<uint16_t> &altas);

    // Ordena y quita repetidos.
    void Normalizar(std::vector<uint16_t> &numeros);

}
