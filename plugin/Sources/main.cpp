// Plugin de la edición definitiva de Inazuma Eleven 1·2·3 (ver docs/definitiva/).
//
// Fase actual: menú propio y copia de seguridad de las partidas. Las ayudas que escriben en la
// memoria del juego y el traspaso se añaden cuando sus direcciones estén comprobadas en la ROM
// (incidencias #1, #2 y #7). Hasta entonces el plugin no toca la memoria del juego.

#include <3ds.h>

#include <CTRPluginFramework.hpp>

#include "Partidas.hpp"

namespace CTRPluginFramework {

    namespace {

        constexpr u64 TITULO_RECOPILACION = 0x00040000000BB800ULL;
        const char *CARPETA_COPIAS = "/Inazuma123Definitiva/partidas";

        void CopiarPartidas(MenuEntry *) {
            std::string destino;
            int copiados = Definitiva::CopiarPartidasASD(CARPETA_COPIAS, destino);
            if (copiados < 0)
                MessageBox("Copia de seguridad", "No se ha podido abrir la partida del juego.")();
            else
                MessageBox("Copia de seguridad",
                           Utils::Format("%d archivo(s) copiado(s) en\n%s", copiados, destino.c_str()))();
        }

        void VerPartidas(MenuEntry *) {
            std::vector<Definitiva::ArchivoPartida> archivos;
            if (!Definitiva::ListarPartidas(archivos)) {
                MessageBox("Partidas", "No se ha podido abrir la partida del juego.")();
                return;
            }
            std::string texto;
            for (const auto &a : archivos)
                texto += Utils::Format("%s  (%llu B)\n", a.nombre.c_str(), a.tamano);
            MessageBox("Partidas", texto.empty() ? "No hay archivos de partida." : texto)();
        }

    }

    void PatchProcess(FwkSettings &) {}

    void OnProcessExit(void) {}

    int main(void) {
        PluginMenu *menu = new PluginMenu("Inazuma Eleven 1·2·3 — Edición definitiva", 0, 1, 0,
                                          "Continuidad entre los tres juegos y ayudas opcionales.\n"
                                          "Proyecto de la traducción al español.");
        menu->SynchronizeWithFrame(true);

        if (Process::GetTitleID() != TITULO_RECOPILACION) {
            MessageBox("Edición definitiva", "Este plugin es para Inazuma Eleven 1·2·3 (00040000000BB800).")();
        }

        MenuFolder *partidas = new MenuFolder("Partidas", "Copias de seguridad antes de un traspaso.");
        *partidas += new MenuEntry("Ver archivos de partida", nullptr, VerPartidas,
                                   "Lista los archivos de partida de los tres juegos.");
        *partidas += new MenuEntry("Copiar partidas a la SD", nullptr, CopiarPartidas,
                                   "Copia las partidas en /Inazuma123Definitiva/partidas/<fecha>/ de la SD.");
        *menu += partidas;

        menu->Run();
        delete menu;
        return 0;
    }

}
