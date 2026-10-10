#pragma once

#include "PlantillaEnJuego.hpp"

namespace Definitiva {

    // Mantiene el nivel de los rivales (modos de Nucleo/Rivales.hpp) en la copia de logic/games.dat que el juego tiene
    // en memoria: la busca a trozos, la comprueba entera antes de escribir (solo se escriben los niveles) y vuelve a
    // ponerlos cuando el juego la recarga, que lo hace al cambiar de zona y a veces en otro sitio. Con el modo
    // original deja la tabla como venía. Cuenta desde el siguiente partido: el juego lee el nivel al prepararlo.
    // Se llama cada milisegundo desde el hilo de las ayudas; `nivelEquipo` es la media de tus 11 mejores (0 si no se
    // sabe) y solo cuenta en «a tu nivel».
    void CuidarRivales(JuegoActivo juego, int modo, int nivelEquipo);

}
