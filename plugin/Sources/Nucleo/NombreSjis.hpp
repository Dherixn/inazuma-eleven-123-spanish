#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace Definitiva {

    // Nombre de una ficha (Shift-JIS de la traducción) en UTF-8 para el menú del plugin: ASCII, letras y cifras
    // de ancho completo, los portadores griegos de los acentos (Α = á, Η = ñ...; ver sjis_portador.py) y algún
    // signo. Lo demás (kana, kanji, los glifos de dos letras del IE1/IE2) sale como «·».
    std::string DecodificarNombre(const uint8_t *campo, size_t ancho);

    // Para buscar: minúsculas y sin acentos (á → a, ñ → n...).
    std::string NormalizarBusqueda(const std::string &utf8);

}
