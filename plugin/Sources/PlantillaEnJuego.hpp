#pragma once

namespace Definitiva {

    enum class JuegoActivo { Ninguno, IE1, IE2, IE3, Ogro };

    // Busca la plantilla del juego que está corriendo y le aplica las ayudas de jugadores: PE y PT siempre llenos
    // y la EXP ganada multiplicada (factor 1 = como el original). Se llama unas diez veces por segundo desde el
    // hilo de las ayudas; la búsqueda va a trozos para no frenar el juego.
    void CuidarPlantilla(JuegoActivo juego, bool llenarPePt, int factorExp);

}
