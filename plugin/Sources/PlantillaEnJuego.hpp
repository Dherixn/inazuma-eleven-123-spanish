#pragma once

#include <cstdint>
#include <vector>

#include "Nucleo/MemoriaEquipo.hpp"

namespace Definitiva {

    enum class JuegoActivo { Ninguno, IE1, IE2, IE3, Ogro };

    // Busca la plantilla del juego que está corriendo y le aplica las ayudas de jugadores: PE y PT siempre llenos
    // y la EXP ganada multiplicada (factor 1 = como el original). Con `recordar`, la memoria del equipo de esa
    // partida (Nucleo/MemoriaEquipo.hpp): apunta lo que llega a tener cada jugador y, en IE2, IE3 y Ogro, devuelve
    // nivel, EXP y técnicas a quien llega con la historia tras un traspaso o vuelve con menos nivel del que tuvo.
    // Se llama unas diez veces por segundo desde el hilo de las ayudas; las búsquedas van a trozos para no frenar
    // el juego.
    void CuidarPlantilla(JuegoActivo juego, bool llenarPePt, int factorExp, bool recordar);

    struct TecnicasPerdidas {
        uint16_t numero;
        std::vector<uint16_t> tecnicas;
    };

    // Jugadores de la plantilla actual que han perdido técnicas que sabían (según lo apuntado).
    std::vector<TecnicasPerdidas> BuscarTecnicasPerdidas();

    // Devuelve técnicas a un jugador de la plantilla, en sus huecos libres. Cuántas se pusieron; -1 si no está.
    int DevolverTecnicas(uint16_t numero, const std::vector<uint16_t> &tecnicas);

    // El juego que reconoció el hilo de las ayudas la última vez.
    JuegoActivo JuegoActual();

    // Memoria del equipo de un archivo de partida (Partida::Identificador; `tiempo`, su tiempo de juego: si la de
    // la SD es de una partida anterior sobre el mismo archivo, se aparta). Si es la que usa el hilo de las ayudas,
    // esa misma. Para el traspaso y «Recuperar jugadores», desde el menú.
    MemoriaEquipo LeerMemoriaEquipo(uint32_t partida, uint32_t tiempo);
    bool EscribirMemoriaEquipo(const MemoriaEquipo &m);

    // Dirección de la pasión (u32; detrás va la amistad) del juego que corre. En el IE1 y el IE2 está a una
    // distancia fija de la plantilla (los códigos de Sharkive y Azahar dan lo mismo: -0xCE4 y -0xF0C); en el
    // IE3 y Ogro no, y entonces es 0, igual que sin partida cargada. Si hace falta, busca la plantilla de una
    // vez (unos segundos); llamar desde el menú.
    uint32_t DireccionPuntos();

}
