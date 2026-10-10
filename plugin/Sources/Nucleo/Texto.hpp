#pragma once

#include <cstddef>
#include <string>

namespace Definitiva {

    // Caracteres por línea que caben en las cajas de CTRPF (pantalla de arriba, su fuente).
    constexpr size_t ANCHO_CAJA = 46;

    // Parte el texto en líneas de como mucho `ancho` caracteres sin cortar palabras (CTRPF corta por letras).
    // Cuenta caracteres UTF-8, no bytes; respeta los saltos que ya trae; una palabra más larga que la línea
    // se queda entera en la suya.
    std::string Ajustar(const std::string &texto, size_t ancho = ANCHO_CAJA);

}
