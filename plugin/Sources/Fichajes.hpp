#pragma once

#include <CTRPluginFramework.hpp>

namespace Definitiva {

    // Menú «Fichajes especiales»: jugadores que el juego no deja fichar (de momento, Aiden Froste).
    // Necesitan la capa de datos de la edición definitiva en archive.fa (python -m ie123kit.definitiva construir).
    CTRPluginFramework::MenuFolder *CrearMenuFichajes(const char *carpetaCopias);

}
