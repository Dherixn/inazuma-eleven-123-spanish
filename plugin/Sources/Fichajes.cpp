#include "Fichajes.hpp"

#include <string>
#include <vector>

#include "Nucleo/Texto.hpp"
#include "Nucleo/DatosDestino.hpp"
#include "Nucleo/Partida.hpp"
#include "Nucleo/Traspaso.hpp"
#include "Partidas.hpp"
#include "TablasJuego.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        const char *gCarpetaCopias = "/Inazuma123Definitiva/partidas";

        struct Juego {
            const char *archivo;
            const char *nombre;
            const FormatoPartida *formato;
            const char *tablas;
            const PerfilJuego *perfil;
            bool esIE2;
        };

        const Juego JUEGOS[] = {
            {"/ina2F_save.dat", "Tormenta de Fuego", &FORMATO_IE2, "inazuma2", &PERFIL_IE2, true},
            {"/ina2B_save.dat", "Ventisca Eterna", &FORMATO_IE2, "inazuma2", &PERFIL_IE2, true},
            {"/ina3S_save.dat", "Rayo Celeste", &FORMATO_IE3, "inazuma3", &PERFIL_IE3, false},
            {"/ina3B_save.dat", "Fuego Explosivo", &FORMATO_IE3, "inazuma3", &PERFIL_IE3, false},
            {"/ina3O_save.dat", "La amenaza del Ogro", &FORMATO_IE3, "inazuma3_ogre", &PERFIL_IE3, false},
        };

        // Aiden Froste: su ficha es la única jugable con la cara 0x51B (ver tools/src/ie123kit/definitiva/aiden.py).
        constexpr uint16_t CARA_AIDEN = 0x051B;
        const uint16_t SHAWN[] = {2329, 2337, 2338};  // defensa, delantero y delantero completo (IE2 e IE3)
        constexpr uint16_t AULLIDO_DE_LOBO = 0x114;

        void Aviso(const std::string &texto) { MessageBox("Fichajes especiales", Ajustar(texto))(); }

        int NumeroDeAiden(const JuegoDatos &tablas) {
            for (size_t i = 0; i < tablas.Fichas(); ++i)
                if (tablas.Cara(i) == CARA_AIDEN && tablas.Jugable(i))
                    return tablas.Numero(i);
            return -1;
        }

        bool EsShawn(uint16_t numero) {
            for (uint16_t s : SHAWN)
                if (s == numero)
                    return true;
            return false;
        }

        void FicharAiden(MenuEntry *) {
            // Partidas donde se puede: las que existen y están bien.
            std::vector<const Juego *> hay;
            std::vector<std::string> nombres;
            for (const Juego &j : JUEGOS) {
                std::vector<u8> datos;
                Partida p;
                if (LeerPartida(j.archivo, datos) && p.Cargar(datos, j.formato, 0) && p.Ocupado()
                    && p.Validar() == Error::Ninguno) {
                    hay.push_back(&j);
                    nombres.push_back(j.nombre);
                }
            }
            if (hay.empty()) {
                Aviso("Aiden llega en el IE2 o el IE3: empieza uno de ellos y guarda una vez.");
                return;
            }
            // CTRPF parte las líneas por letras: los saltos van a mano, a 46 caracteres como mucho.
            Keyboard kb("Aiden Froste\n\n"
                        "El hermano de Shawn, la voz que le pedía\n"
                        "marcar siempre. Cuando Shawn deja de\n"
                        "esconderse tras él (aprende «Aullido de\n"
                        "lobo»), Aiden juega por fin a su lado.\n\n"
                        "¿En qué partida?",
                        nombres);
            int eleccion = kb.Open();
            if (eleccion < 0)
                return;
            const Juego &juego = *hay[eleccion];

            const JuegoDatos *tablas = TablasDe(juego.tablas, *juego.perfil);
            int numero = tablas ? NumeroDeAiden(*tablas) : -1;
            if (numero < 0) {
                Aviso("Este archive.fa no tiene a Aiden: falta la capa de datos de la edición definitiva.");
                return;
            }
            std::vector<u8> datos;
            Partida partida;
            LeerPartida(juego.archivo, datos);
            partida.Cargar(datos, juego.formato, 0);

            // Shawn tiene que estar en el equipo; en el IE2, además, haber superado lo de su hermano.
            int shawn = -1;
            bool superado = !juego.esIE2;
            for (uint32_t k = 0; k < partida.NumJugadores(); ++k) {
                JugadorPartida j = partida.Jugador(k);
                if (!EsShawn(j.id))
                    continue;
                if (shawn < 0 || j.nivel > partida.Jugador(shawn).nivel)
                    shawn = static_cast<int>(k);
                for (uint32_t t = 0; t < NUM_TECNICAS; ++t)
                    if (j.tecnicas[t] == AULLIDO_DE_LOBO)
                        superado = true;
            }
            if (shawn < 0) {
                Aviso("Aiden solo vendrá con su hermano: ficha antes a Shawn Froste.");
                return;
            }
            if (!superado) {
                Aviso("Aiden todavía vive dentro de Shawn.\n\n"
                      "Cuando Shawn aprenda «Aullido de lobo» y\n"
                      "juegue siendo él mismo, Aiden podrá por fin\n"
                      "salir al campo a su lado.");
                return;
            }
            JugadorPartida hermano = partida.Jugador(static_cast<uint32_t>(shawn));

            Opciones op;
            tablas->Preparar(op);
            std::string copia;
            if (CopiarPartidasASD(gCarpetaCopias, copia) <= 0) {
                Aviso("No se ha podido hacer la copia de seguridad en la SD. No se ha tocado nada.");
                return;
            }
            Fichaje r = Fichar(partida, static_cast<uint16_t>(numero), hermano.nivel, op, &hermano.crudo);
            if (r == Fichaje::YaEsta) {
                Aviso("Aiden ya está en tu equipo.");
                return;
            }
            if (r != Fichaje::Hecho) {
                Aviso(r == Fichaje::SinSitio ? "No cabe nadie más en el equipo." : "No se ha podido fichar a Aiden.");
                return;
            }
            if (partida.Sellar() != Error::Ninguno || !EscribirPartida(juego.archivo, partida.Datos())) {
                Aviso(Utils::Format("No se ha podido guardar. Tu partida sigue como estaba (copia en %s).", copia.c_str()));
                return;
            }
            Aviso(Utils::Format("«Hermano, esta vez jugamos los dos.»\n\n"
                                "Aiden Froste se une al equipo con el\n"
                                "nivel de Shawn (%u).\n"
                                "Copia de seguridad en la SD:\n%s\n\n"
                                "Para verlo, sal al título de %s\n"
                                "SIN GUARDAR y pulsa Continuar.",
                                hermano.nivel, copia.c_str(), juego.nombre));
        }

    }

    MenuFolder *CrearMenuFichajes(const char *carpetaCopias) {
        gCarpetaCopias = carpetaCopias;
        MenuFolder *carpeta = new MenuFolder("Fichajes especiales", "Jugadores que el juego original no deja fichar.");
        *carpeta += new MenuEntry("Aiden Froste", nullptr, FicharAiden,
                                  "El hermano de Shawn, delantero de Viento con\n"
                                  "«Ventisca eterna».\n\n"
                                  "IE2: cuando Shawn aprende «Aullido de lobo».\n"
                                  "IE3 y Ogro: si Shawn está en el equipo.\n"
                                  "Si lo fichas en el IE2, pasa al IE3 con la\n"
                                  "Continuidad.");
        return carpeta;
    }

}
