#include "Ayudas.hpp"

#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <3ds.h>

#include "Nucleo/Texto.hpp"
#include "Nucleo/TablaIni.hpp"
#include "PlantillaEnJuego.hpp"
#include "Secretos.hpp"
#include "TablasJuego.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        const char *RUTA_CARPETA = "/Inazuma123Definitiva";
        const char *RUTA_AJUSTES = "/Inazuma123Definitiva/ajustes.txt";

        // El juego deja la tabla del INI al principio del heap lineal (0x14015280 en el IE2, 0x14018E80 en el
        // IE3), siempre en el mismo sitio para cada juego. El objeto que apunta a ella está en los datos de cada
        // módulo (ina_main1/2/3ogre.cro), así que no se usa: se busca la tabla por su contenido.
        constexpr u32 ZONA_TABLAS = 0x14000000;
        constexpr u32 ZONA_CERCA = 0x00040000;
        constexpr u32 ZONA_LARGO = 0x00100000;
        constexpr u32 MAX_VIGILADAS = 4;

        // Nivel que deja el valor que trae el INI de cada juego.
        constexpr s32 NO_TOCAR = INT32_MIN;

        struct Nivel {
            const char *nombre;
            s32 valor;
        };

        struct Ayuda {
            const char *id;
            const char *titulo;
            const char *clave;  // la del INI; nullptr = ayuda sobre la plantilla (PlantillaEnJuego)
            const char *nota;
            std::vector<Nivel> niveles;  // el primero es el valor de fábrica
            int elegido;
            MenuEntry *entrada;
        };

        const char *CUANDO_CARGA =
            "\n\nEl juego lo lee al cargarse: elígelo antes de entrar en un juego, o sal al menú 1·2·3 y vuelve "
            "a entrar.";

        std::vector<Ayuda> &Lista() {
            static std::vector<Ayuda> lista = {
                {"encuentros", "Encuentros aleatorios", "RPG_ENCOUNT_TIME",
                 "Cada cuánto salen partidos al ir por el mapa. Se nota al momento.",
                 {{"Normal", 60}, {"Menos", 150}, {"Muy pocos", 400}, {"Ninguno", 99999}, {"Más (para entrenar)", 25}},
                 0, nullptr},
                {"velocidad", "Velocidad al andar", "RPG_MOVESPEED",
                 "Se aplica al volver al mapa: después de un partido o al cargar la partida.",
                 {{"Normal", 320}, {"Rápida", 448}, {"Muy rápida", 576}}, 0, nullptr},
                {"rival", "Supertécnicas del rival", "CPU_USE_SPECIAL_PERCENT",
                 "Con qué frecuencia usa supertécnicas la CPU (la parte más notable de la dificultad).",
                 {{"Normal", 40}, {"Pocas", 15}, {"Muchas", 70}, {"Siempre", 100}}, 0, nullptr},
                {"venta", "Precio al vender objetos", "ITEM_SELL_PERCENT", "Lo que pagan las tiendas por un objeto.",
                 {{"Normal (la mitad)", 50}, {"Precio completo", 100}}, 0, nullptr},
                {"fueradejuego", "Fuera de juego", "OFFSIDE_EFFECTIVE", "Si se pita el fuera de juego en los partidos.",
                 {{"Se pita", 1}, {"No se pita", 0}}, 0, nullptr},
                // IZ_TYPE decide los exclusivos (equipos de los encuentros, fichajes de la carpeta, entrenamientos
                // y, en el IE2, los scripts de eventos de data_iz_blizzard). El código lo consulta en cada uso,
                // así que con una sola partida se puede jugar el contenido de las dos versiones.
                {"version", "Versión", "IZ_TYPE",
                 "Qué versión manda en los encuentros, fichajes y eventos exclusivos, sea cual sea el juego elegido: "
                 "la partida es la misma. IE2 e IE3 (Ogro no tiene otra versión). Cámbiala cuando quieras para "
                 "tener los exclusivos de las dos.",
                 {{"la del juego elegido", NO_TOCAR}, {"Fuego / Rayo Celeste", 0}, {"Ventisca / Fuego Explosivo", 1}},
                 0, nullptr},
                // Sobre la plantilla en memoria (ver PlantillaEnJuego): menos farmeo, todo opcional.
                {"pept", "PE y PT siempre llenos", nullptr,
                 "Los PE y PT de todos tus jugadores se mantienen al máximo, también en mitad de los partidos: "
                 "las supertécnicas no se agotan y se acabó descansar entre partidos.",
                 {{"No", 0}, {"Sí", 1}}, 0, nullptr},
                {"exp", "Experiencia", nullptr,
                 "Multiplica la EXP que gana cada jugador. El juego sube de nivel al acabar el partido, así que "
                 "parte de lo ganado de más puede notarse en el siguiente.",
                 {{"Normal (x1)", 1}, {"Doble (x2)", 2}, {"Triple (x3)", 3}}, 0, nullptr},
                // No cambia nada del juego: solo apunta en la SD, para «Recuperar técnicas perdidas» (#13).
                {"tecnicas", "Memoria de técnicas", nullptr,
                 "Apunta en la SD las técnicas que llega a saber cada jugador. No cambia nada del juego: sirve para "
                 "devolvérselas con «Recuperar técnicas perdidas» si la historia se las quita (en el IE3, Shawn y "
                 "Jordan vuelven tras el partido contra Corea sin sus técnicas de manual).",
                 {{"Sí", 1}, {"No", 0}}, 0, nullptr},
            };
            return lista;
        }

        s32 Valor(const char *id) {
            for (const Ayuda &a : Lista())
                if (std::strcmp(a.id, id) == 0)
                    return a.niveles[a.elegido].valor;
            return 0;
        }

        bool SeLeeAlCargar(const Ayuda &a) {
            std::string id = a.id;
            return id == "rival" || id == "venta" || id == "fueradejuego" || id == "version";
        }

        // Ogro: sus INI tienen SEARCH_LV191 (como el IE3) pero no BGM_OFF, y no tiene datos de otra versión.
        bool EsOgro(const u8 *tabla, u32 entradas) {
            return BuscarClave(tabla, entradas, "SEARCH_LV191") >= 0 && BuscarClave(tabla, entradas, "BGM_OFF") < 0;
        }

        std::string Nombre(const Ayuda &a) { return std::string(a.titulo) + ": " + a.niveles[a.elegido].nombre; }

        // --- Ajustes en la SD: una línea «id=valor» por ayuda ---

        void Cargar() {
            File f;
            if (File::Open(f, RUTA_AJUSTES, File::READ) != 0)
                return;
            u64 largo = f.GetSize();
            std::string texto(largo > 4096 ? 4096 : static_cast<size_t>(largo), '\0');
            if (!texto.empty() && f.Read(&texto[0], texto.size()) != 0)
                texto.clear();
            f.Close();
            size_t ini = 0;
            while (ini < texto.size()) {
                size_t fin = texto.find('\n', ini);
                std::string linea = texto.substr(ini, fin == std::string::npos ? std::string::npos : fin - ini);
                ini = fin == std::string::npos ? texto.size() : fin + 1;
                size_t igual = linea.find('=');
                if (igual == std::string::npos)
                    continue;
                std::string id = linea.substr(0, igual);
                s32 valor = static_cast<s32>(std::atoi(linea.c_str() + igual + 1));
                for (Ayuda &a : Lista())
                    if (id == a.id)
                        for (size_t n = 0; n < a.niveles.size(); ++n)
                            if (a.niveles[n].valor == valor)
                                a.elegido = static_cast<int>(n);
            }
        }

        bool Guardar() {
            Directory::Create(RUTA_CARPETA);
            File f;
            if (File::Open(f, RUTA_AJUSTES, File::RWC | File::TRUNCATE) != 0)
                return false;
            std::string texto;
            for (const Ayuda &a : Lista())
                texto += Utils::Format("%s=%ld\n", a.id, static_cast<long>(a.niveles[a.elegido].valor));
            bool ok = f.Write(texto.data(), texto.size()) == 0;
            f.Flush();
            f.Close();
            return ok;
        }

        // --- Hilo que mantiene los valores en la tabla ---

        void Aplicar(u8 *tabla, u32 entradas) {
            for (const Ayuda &a : Lista()) {
                s32 valor = a.niveles[a.elegido].valor;
                if (!a.clave || valor == NO_TOCAR || (std::strcmp(a.id, "version") == 0 && EsOgro(tabla, entradas)))
                    continue;
                int i = BuscarClave(tabla, entradas, a.clave);
                if (i >= 0)
                    PonerValor(tabla + i * INI_ENTRADA, valor);
            }
        }

        bool Legible(u32 dir, u32 largo) {
            return Process::CheckAddress(dir) && Process::CheckAddress(dir + largo - 1);
        }

        u32 vigiladas[MAX_VIGILADAS];
        u32 numVigiladas = 0;

        void Vigilar(u32 tabla) {
            for (u32 k = 0; k < numVigiladas; ++k)
                if (vigiladas[k] == tabla)
                    return;
            if (numVigiladas < MAX_VIGILADAS)
                vigiladas[numVigiladas++] = tabla;
            else
                vigiladas[tabla % MAX_VIGILADAS] = tabla;
        }

        void Buscar(u32 largo) {
            if (!Legible(ZONA_TABLAS, largo))
                return;
            u32 halladas[MAX_VIGILADAS];
            u32 n = BuscarTablas(reinterpret_cast<const u8 *>(ZONA_TABLAS), largo, halladas, MAX_VIGILADAS);
            for (u32 k = 0; k < n; ++k)
                Vigilar(ZONA_TABLAS + halladas[k]);
        }

        // Cada milisegundo se repasan las tablas ya vistas: el juego lee los parámetros del partido justo después
        // de cargar el INI (con varias cargas de archivo entre medias), y hay que llegar antes. Al cargar otro
        // juego, su tabla cae en otro sitio: se busca cada 10 ms cerca del principio y cada medio segundo en 1 MB.
        // Qué juego tiene cargada esta tabla: el IE3 trae SEARCH_LV191 y BGM_OFF; Ogro, solo SEARCH_LV191; el
        // IE2, IZ_TYPE; el IE1, ninguna de las tres.
        JuegoActivo Clasificar(const u8 *tabla, u32 entradas) {
            if (BuscarClave(tabla, entradas, "SEARCH_LV191") >= 0)
                return BuscarClave(tabla, entradas, "BGM_OFF") >= 0 ? JuegoActivo::IE3 : JuegoActivo::Ogro;
            return BuscarClave(tabla, entradas, "IZ_TYPE") >= 0 ? JuegoActivo::IE2 : JuegoActivo::IE1;
        }

        // Cada milisegundo se repasan las tablas ya vistas: el juego lee los parámetros del partido justo después
        // de cargar el INI (con varias cargas de archivo entre medias), y hay que llegar antes. Al cargar otro
        // juego, su tabla cae en otro sitio: se busca cada 10 ms cerca del principio y cada medio segundo en 1 MB.
        // Cada 100 ms, las ayudas de la plantilla, con el juego de la tabla viva más reciente.
        void Bucle(void *) {
            JuegoActivo juego = JuegoActivo::Ninguno;
            for (u32 vuelta = 0;; ++vuelta) {
                svcSleepThread(1000000LL);
                bool clasificar = vuelta % 100 == 0;
                JuegoActivo visto = JuegoActivo::Ninguno;
                for (u32 j = numVigiladas; j-- > 0;) {  // de la más reciente a la más antigua
                    u32 tabla = vigiladas[j];
                    if (!Legible(tabla, 2 * INI_ENTRADA))
                        continue;
                    u32 cabe = (ZONA_TABLAS + ZONA_LARGO - tabla) / INI_ENTRADA;
                    u32 entradas = ContarEntradas(reinterpret_cast<const u8 *>(tabla), cabe);
                    if (EsTablaIni(reinterpret_cast<const u8 *>(tabla), entradas) && Legible(tabla, entradas * INI_ENTRADA)) {
                        Aplicar(reinterpret_cast<u8 *>(tabla), entradas);
                        if (clasificar && visto == JuegoActivo::Ninguno)
                            visto = Clasificar(reinterpret_cast<const u8 *>(tabla), entradas);
                    }
                }
                if (clasificar) {
                    juego = visto;
                    CuidarPlantilla(juego, Valor("pept") != 0, static_cast<int>(Valor("exp")), Valor("tecnicas") != 0);
                    SecretosPendientes();
                }
                if (vuelta % 500 == 0)
                    Buscar(ZONA_LARGO);
                else if (vuelta % 10 == 0)
                    Buscar(ZONA_CERCA);
            }
        }

        void Elegir(MenuEntry *entrada) {
            for (Ayuda &a : Lista()) {
                if (a.entrada != entrada)
                    continue;
                std::vector<std::string> opciones;
                for (const Nivel &n : a.niveles)
                    opciones.push_back(n.nombre);
                Keyboard teclado(Ajustar(std::string(a.titulo) + "\n\n" + a.nota), opciones);
                int r = teclado.Open();
                if (r < 0 || r == a.elegido)
                    return;
                a.elegido = r;
                entrada->Name() = Nombre(a);
                if (!Guardar())
                    MessageBox("Ayudas", Ajustar("No se ha podido guardar el ajuste en la SD; vale hasta que apagues."))();
                else if (SeLeeAlCargar(a))
                    MessageBox("Ayudas", Ajustar(std::string("Hecho.") + CUANDO_CARGA))();
                return;
            }
        }

        const char *TablasDelJuego(JuegoActivo j, const PerfilJuego *&perfil) {
            switch (j) {
            case JuegoActivo::IE1: perfil = &PERFIL_IE1; return "inazuma1";
            case JuegoActivo::IE2: perfil = &PERFIL_IE2; return "inazuma2";
            case JuegoActivo::IE3: perfil = &PERFIL_IE3; return "inazuma3";
            case JuegoActivo::Ogro: perfil = &PERFIL_IE3; return "inazuma3_ogre";
            default: perfil = nullptr; return nullptr;
            }
        }

        void RecuperarTecnicas(MenuEntry *) {
            const char *titulo = "Recuperar técnicas";
            if (Valor("tecnicas") == 0) {
                MessageBox(titulo, Ajustar("Activa antes «Memoria de técnicas»: sin ella no se apunta nada."))();
                return;
            }
            const PerfilJuego *perfil = nullptr;
            const char *juego = TablasDelJuego(JuegoActual(), perfil);
            std::vector<TecnicasPerdidas> lista = juego ? BuscarTecnicasPerdidas() : std::vector<TecnicasPerdidas>();
            if (!juego) {
                MessageBox(titulo, Ajustar("Carga tu partida, anda un poco por el mapa y vuelve a probar."))();
                return;
            }
            if (lista.empty()) {
                MessageBox(titulo, Ajustar("Nadie de tu equipo ha perdido técnicas que se hayan apuntado. (Si "
                                           "acabas de cargar la partida, espera unos segundos.)"))();
                return;
            }
            const std::vector<std::string> *nombres = TecnicasDe(juego);
            auto NombreTecnica = [&](u16 id) {
                return nombres && id < nombres->size() && !(*nombres)[id].empty() ? (*nombres)[id]
                                                                                   : Utils::Format("técnica %u", id);
            };
            std::vector<std::string> opciones;
            for (const TecnicasPerdidas &p : lista)
                opciones.push_back(Utils::Format("%s (%u)", NombreDeJugador(juego, *perfil, p.numero).c_str(),
                                                 static_cast<unsigned>(p.tecnicas.size())));
            Keyboard teclado(Ajustar("Jugadores que han perdido técnicas. Elige a quién devolvérselas."), opciones);
            int r = teclado.Open();
            if (r < 0)
                return;
            const TecnicasPerdidas &p = lista[r];
            std::string detalle;
            for (u16 id : p.tecnicas)
                detalle += "\n- " + NombreTecnica(id);
            std::string nombre = NombreDeJugador(juego, *perfil, p.numero);
            if (!MessageBox(titulo, Ajustar(nombre + " sabía:") + detalle + "\n\n" +
                                        Ajustar("Se ponen en sus huecos libres (tiene 6). ¿Devolvérselas?"),
                            DialogType::DialogYesNo)())
                return;
            int puestas = DevolverTecnicas(p.numero, p.tecnicas);
            if (puestas < 0)
                MessageBox(titulo, Ajustar(nombre + " ya no está en la plantilla. Vuelve a abrir esta opción."))();
            else if (puestas == 0)
                MessageBox(titulo, Ajustar(nombre + " no tiene huecos libres: olvida alguna técnica en el juego y "
                                                    "vuelve a probar."))();
            else
                MessageBox(titulo, Ajustar(Utils::Format("%s recupera %d de %u. Guarda la partida en el juego para "
                                                         "conservarlas.",
                                                         nombre.c_str(), puestas,
                                                         static_cast<unsigned>(p.tecnicas.size()))))();
        }

    }

    void IniciarAyudas() {
        Cargar();
        // Pila holgada: el hilo también lee las tablas de archive.fa para reconocer la plantilla.
        threadCreate(Bucle, nullptr, 0x8000, 0x1A, -2, true);
    }

    MenuFolder *CrearMenuAyudas() {
        MenuFolder *carpeta = new MenuFolder("Ayudas", "Opcionales y reversibles: de fábrica, todo como el juego original.\n"
                                                       "Se guardan en la SD (/Inazuma123Definitiva/ajustes.txt).");
        for (Ayuda &a : Lista()) {
            std::string nota = a.nota;
            if (SeLeeAlCargar(a))
                nota += CUANDO_CARGA;
            a.entrada = new MenuEntry(Nombre(a), nullptr, Elegir, Ajustar(nota));
            *carpeta += a.entrada;
        }
        *carpeta += new MenuEntry("Recuperar técnicas perdidas", nullptr, RecuperarTecnicas,
                                  Ajustar("Devuelve a tus jugadores las técnicas que sabían y han perdido (lo que "
                                          "apuntó «Memoria de técnicas»). Lo eliges tú, jugador a jugador."));
        return carpeta;
    }

}
