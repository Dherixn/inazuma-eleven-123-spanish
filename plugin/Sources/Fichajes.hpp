#pragma once

#include <CTRPluginFramework.hpp>

namespace Definitiva {

    // Menú «Fichajes especiales»: jugadores que el juego no deja fichar (de momento, Aiden Froste).
    // Necesitan la capa de datos de la edición definitiva en archive.fa (python -m ie123kit.definitiva construir).
    CTRPluginFramework::MenuFolder *CrearMenuFichajes(const char *carpetaCopias);

    // Secreto del Cuaderno del abuelo («CANON»): Canon Evans, el bisnieto de Mark, llega del futuro a la partida
    // que se elija (IE2, IE3 u Ogro) con el nivel de Mark.
    void FicharACanon();

}
