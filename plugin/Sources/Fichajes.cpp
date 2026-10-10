#include "Fichajes.hpp"

#include <string>
#include <vector>

#include <algorithm>
#include <map>

#include "Nucleo/NombreSjis.hpp"
#include "Nucleo/Texto.hpp"
#include "Nucleo/DatosDestino.hpp"
#include "Nucleo/Partida.hpp"
#include "Nucleo/Traspaso.hpp"
#include "Partidas.hpp"
#include "TablasJuego.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        // Dónde queda el fichado: en el grupo (los que juegan) o en la reserva.
        const char *DondeVa(const Partida &partida, uint16_t numero) {
            return partida.EnGrupo(numero) ? "Entra en el grupo (página 1 de Equipo)."
                                           : "Está en la reserva (página 2 de Equipo).";
        }

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

        // Sí o no, con las opciones en español (las de MessageBox salen en inglés).
        bool Preguntar(const std::string &texto) {
            Keyboard teclado(Ajustar(texto), {"Sí", "No"});
            return teclado.Open() == 0;
        }

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

        // --- Fichaje libre: cualquier jugador del juego (también los que solo daban Wi-Fi o StreetPass) ---

        struct Elegida {
            const char *archivo;
            const char *nombre;
            const FormatoPartida *formato;
            const char *tablas;
            const PerfilJuego *perfil;
            bool esIE3;
            u32 espacio;
        };

        const char *POSICION[] = {"?", "?", "PR", "?", "DF", "?", "MD", "?", "DL"};

        // Partida y espacio donde fichar; false si no hay o se cancela. Solo las de formato verificado para
        // escribir (el del IE1 aún no lo está: no se ofrece).
        bool ElegirPartida(Elegida &e, std::vector<u8> &datos) {
            const Elegida CANDIDATAS[] = {
                {"/ina1_save.dat", "Inazuma Eleven", &FORMATO_IE1, "inazuma1", &PERFIL_IE1, false, 0},
                {"/ina2F_save.dat", "Tormenta de Fuego", &FORMATO_IE2, "inazuma2", &PERFIL_IE2, false, 0},
                {"/ina2B_save.dat", "Ventisca Eterna", &FORMATO_IE2, "inazuma2", &PERFIL_IE2, false, 0},
                {"/ina3S_save.dat", "Rayo Celeste", &FORMATO_IE3, "inazuma3", &PERFIL_IE3, true, 0},
                {"/ina3B_save.dat", "Fuego Explosivo", &FORMATO_IE3, "inazuma3", &PERFIL_IE3, true, 0},
                {"/ina3O_save.dat", "La amenaza del Ogro", &FORMATO_IE3, "inazuma3_ogre", &PERFIL_IE3, true, 0},
            };
            std::vector<Elegida> hay;
            std::vector<std::string> nombres;
            for (const Elegida &c : CANDIDATAS) {
                std::vector<u8> d;
                if (!c.formato->verificado || !LeerPartida(c.archivo, d))
                    continue;
                for (u32 k = 0; k < c.formato->numEspacios; ++k) {
                    Partida p;
                    if (!p.Cargar(d, c.formato, k) || !p.Ocupado() || p.Validar() != Error::Ninguno)
                        continue;
                    Elegida x = c;
                    x.espacio = k;
                    hay.push_back(x);
                    nombres.push_back(c.formato->numEspacios > 1 ? Utils::Format("%s (espacio %lu)", c.nombre,
                                                                                 static_cast<unsigned long>(k + 1))
                                                                  : std::string(c.nombre));
                }
            }
            if (hay.empty()) {
                Aviso("No hay ninguna partida guardada todavía.");
                return false;
            }
            Keyboard kb("¿En qué partida?", nombres);
            int r = kb.Open();
            if (r < 0)
                return false;
            e = hay[static_cast<size_t>(r)];
            return LeerPartida(e.archivo, datos);
        }

        void FichajeLibre(MenuEntry *) {
            if (!Preguntar("Fichaje libre\n\nEs una ayuda: deja fichar a cualquier jugador del juego, también los que "
                           "solo se conseguían por Wi-Fi o StreetPass, y hace el juego más fácil. ¿Seguir?"))
                return;
            Elegida e;
            std::vector<u8> datos;
            if (!ElegirPartida(e, datos))
                return;
            const JuegoDatos *t = TablasDe(e.tablas, *e.perfil);
            // Los nombres del IE1 y el IE2 llevan glifos de dos letras de la traducción: se enseña el del mismo
            // personaje (misma cara) en el IE3, que va en letras normales.
            const JuegoDatos *ie3 = e.esIE3 ? t : TablasDe("inazuma3", PERFIL_IE3);
            if (!t || !ie3) {
                Aviso("No se han podido leer las tablas del juego (archive.fa).");
                return;
            }
            std::map<uint16_t, size_t> caraIE3;
            for (size_t i = 0; i < ie3->Fichas(); ++i)
                if (ie3->Jugable(i) && !caraIE3.count(ie3->Cara(i)))
                    caraIE3[ie3->Cara(i)] = i;
            auto nombre = [&](size_t i) {
                if (!e.esIE3) {
                    auto it = caraIE3.find(t->Cara(i));
                    if (it != caraIE3.end())
                        return DecodificarNombre(ie3->NombreCrudo(it->second), ie3->AnchoNombre());
                }
                return DecodificarNombre(t->NombreCrudo(i), t->AnchoNombre());
            };

            Keyboard teclado(Ajustar("Escribe parte del nombre (por ejemplo, «Axel» o «Byron»):"));
            std::string buscado;
            if (teclado.Open(buscado) < 0 || buscado.empty())
                return;
            std::string clave = NormalizarBusqueda(buscado);
            // Una ficha por personaje: la que la historia une al equipo si la hay (así no se duplica cuando lo
            // fiche), si no la primera.
            const std::vector<uint16_t> *altas = AltasDe(e.tablas);
            auto historia = [altas](uint16_t n) {
                return altas && std::binary_search(altas->begin(), altas->end(), n);
            };
            std::vector<size_t> fichas;
            std::vector<std::string> opciones;
            std::map<uint32_t, size_t> vistos;  // personaje -> posición en `fichas`
            for (size_t i = 0; i < t->Fichas(); ++i) {
                if (!t->Jugable(i) || t->Ficha(t->Numero(i)) != static_cast<int>(i))
                    continue;
                std::string n = nombre(i);
                if (NormalizarBusqueda(n).find(clave) == std::string::npos)
                    continue;
                std::string opcion = Utils::Format("%s (%s)", n.c_str(), POSICION[t->Posicion(i) < 9 ? t->Posicion(i) : 0]);
                auto visto = vistos.find(t->Clave(t->Numero(i)));
                if (visto != vistos.end()) {
                    if (historia(t->Numero(i)) && !historia(t->Numero(fichas[visto->second]))) {
                        fichas[visto->second] = i;
                        opciones[visto->second] = opcion;
                    }
                    continue;
                }
                if (opciones.size() == 30)
                    continue;  // se sigue mirando por si llega la ficha de la historia de uno ya listado
                vistos[t->Clave(t->Numero(i))] = fichas.size();
                fichas.push_back(i);
                opciones.push_back(opcion);
            }
            if (opciones.empty()) {
                Aviso("No hay ningún jugador con ese nombre en este juego.");
                return;
            }
            Keyboard lista(opciones.size() == 30 ? "Hay más: afina la búsqueda. ¿A quién fichas?" : "¿A quién fichas?",
                           opciones);
            int r = lista.Open();
            if (r < 0)
                return;
            size_t ficha = fichas[static_cast<size_t>(r)];

            Partida partida;
            partida.Cargar(datos, e.formato, e.espacio);
            u32 suma = 0;
            u8 mejor = 1;
            for (u32 k = 0; k < partida.NumJugadores(); ++k) {
                u8 n = partida.Jugador(k).nivel;
                suma += n;
                mejor = n > mejor ? n : mejor;
            }
            u8 media = static_cast<u8>(partida.NumJugadores() ? (suma + partida.NumJugadores() / 2) / partida.NumJugadores() : 1);
            if (media < 1)
                media = 1;
            Keyboard nivelLista("¿Con qué nivel llega?", {Utils::Format("La media de tu equipo (%u)", media),
                                                         Utils::Format("El de tu mejor jugador (%u)", mejor),
                                                         "Otro..."});
            int eleccion = nivelLista.Open();
            if (eleccion < 0)
                return;
            u8 nivel = eleccion == 0 ? media : mejor;
            if (eleccion == 2) {
                Keyboard nivelTeclado("Nivel (1-99):");
                nivelTeclado.IsHexadecimal(false);
                if (nivelTeclado.Open(nivel, media) < 0 || nivel < 1 || nivel > 99)
                    return;
            }

            Opciones op;
            t->Preparar(op);
            std::string copia;
            if (CopiarPartidasASD(gCarpetaCopias, copia) <= 0) {
                Aviso("No se ha podido hacer la copia de seguridad en la SD. No se ha tocado nada.");
                return;
            }
            Fichaje f = Fichar(partida, t->Numero(ficha), nivel, op);
            if (f == Fichaje::YaEsta) {
                Aviso("Ya está en tu equipo.");
                return;
            }
            if (f != Fichaje::Hecho) {
                Aviso(f == Fichaje::SinSitio ? "No cabe nadie más en el equipo." : "No se ha podido fichar.");
                return;
            }
            if (partida.Sellar() != Error::Ninguno || !EscribirPartida(e.archivo, partida.Datos())) {
                Aviso(Utils::Format("No se ha podido guardar. Tu partida sigue como estaba (copia en %s).", copia.c_str()));
                return;
            }
            Aviso(Utils::Format("%s se une al equipo con nivel %u.\n%s\nCopia de seguridad en la SD: %s\n\n"
                                "Para verlo, sal al título de %s SIN GUARDAR y pulsa Continuar.",
                                nombre(ficha).c_str(), nivel, DondeVa(partida, t->Numero(ficha)), copia.c_str(),
                                e.nombre));
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
                                "nivel de Shawn (%u).\n%s\n"
                                "Copia de seguridad en la SD:\n%s\n\n"
                                "Para verlo, sal al título de %s\n"
                                "SIN GUARDAR y pulsa Continuar.",
                                hermano.nivel, DondeVa(partida, static_cast<uint16_t>(numero)), copia.c_str(),
                                juego.nombre));
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
        *carpeta += new MenuEntry("Fichaje libre (ayuda)", nullptr, FichajeLibre,
                                  Ajustar("Busca por nombre a cualquier jugador del juego y fíchalo, también los que "
                                          "solo se conseguían por Wi-Fi o StreetPass. Opcional: hace el juego más "
                                          "fácil."));
        return carpeta;
    }

}
