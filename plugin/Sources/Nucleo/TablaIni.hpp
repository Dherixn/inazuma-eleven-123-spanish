#pragma once

#include <cstddef>
#include <cstdint>

namespace Definitiva {

    // INAZUMA.INI tal como lo deja el juego en memoria al cargarlo: entradas de 0x40 bytes, clave de 32 y
    // valor de 32, texto con ceros detrás. GetIniInt busca la clave y hace atoi del valor en cada llamada,
    // así que cambiar el texto basta para todo lo que el juego lea después.
    constexpr uint32_t INI_ENTRADA = 0x40;
    constexpr uint32_t INI_TEXTO = 0x20;
    constexpr uint32_t INI_MAX_ENTRADAS = 160;

    // ¿Empieza aquí una tabla de INAZUMA.INI? En los seis juegos la primera clave es FARCLIP_LENGTH y la
    // segunda MDL_SCALE_CHAR.
    bool EsTablaIni(const uint8_t *tabla, uint32_t entradas);

    // Cuántas entradas seguidas tienen pinta de clave (mayúsculas, cifras y «_» hasta un cero), hasta `max`.
    uint32_t ContarEntradas(const uint8_t *tabla, uint32_t max);

    // Índice de la clave en la tabla, o -1.
    int BuscarClave(const uint8_t *tabla, uint32_t entradas, const char *clave);

    // Valor actual de una entrada (atoi, como el juego).
    int32_t LeerValor(const uint8_t *entrada);

    // Escribe el valor en texto si la entrada no lo tiene ya; true si ha escrito.
    bool PonerValor(uint8_t *entrada, int32_t valor);

    // Busca tablas en una zona (desplazamientos múltiplos de 0x40); devuelve cuántas ha encontrado.
    uint32_t BuscarTablas(const uint8_t *zona, size_t largo, uint32_t *salida, uint32_t max);

}
