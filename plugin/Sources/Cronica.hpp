#pragma once

#include <string>

namespace Definitiva {

    // «Crónica de la leyenda»: las partidas de los tres juegos vistas como una sola historia (temporadas,
    // episodio, nivel de equipo, jugadores y horas), leídas de los archivos de partida.
    std::string TextoCronica();

    // Lo que pasó entre un juego y el siguiente, para enseñarlo al traspasar el equipo. `destino` es la carpeta
    // del juego de destino ("inazuma2", "inazuma3", "inazuma3_ogre"); `desdeIE3` distingue el IE3 → Ogro.
    const char *TextoPuente(const char *destino, bool desdeIE3);

}
