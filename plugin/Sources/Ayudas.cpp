#include "Ayudas.hpp"

#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <3ds.h>

#include "Nucleo/TablaIni.hpp"

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
            const char *clave;
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
            };
            return lista;
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
                if (valor == NO_TOCAR || (std::strcmp(a.id, "version") == 0 && EsOgro(tabla, entradas)))
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
        void Bucle(void *) {
            for (u32 vuelta = 0;; ++vuelta) {
                svcSleepThread(1000000LL);
                for (u32 k = 0; k < numVigiladas; ++k) {
                    u32 tabla = vigiladas[k];
                    if (!Legible(tabla, 2 * INI_ENTRADA))
                        continue;
                    u32 cabe = (ZONA_TABLAS + ZONA_LARGO - tabla) / INI_ENTRADA;
                    u32 entradas = ContarEntradas(reinterpret_cast<const u8 *>(tabla), cabe);
                    if (EsTablaIni(reinterpret_cast<const u8 *>(tabla), entradas) && Legible(tabla, entradas * INI_ENTRADA))
                        Aplicar(reinterpret_cast<u8 *>(tabla), entradas);
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
                Keyboard teclado(std::string(a.titulo) + "\n\n" + a.nota, opciones);
                int r = teclado.Open();
                if (r < 0 || r == a.elegido)
                    return;
                a.elegido = r;
                entrada->Name() = Nombre(a);
                if (!Guardar())
                    MessageBox("Ayudas", "No se ha podido guardar el ajuste en la SD; vale hasta que apagues.")();
                else if (SeLeeAlCargar(a))
                    MessageBox("Ayudas", std::string("Hecho.") + CUANDO_CARGA)();
                return;
            }
        }

    }

    void IniciarAyudas() {
        Cargar();
        threadCreate(Bucle, nullptr, 0x1000, 0x1A, -2, true);
    }

    MenuFolder *CrearMenuAyudas() {
        MenuFolder *carpeta = new MenuFolder("Ayudas", "Opcionales y reversibles: de fábrica, todo como el juego original.\n"
                                                       "Se guardan en la SD (/Inazuma123Definitiva/ajustes.txt).");
        for (Ayuda &a : Lista()) {
            std::string nota = a.nota;
            if (SeLeeAlCargar(a))
                nota += CUANDO_CARGA;
            a.entrada = new MenuEntry(Nombre(a), nullptr, Elegir, nota);
            *carpeta += a.entrada;
        }
        return carpeta;
    }

}
