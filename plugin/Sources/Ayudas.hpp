#pragma once

#include <CTRPluginFramework.hpp>

namespace Definitiva {

    // Lee los ajustes guardados en la SD y arranca el hilo que los aplica a INAZUMA.INI en memoria.
    void IniciarAyudas();

    // Menú «Ayudas»: todas opcionales; de fábrica, todo como el juego original.
    CTRPluginFramework::MenuFolder *CrearMenuAyudas();

}
