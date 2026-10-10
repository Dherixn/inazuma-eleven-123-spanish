#pragma once

#include <string>
#include <vector>

#include "Nucleo/DatosDestino.hpp"

namespace Definitiva {

    // Tablas de un juego («inazuma1», «inazuma2», «inazuma3», «inazuma3_ogre») leídas de archive.fa la
    // primera vez que se piden. Se lee la copia del mod en la SD (LayeredFS de Luma3DS) si existe y, si
    // no, la RomFS del juego (en Azahar ya incluye la carpeta de mods). nullptr si no se pueden leer.
    const JuegoDatos *TablasDe(const std::string &juego, const PerfilJuego &perfil);

    // Nombres de las técnicas del juego por número (command.dat y command.STR). nullptr si no se pueden leer.
    const std::vector<std::string> *TecnicasDe(const std::string &juego);

    // Elemento de cada técnica del IE3 (command.dat +8: 1 Aire, 2 Bosque, 3 Fuego, 4 Montaña; 0 sin elemento). Los
    // números de técnica son los mismos en los tres juegos. nullptr si no se puede leer.
    const std::vector<uint8_t> *ElementosDeTecnicas();

    // Partidos del juego: games.dat (uno cada 48 B; ver Nucleo/Rivales.hpp) y, por número de equipo, el ajuste más
    // alto de sus jugadores en team.pkb. nullptr si no se pueden leer.
    struct Partidos {
        std::vector<uint8_t> tabla;
        std::vector<uint8_t> extra;
    };
    const Partidos *PartidosDe(const std::string &juego);

    // Números que la historia del juego une al equipo (ordenados; ver Nucleo/Eventos.hpp). Lee todos los
    // eventos la primera vez (unos segundos); nullptr si no se pueden leer.
    const std::vector<uint16_t> *AltasDe(const std::string &juego);

    // Nombre legible de un jugador para el menú (en IE1/IE2, el del mismo personaje en el IE3).
    std::string NombreDeJugador(const std::string &juego, const PerfilJuego &perfil, uint16_t numero);

}
