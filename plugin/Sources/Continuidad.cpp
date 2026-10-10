#include "Continuidad.hpp"

#include <string>
#include <vector>

#include "Nucleo/DatosDestino.hpp"
#include "Nucleo/Equivalencias.hpp"
#include "Nucleo/Partida.hpp"
#include "Nucleo/Traspaso.hpp"
#include "Partidas.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        const char *gCarpetaCopias = "/Inazuma123Definitiva/partidas";

        struct Destino {
            const char *archivo;
            const char *nombre;
            const FormatoPartida *formato;
            const JuegoDestino *datos;
        };

        constexpr uint32_t OFF_NIVEL_EQUIPO = 0x55;

        void Aviso(const std::string &texto) { MessageBox("Continuidad", texto)(); }

        // Espacio del IE1 de origen: el jugador elige entre los que tienen partida válida.
        int ElegirEspacio(const std::vector<u8> &datos, const FormatoPartida &formato) {
            std::vector<std::string> opciones;
            std::vector<int> espacios;
            for (uint32_t k = 0; k < formato.numEspacios; ++k) {
                Partida p;
                if (!p.Cargar(datos, &formato, k) || !p.Ocupado() || p.Validar() != Error::Ninguno)
                    continue;
                uint32_t base = k * formato.EspacioLargo();
                opciones.push_back(Utils::Format("Espacio %lu · nivel de equipo %u · %u jugadores",
                                                 static_cast<unsigned long>(k + 1), datos[base + OFF_NIVEL_EQUIPO],
                                                 p.NumJugadores()));
                espacios.push_back(static_cast<int>(k));
            }
            if (espacios.empty())
                return -1;
            if (espacios.size() == 1)
                return espacios[0];
            Keyboard kb("¿De qué partida traes tu equipo?", opciones);
            int eleccion = kb.Open();
            return eleccion < 0 ? -2 : espacios[eleccion];
        }

        bool ElegirModo(Opciones &op) {
            Keyboard kb("¿Cómo llegan tus jugadores?\n\n"
                        "Completa: cada uno con su nivel.\n"
                        "Equilibrada: nadie pasa de un nivel máximo.\n"
                        "Solo fichajes: todos al mismo nivel.",
                        {"Completa", "Equilibrada", "Solo fichajes"});
            int eleccion = kb.Open();
            if (eleccion < 0)
                return false;
            op.modo = eleccion == 0 ? Modo::Completa : eleccion == 1 ? Modo::Equilibrada : Modo::SoloFichajes;
            if (op.modo == Modo::Completa)
                return true;
            Keyboard teclado(op.modo == Modo::Equilibrada ? "Nivel máximo (1-99):" : "Nivel de todos (1-99):");
            teclado.IsHexadecimal(false);
            u8 nivel = 20;
            if (teclado.Open(nivel, nivel) < 0 || nivel < 1 || nivel > 99)
                return false;
            (op.modo == Modo::Equilibrada ? op.nivelTope : op.nivelFichaje) = nivel;
            return true;
        }

        void Traer(const char *archivoOrigen, const FormatoPartida &formatoOrigen, const char *nombreOrigen,
                   const Destino &destino, const Mapa &mapa) {
            std::vector<u8> bruto;
            if (!LeerPartida(archivoOrigen, bruto)) {
                Aviso(Utils::Format("No hay partida de %s.", nombreOrigen));
                return;
            }
            int espacio = ElegirEspacio(bruto, formatoOrigen);
            if (espacio == -1) {
                Aviso(Utils::Format("La partida de %s está vacía o dañada.", nombreOrigen));
                return;
            }
            if (espacio < 0)
                return;
            Partida origen;
            origen.Cargar(bruto, &formatoOrigen, static_cast<uint32_t>(espacio));

            std::vector<u8> brutoDestino;
            Partida dest;
            if (!LeerPartida(destino.archivo, brutoDestino) || !dest.Cargar(brutoDestino, destino.formato, 0)
                || !dest.Ocupado() || dest.Validar() != Error::Ninguno) {
                Aviso(Utils::Format("Antes de traer tu equipo, empieza %s y guarda una vez.", destino.nombre));
                return;
            }

            Opciones op;
            if (!ElegirModo(op))
                return;
            destino.datos->Preparar(op);

            std::string copia;
            if (CopiarPartidasASD(gCarpetaCopias, copia) <= 0) {
                Aviso("No se ha podido hacer la copia de seguridad en la SD. No se ha tocado nada.");
                return;
            }
            Informe informe = Traspasar(origen, dest, mapa, op);
            if (informe.error != Error::Ninguno) {
                Aviso(DescribirError(informe.error));
                return;
            }
            if (dest.Sellar() != Error::Ninguno || !EscribirPartida(destino.archivo, dest.Datos())) {
                Aviso(Utils::Format("No se ha podido guardar. Tu partida sigue como estaba (copia en %s).",
                                    copia.c_str()));
                return;
            }
            Aviso(Utils::Format("Han llegado %u jugadores nuevos y %u ya conocidos han mejorado.\n"
                                "Copia de seguridad: %s\n\n"
                                "Para verlos, sal al título de %s SIN GUARDAR y pulsa Continuar.",
                                static_cast<unsigned>(informe.traspasados.size()),
                                static_cast<unsigned>(informe.mejorados.size()), copia.c_str(), destino.nombre));
        }

        const Destino IE2_FUEGO = {"/ina2F_save.dat", "Inazuma Eleven 2: Tormenta de Fuego", &FORMATO_IE2, &DESTINO_IE2};
        const Destino IE2_VENTISCA = {"/ina2B_save.dat", "Inazuma Eleven 2: Ventisca Eterna", &FORMATO_IE2, &DESTINO_IE2};

        void TraerIE1aFuego(MenuEntry *) { Traer("/ina1_save.dat", FORMATO_IE1, "Inazuma Eleven", IE2_FUEGO, MAPA_IE1_IE2); }
        void TraerIE1aVentisca(MenuEntry *) {
            Traer("/ina1_save.dat", FORMATO_IE1, "Inazuma Eleven", IE2_VENTISCA, MAPA_IE1_IE2);
        }

    }

    MenuFolder *CrearMenuContinuidad(const char *carpetaCopias) {
        gCarpetaCopias = carpetaCopias;
        MenuFolder *carpeta = new MenuFolder("Continuidad", "Tu equipo pasa de un juego al siguiente.");
        *carpeta += new MenuEntry("Traer mi equipo del IE1 a Tormenta de Fuego", nullptr, TraerIE1aFuego,
                                  "Tus fichajes del Inazuma Eleven llegan al IE2 con su nivel y técnicas.\n"
                                  "Hazlo desde el menú de la recopilación o vuelve luego al título.");
        *carpeta += new MenuEntry("Traer mi equipo del IE1 a Ventisca Eterna", nullptr, TraerIE1aVentisca,
                                  "Tus fichajes del Inazuma Eleven llegan al IE2 con su nivel y técnicas.");
        return carpeta;
    }

}
