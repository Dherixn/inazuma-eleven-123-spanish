#include "Continuidad.hpp"

#include <string>
#include <vector>

#include "Nucleo/Texto.hpp"
#include "Nucleo/DatosDestino.hpp"
#include "Nucleo/Partida.hpp"
#include "Nucleo/Traspaso.hpp"
#include "Partidas.hpp"
#include "Cronica.hpp"
#include "TablasJuego.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        const char *gCarpetaCopias = "/Inazuma123Definitiva/partidas";

        // `juego` es la carpeta de sus tablas en archive.fa (Ventisca y Fuego Explosivo comparten las de
        // su juego: sus carpetas propias solo cambian INI, vídeos, imágenes y algún script).
        struct Origen {
            const char *archivo;
            const char *nombre;
            const FormatoPartida *formato;
            const char *juego;
            const PerfilJuego *perfil;
        };

        struct Destino {
            const char *archivo;
            const char *nombre;
            const FormatoPartida *formato;
            const char *juego;
            const PerfilJuego *perfil;
        };

        constexpr uint32_t OFF_NIVEL_EQUIPO = 0x55;

        void Aviso(const std::string &texto) { MessageBox("Continuidad", Ajustar(texto))(); }

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

        // De entre los orígenes posibles (Fuego o Ventisca), los que tienen partida; si hay dos, se pregunta.
        const Origen *ElegirOrigen(const std::vector<Origen> &origenes, std::vector<u8> &bruto) {
            std::vector<const Origen *> hay;
            std::vector<std::string> nombres;
            for (const Origen &o : origenes) {
                std::vector<u8> datos;
                if (LeerPartida(o.archivo, datos)) {
                    hay.push_back(&o);
                    nombres.push_back(o.nombre);
                }
            }
            if (hay.empty())
                return nullptr;
            size_t k = 0;
            if (hay.size() > 1) {
                Keyboard kb("¿De qué juego traes tu equipo?", nombres);
                int eleccion = kb.Open();
                if (eleccion < 0)
                    return nullptr;
                k = static_cast<size_t>(eleccion);
            }
            LeerPartida(hay[k]->archivo, bruto);
            return hay[k];
        }

        void Traer(const std::vector<Origen> &origenes, const Destino &destino) {
            std::vector<u8> bruto;
            const Origen *elegido = ElegirOrigen(origenes, bruto);
            if (!elegido) {
                Aviso(Utils::Format("No hay partida de %s.", origenes[0].nombre));
                return;
            }
            const FormatoPartida &formatoOrigen = *elegido->formato;
            const char *nombreOrigen = elegido->nombre;
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

            const JuegoDatos *tablasOrigen = TablasDe(elegido->juego, *elegido->perfil);
            const JuegoDatos *tablasDestino = TablasDe(destino.juego, *destino.perfil);
            if (!tablasOrigen || !tablasDestino) {
                Aviso("No se han podido leer las tablas del juego (archive.fa).");
                return;
            }
            std::vector<Par> pares = EmparejarPorCara(*tablasOrigen, *tablasDestino);
            Mapa mapa = {pares.data(), pares.size()};

            Opciones op;
            if (!ElegirModo(op))
                return;
            tablasDestino->Preparar(op);

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
            const char *puente = TextoPuente(destino.juego, std::string(elegido->juego) == "inazuma3");
            if (*puente)
                MessageBox("Capítulo siguiente", Ajustar(puente))();
            Aviso(Utils::Format("Han llegado %u jugadores nuevos (%u al grupo; el resto, a la reserva) y %u ya\n"
                                "conocidos han mejorado.\n"
                                "Copia de seguridad: %s\n\n"
                                "Para verlos, sal al título de %s SIN GUARDAR y pulsa Continuar.",
                                static_cast<unsigned>(informe.traspasados.size()),
                                static_cast<unsigned>(informe.alGrupo.size()),
                                static_cast<unsigned>(informe.mejorados.size()), copia.c_str(), destino.nombre));
        }

        // Nombres de archivo sacados del código de la recopilación (code.bin descomprimido).
        const std::vector<Origen> DESDE_IE1 = {{"/ina1_save.dat", "Inazuma Eleven", &FORMATO_IE1, "inazuma1", &PERFIL_IE1}};
        const std::vector<Origen> DESDE_IE2 = {
            {"/ina2F_save.dat", "Tormenta de Fuego", &FORMATO_IE2, "inazuma2", &PERFIL_IE2},
            {"/ina2B_save.dat", "Ventisca Eterna", &FORMATO_IE2, "inazuma2", &PERFIL_IE2}};
        const std::vector<Origen> DESDE_IE3 = {
            {"/ina3S_save.dat", "Rayo Celeste", &FORMATO_IE3, "inazuma3", &PERFIL_IE3},
            {"/ina3B_save.dat", "Fuego Explosivo", &FORMATO_IE3, "inazuma3", &PERFIL_IE3}};
        const Destino IE2_FUEGO = {"/ina2F_save.dat", "Inazuma Eleven 2: Tormenta de Fuego", &FORMATO_IE2, "inazuma2",
                                   &PERFIL_IE2};
        const Destino IE2_VENTISCA = {"/ina2B_save.dat", "Inazuma Eleven 2: Ventisca Eterna", &FORMATO_IE2, "inazuma2",
                                      &PERFIL_IE2};
        const Destino IE3_RAYO = {"/ina3S_save.dat", "Inazuma Eleven 3: Rayo Celeste", &FORMATO_IE3, "inazuma3", &PERFIL_IE3};
        const Destino IE3_FUEGO = {"/ina3B_save.dat", "Inazuma Eleven 3: Fuego Explosivo", &FORMATO_IE3, "inazuma3",
                                   &PERFIL_IE3};
        const Destino IE3_OGRO = {"/ina3O_save.dat", "Inazuma Eleven 3: La amenaza del Ogro", &FORMATO_IE3, "inazuma3_ogre",
                                  &PERFIL_IE3};

        void TraerIE1aFuego(MenuEntry *) { Traer(DESDE_IE1, IE2_FUEGO); }
        void TraerIE1aVentisca(MenuEntry *) { Traer(DESDE_IE1, IE2_VENTISCA); }
        void TraerIE2aRayo(MenuEntry *) { Traer(DESDE_IE2, IE3_RAYO); }
        void TraerIE2aFuegoExplosivo(MenuEntry *) { Traer(DESDE_IE2, IE3_FUEGO); }
        void TraerIE2aOgro(MenuEntry *) { Traer(DESDE_IE2, IE3_OGRO); }
        void TraerIE3aOgro(MenuEntry *) { Traer(DESDE_IE3, IE3_OGRO); }

        void VerCronica(MenuEntry *) { MessageBox("Crónica de la leyenda", Ajustar(TextoCronica()))(); }

    }

    MenuFolder *CrearMenuContinuidad(const char *carpetaCopias) {
        gCarpetaCopias = carpetaCopias;
        MenuFolder *carpeta = new MenuFolder("Continuidad", "Tu equipo pasa de un juego al siguiente.");
        *carpeta += new MenuEntry("Crónica de la leyenda", nullptr, VerCronica,
                                  Ajustar("Tus partidas de los tres juegos como una sola historia: temporadas, "
                                          "nivel del equipo, jugadores y horas."));
        *carpeta += new MenuEntry("Traer mi equipo del IE1 a Tormenta de Fuego", nullptr, TraerIE1aFuego,
                                  "Tus fichajes del Inazuma Eleven llegan al IE2 con su nivel y técnicas.\n"
                                  "Hazlo desde el menú de la recopilación o vuelve luego al título.");
        *carpeta += new MenuEntry("Traer mi equipo del IE1 a Ventisca Eterna", nullptr, TraerIE1aVentisca,
                                  "Tus fichajes del Inazuma Eleven llegan al IE2 con su nivel y técnicas.");
        *carpeta += new MenuEntry("Traer mi equipo del IE2 a Rayo Celeste", nullptr, TraerIE2aRayo,
                                  "Tus fichajes del IE2 (Fuego o Ventisca) llegan al IE3.");
        *carpeta += new MenuEntry("Traer mi equipo del IE2 a Fuego Explosivo", nullptr, TraerIE2aFuegoExplosivo,
                                  "Tus fichajes del IE2 (Fuego o Ventisca) llegan al IE3.");
        *carpeta += new MenuEntry("Traer mi equipo del IE2 a La amenaza del Ogro", nullptr, TraerIE2aOgro,
                                  "Tus fichajes del IE2 (Fuego o Ventisca) llegan al IE3.");
        *carpeta += new MenuEntry("Traer mi equipo del IE3 a La amenaza del Ogro", nullptr, TraerIE3aOgro,
                                  "El capítulo final: tu Inazuma Japón de Rayo Celeste o Fuego Explosivo, con su\n"
                                  "nivel y técnicas, llega a La amenaza del Ogro para plantar cara al Ogro.");
        return carpeta;
    }

}
