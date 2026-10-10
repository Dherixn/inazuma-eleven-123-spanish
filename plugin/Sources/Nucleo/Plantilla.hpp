#pragma once

// La plantilla del juego en memoria (no la del archivo de partida): un registro por jugador con la EXP, el
// número, el nivel y, al lado, los PE y PT actuales y máximos. Su dirección cambia de una sesión a otra (la
// traducción cambia el tamaño de lo que se carga antes), así que se busca por su forma: registros seguidos
// con números de jugadores de verdad, nivel 1..99 y una EXP que llega al menos a la de su nivel.
//
// Medido en Azahar (los códigos de Sharkive para este juego dan las mismas distancias):
//   IE1: registros cada 0x188, nivel en +0x3E, PE/PT en +0xAC del registro.
//   IE2: registros cada 0xFC,  nivel en +0x4A, PE/PT en +0x70 del registro.
//   IE3 y Ogro: registros cada 0x6C (los de la partida, seguidos), nivel en +0x4A; PE/PT aparte, en
//   inicio + 0xCA6C + i * 0x98.
// Los PE y PT actuales van en el propio registro, +0x34 (dos u16); en «PE/PT» de arriba están los máximos
// (PE, PT y otro par que no se toca). Comprobado en el IE2 (Mark: actuales 37/41 en +0x34, máximos 75/85 en
// +0x70, como enseña el menú) y en el IE3 (cambiar el primer par cambia el máximo que se ve).

#include <cstddef>
#include <cstdint>
#include <map>

#include "DatosDestino.hpp"

namespace Definitiva {

    constexpr uint32_t OFF_PE_PT_ACTUALES = 0x34;  // en el registro, en los tres juegos

    struct GeometriaPlantilla {
        uint32_t paso;
        uint32_t offNivel;
        uint32_t maxJugadores;
        uint32_t offPePt;   // máximos de PE y PT, desde el comienzo de la plantilla
        uint32_t pasoPePt;
        // Byte que el juego pone a 1 en las fichas de los rivales de un partido. En el IE2 las deja justo detrás de
        // tu plantilla, en la misma tabla (medido en un duelo: 18 tuyos y detrás los 4 rivales): sin esto, las ayudas
        // los tomaban por jugadores tuyos. 0 = el juego no hace eso.
        uint32_t offRival;
    };

    extern const GeometriaPlantilla GEO_IE1, GEO_IE2, GEO_IE3;

    // Hasta dónde puede llegar la EXP de un jugador de ese nivel para seguir siendo creíble: la de dos niveles
    // más (el juego sube de nivel al acabar el partido; el multiplicador no pasa de aquí).
    uint32_t TopeExp(const JuegoDatos &t, uint16_t numero, uint8_t nivel);

    // ¿Es un registro de jugador plausible para este juego? Número jugable, nivel 1..99 y EXP entre la de su
    // nivel y TopeExp. Sin `estricto` no se mira el tope: para buscar la plantilla hace falta (evita falsos
    // positivos), pero una vez encontrada, un jugador con más EXP de la cuenta (otro truco, una prueba) no
    // debe dejar sin ayudas a los que van detrás.
    bool RegistroValido(const uint8_t *reg, const GeometriaPlantilla &g, const JuegoDatos &t, bool estricto = true);

    // Registros válidos seguidos desde `inicio` (como mucho g.maxJugadores o lo que quepa en `disponible`);
    // se para en el primer número repetido.
    uint32_t ContarRegistros(const uint8_t *inicio, size_t disponible, const GeometriaPlantilla &g, const JuegoDatos &t,
                             bool estricto = true);

    // Primer desplazamiento de la zona donde empieza una plantilla de al menos `minimo` registros (el
    // registro de antes, si cabe en la zona, no vale). -1 si no hay.
    long BuscarPlantilla(const uint8_t *zona, size_t largo, const GeometriaPlantilla &g, const JuegoDatos &t,
                         uint32_t minimo = 4);

    // Sube los PE y PT actuales (dos u16) hasta los máximos (dos u16). Nunca los baja. true si cambió algo.
    bool LlenarPePt(uint8_t *actuales, const uint8_t *maximos);

    // Multiplica la EXP que se gana: recuerda la última EXP de cada jugador (por número) y, si ha subido,
    // suma (factor - 1) veces la subida, sin pasar de `tope`.
    class MultiplicadorExp {
    public:
        void Olvidar() { ultima_.clear(); }
        // Devuelve la EXP que debe quedar.
        uint32_t Aplicar(uint16_t numero, uint32_t exp, int factor, uint32_t tope);

    private:
        std::map<uint16_t, uint32_t> ultima_;
    };

}
