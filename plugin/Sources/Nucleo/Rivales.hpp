#pragma once

#include <cstdint>

namespace Definitiva {

    // Nivel de los rivales. Cada partido (de la historia, duelo o encuentro por el mapa) es un registro de 48 B de
    // logic/games.dat: equipo rival (u16) en +0, 1 en +2 si el partido lleva nivel y el nivel del rival en +3. Cada
    // jugador del rival suma a ese nivel su ajuste de team.pkb (byte +4 de su hueco; de 0 a 40). Medido en Azahar:
    // con el +3 de un duelo del IE2 a 77, sus cuatro rivales salen a nivel 77. Ver docs/definitiva/HALLAZGOS_3DS.md.
    // Gemela: tools/src/ie123kit/definitiva/rivales.py.
    constexpr uint32_t PARTIDO_PASO = 48;
    constexpr uint32_t OFF_PARTIDO_EQUIPO = 0, OFF_PARTIDO_CON_NIVEL = 2, OFF_PARTIDO_NIVEL = 3;
    constexpr uint8_t NIVEL_MAXIMO = 99;

    enum ModoRivales : int { RIVALES_ORIGINAL = 0, RIVALES_FACILES = 1, RIVALES_DUROS = 2, RIVALES_MUY_DUROS = 3,
                             RIVALES_A_TU_NIVEL = 4 };

    // Nivel del rival para el modo. `extra` es el ajuste más alto de sus jugadores: con él, nadie pasa de 99 (salvo
    // que el original ya pase). `nivelEquipo` (media de tus 11 mejores) solo cuenta en «a tu nivel»; 0 = no se sabe.
    uint8_t NivelRival(uint8_t original, int modo, int nivelEquipo, uint8_t extra);

    // ¿Lleva nivel este partido? (equipo, +2 a 1 y un nivel de 1 a 99; el IE1 tiene alguno a 254 y 255: no se tocan.)
    bool PartidoConNivel(const uint8_t *registro);

    // Copia en `destino` los niveles de `original` (games.dat entero, `largo` bytes) ajustados al modo; el resto de
    // `destino` no se toca. `extraPorEquipo[equipo]` (hasta `equipos`) es el ajuste más alto de cada equipo; puede ser
    // nullptr. Devuelve cuántos partidos quedan con un nivel distinto del original.
    uint32_t AjustarPartidos(const uint8_t *original, uint8_t *destino, uint32_t largo, int modo, int nivelEquipo,
                             const uint8_t *extraPorEquipo, uint32_t equipos);

    // ¿Es `memoria` la tabla `original` salvo los niveles? (Antes de escribir en ella: el juego la recarga en otro
    // sitio al cambiar de zona y donde estaba queda otra cosa.)
    bool MismaTablaDePartidos(const uint8_t *memoria, const uint8_t *original, uint32_t largo);

    // Ajuste más alto de cada equipo de team.pkb (registros de `paso` B: 320 en IE1 e IE2, 352 en IE3 y Ogro; número
    // de equipo en +32 y 16 huecos de 8 B desde +64, con el jugador en +0 y su ajuste en +4). Escribe hasta `equipos`
    // entradas en `salida` (a 0 las que no aparecen).
    void ExtraDeEquipos(const uint8_t *teamPkb, uint32_t largo, uint32_t paso, uint8_t *salida, uint32_t equipos);

}
