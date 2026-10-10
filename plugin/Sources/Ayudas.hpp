#pragma once

#include <CTRPluginFramework.hpp>

namespace Definitiva {

    // Lee los ajustes guardados en la SD y arranca el hilo que los aplica a INAZUMA.INI en memoria.
    void IniciarAyudas();

    // ¿Está activa la «Memoria del equipo»? Sin ella, quien llega con la historia tras un traspaso llega sin lo suyo.
    bool MemoriaDelEquipoActiva();

    // Menú «Ayudas»: todas opcionales; de fábrica, todo como el juego original salvo la memoria del equipo.
    CTRPluginFramework::MenuFolder *CrearMenuAyudas();

}
