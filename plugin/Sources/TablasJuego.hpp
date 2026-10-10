#pragma once

#include <string>

#include "Nucleo/DatosDestino.hpp"

namespace Definitiva {

    // Tablas de un juego («inazuma1», «inazuma2», «inazuma3», «inazuma3_ogre») leídas de archive.fa la
    // primera vez que se piden. Se lee la copia del mod en la SD (LayeredFS de Luma3DS) si existe y, si
    // no, la RomFS del juego (en Azahar ya incluye la carpeta de mods). nullptr si no se pueden leer.
    const JuegoDatos *TablasDe(const std::string &juego, const PerfilJuego &perfil);

}
