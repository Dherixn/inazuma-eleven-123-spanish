#pragma once

#include <cstdint>
#include <vector>

namespace Definitiva {

    enum class JuegoActivo { Ninguno, IE1, IE2, IE3, Ogro };

    // Busca la plantilla del juego que está corriendo y le aplica las ayudas de jugadores: PE y PT siempre llenos
    // y la EXP ganada multiplicada (factor 1 = como el original). Con `recordar`, apunta en la SD las técnicas
    // que llega a saber cada jugador (#13). Se llama unas diez veces por segundo desde el hilo de las ayudas; la
    // búsqueda va a trozos para no frenar el juego.
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

    // Dirección de la pasión (u32; detrás va la amistad) del juego que corre. En el IE1 y el IE2 está a una
    // distancia fija de la plantilla (los códigos de Sharkive y Azahar dan lo mismo: -0xCE4 y -0xF0C); en el
    // IE3 y Ogro no, y entonces es 0, igual que sin partida cargada. Si hace falta, busca la plantilla de una
    // vez (unos segundos); llamar desde el menú.
    uint32_t DireccionPuntos();

}
