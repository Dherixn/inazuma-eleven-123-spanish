#pragma once

#include <CTRPluginFramework.hpp>

namespace Definitiva {

    // Easter eggs de la edición definitiva (nada cambia el juego si no se buscan):
    // - El código Konami (↑↑↓↓←→←→ B A, con la cruceta) despierta el «Cuaderno del abuelo», una carpeta del
    //   menú que estaba oculta.
    // - En el cuaderno: páginas de Daisuke Evans, créditos y «Escribir en el cuaderno», con palabras secretas.
    // - Mensajes en fechas señaladas.
    CTRPluginFramework::MenuFolder *CrearCuaderno();

    // Oculta el cuaderno si aún no se ha encontrado (llamar después de añadirlo al menú) y vigila el código en
    // cada fotograma del juego (OSD::Run: OnNewFrame solo corre con el menú abierto).
    void IniciarSecretos();

    // Lo que no se puede hacer dentro de un fotograma (escribir en la SD, avisos): lo llama el hilo de las
    // ayudas unas diez veces por segundo.
    void SecretosPendientes();

}
