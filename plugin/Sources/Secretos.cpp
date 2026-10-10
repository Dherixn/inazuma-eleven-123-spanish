#include "Secretos.hpp"

#include <cctype>
#include <ctime>
#include <string>

#include "Fichajes.hpp"
#include "Nucleo/Texto.hpp"
#include "PlantillaEnJuego.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        const char *RUTA_CARPETA = "/Inazuma123Definitiva";
        const char *RUTA_SECRETOS = "/Inazuma123Definitiva/secretos.txt";

        MenuFolder *gCuaderno = nullptr;
        bool gDespierto = false;
        bool gRegaloRaimon = false;

        // ↑ ↑ ↓ ↓ ← → ← → B A, con la cruceta.
        const u32 KONAMI[] = {DPadUp, DPadUp, DPadDown, DPadDown, DPadLeft, DPadRight, DPadLeft, DPadRight, B, A};
        constexpr u32 LARGO_KONAMI = sizeof(KONAMI) / sizeof(KONAMI[0]);
        constexpr u32 TECLAS_KONAMI = DPadUp | DPadDown | DPadLeft | DPadRight | B | A;

        const char *FRASES[] = {
            "«El fútbol no se juega solo con los pies: se juega con el corazón.»",
            "«Si el balón llega, para él. Si no llega, ve a buscarlo.»",
            "«Un portero que no tiene miedo al balón ya ha parado la mitad de los tiros.»",
            "«Entrenar cansa. Perder sin haberlo intentado cansa más.»",
            "«Once amigos que se fían unos de otros valen más que once estrellas.»",
        };
        constexpr u32 NUM_FRASES = sizeof(FRASES) / sizeof(FRASES[0]);

        void Aviso(const std::string &titulo, const std::string &texto) { MessageBox(titulo, Ajustar(texto))(); }

        void Guardar() {
            Directory::Create(RUTA_CARPETA);
            File f;
            if (File::Open(f, RUTA_SECRETOS, File::RWC | File::TRUNCATE) != 0)
                return;
            std::string texto = Utils::Format("cuaderno=%d\nregalo_raimon=%d\n", gDespierto ? 1 : 0, gRegaloRaimon ? 1 : 0);
            f.Write(texto.data(), texto.size());
            f.Flush();
            f.Close();
        }

        void Cargar() {
            File f;
            if (File::Open(f, RUTA_SECRETOS, File::READ) != 0)
                return;
            char texto[128] = {};
            u64 largo = f.GetSize();
            f.Read(texto, largo < sizeof(texto) - 1 ? static_cast<u32>(largo) : sizeof(texto) - 1);
            f.Close();
            std::string s = texto;
            gDespierto = s.find("cuaderno=1") != std::string::npos;
            gRegaloRaimon = s.find("regalo_raimon=1") != std::string::npos;
        }

        std::string Frase() { return FRASES[static_cast<u32>(time(nullptr)) % NUM_FRASES]; }

        void Despertar() {
            if (gDespierto) {
                OSD::Notify(Frase());
                return;
            }
            gDespierto = true;
            Guardar();
            if (gCuaderno)
                gCuaderno->Show();
            OSD::Notify("¡Has despertado el Cuaderno del abuelo!");
            OSD::Notify("Ábrelo en el menú del plugin (Select).");
        }

        std::string Normalizar(const std::string &texto) {
            std::string s;
            for (unsigned char c : texto)
                if (std::isalnum(c))
                    s += static_cast<char>(std::toupper(c));
            return s;
        }

        // --- Páginas ---

        void PaginaFutbol(MenuEntry *) {
            Aviso("Cuaderno del abuelo",
                  std::string("Página arrugada, escrita a lápiz:\n\n") + FRASES[0] +
                      "\n\nDebajo, con otra letra: «Mark, cuida de tus amigos.»");
        }

        void PaginaContinuidad(MenuEntry *) {
            Aviso("Cuaderno del abuelo",
                  "Tres temporadas, un solo equipo.\n\n"
                  "Quien juega desde el principio llega al final con los suyos: lo que entrenes en el primer año "
                  "te acompaña hasta el último partido (menú Continuidad).");
        }

        void PaginaHermanos(MenuEntry *) {
            Aviso("Cuaderno del abuelo",
                  "Sobre los hermanos del norte:\n\n"
                  "«Uno defiende, el otro ataca. Mientras el mayor no se acepte, el pequeño no podrá jugar a su "
                  "lado. El aullido del lobo lo cambia todo.»");
        }

        void PaginaFinal(MenuEntry *) {
            Aviso("Cuaderno del abuelo",
                  "La última página tiene una mancha de tinta con forma de ogro.\n\n"
                  "«Cuando el mundo creas que está ganado, vendrá alguien de muy lejos. Lleva contigo a los de "
                  "siempre.»");
        }

        void Creditos(MenuEntry *) {
            Aviso("Créditos",
                  "Inazuma Eleven 1·2·3 — Edición definitiva.\n\n"
                  "Hecha sobre la traducción al español de luishidalgoa y su equipo: sin su trabajo no habría nada "
                  "que ampliar. Gracias a todos los que la han probado y a ti, por jugar.\n\n"
                  "Inazuma Eleven es de LEVEL-5. Proyecto de fans, sin ánimo de lucro.");
        }

        void Escribir(MenuEntry *) {
            Keyboard teclado(Ajustar("Escribe una palabra en el cuaderno del abuelo:"));
            std::string palabra;
            if (teclado.Open(palabra) < 0 || palabra.empty())
                return;
            std::string p = Normalizar(palabra);
            if (p == "RAIMON") {
                if (gRegaloRaimon) {
                    Aviso("Cuaderno", "«El club ya te dio las gracias. Ahora, ¡a entrenar!»");
                    return;
                }
                u32 puntos = DireccionPuntos();
                if (!puntos) {
                    Aviso("Cuaderno", "«Vuelve con tu equipo del Raimon, con la partida cargada.»");
                    return;
                }
                u32 *valores = reinterpret_cast<u32 *>(puntos);
                valores[0] += 500;  // pasión
                valores[1] += 50;   // amistad
                OlvidarPuntos();    // que el multiplicador de Pasión y Amistad no lo cuente como ganado
                gRegaloRaimon = true;
                Guardar();
                Aviso("Cuaderno", "Entre las páginas había un sobre del club de fútbol del Raimon:\n\n"
                                  "+500 de Pasión y +50 de Amistad. ¡Gracias por no rendirte nunca!");
            } else if (p == "AIDEN" || p == "ATSUYA") {
                Aviso("Cuaderno", "Una letra distinta, a toda prisa:\n\n«Hermano, ¿jugamos los dos? Yo marco, tú defiendes.»");
            } else if (p == "MANOCELESTIAL" || p == "GODHAND") {
                Aviso("Cuaderno", "Un dibujo de una mano enorme y dorada, con flechas por todas partes.\n\n"
                                  "«No se para con la mano. Se para con todo el cuerpo... y con el corazón.»");
            } else if (p == "DAISUKE" || p == "ABUELO") {
                Aviso("Cuaderno", "«Si estás leyendo esto, es que te gusta el fútbol tanto como a mí. Me alegro.»");
            } else if (p == "OGRO" || p == "OGRE") {
                Aviso("Cuaderno", "La página está rasgada. Solo se lee: «...del futuro... 80 años... no os rindáis...»");
            } else if (p == "LEYENDA") {
                Aviso("Cuaderno", "La última hoja, que antes estaba en blanco, tiene ahora una sola línea:\n\n"
                                  "«Tres temporadas, un mismo equipo, una sola leyenda. Gracias por llegar hasta aquí.»\n\n"
                                  "Debajo, la firma de Daisuke... y otra más pequeña: «Mark».");
            } else if (p == "CANON") {
                FicharACanon();
            } else if (p == "LUIS") {
                Aviso("Cuaderno", "«Gracias por traducirlo todo. Sin ti, esto seguiría en japonés.»");
            } else if (p == "INAZUMA" || p == "INAZUMAELEVEN") {
                Aviso("Cuaderno", Frase());
            } else {
                Aviso("Cuaderno", "El abuelo tenía muy mala letra: no se entiende nada.");
            }
        }

        // Mensajes de fechas señaladas, una vez por arranque.
        void Fechas() {
            time_t ahora = time(nullptr);
            struct tm *t = localtime(&ahora);
            if (!t)
                return;
            if (t->tm_mon == 7 && t->tm_mday == 22)
                OSD::Notify("22 de agosto: el primer Inazuma Eleven salió tal día como hoy (2008). ¡Felicidades!");
            else if (t->tm_mon == 0 && t->tm_mday == 1)
                OSD::Notify("¡Feliz año nuevo! Que este año marques muchos goles.");
            else if (t->tm_mon == 11 && t->tm_mday == 25)
                OSD::Notify("¡Feliz Navidad! Hasta Mark descansa hoy... o eso dice.");
        }

    }

    MenuFolder *CrearCuaderno() {
        Cargar();
        gCuaderno = new MenuFolder("Cuaderno del abuelo", "Lo encontraste. Daisuke Evans lo escribió para quien viniera después.");
        *gCuaderno += new MenuEntry("Página: el fútbol", nullptr, PaginaFutbol);
        *gCuaderno += new MenuEntry("Página: tres temporadas", nullptr, PaginaContinuidad);
        *gCuaderno += new MenuEntry("Página: los hermanos del norte", nullptr, PaginaHermanos);
        *gCuaderno += new MenuEntry("Página: la última", nullptr, PaginaFinal);
        *gCuaderno += new MenuEntry("Escribir en el cuaderno", nullptr, Escribir, Ajustar("Prueba palabras: algunas despiertan recuerdos."));
        *gCuaderno += new MenuEntry("Créditos", nullptr, Creditos);
        return gCuaderno;  // se oculta en IniciarSecretos, ya dentro del menú (antes, Show la duplicaba)
    }

    namespace {
        volatile bool gKonami = false;

        // En cada fotograma (una vez por pantalla): solo se sigue la secuencia; nada de SD ni ventanas aquí.
        bool Vigia(const Screen &pantalla) {
            static u32 paso = 0, antes = 0;
            if (!pantalla.IsTop)
                return false;
            u32 ahora = Controller::GetKeysDown(true) & TECLAS_KONAMI;
            u32 nuevas = ahora & ~antes;
            antes = ahora;
            if (!nuevas)
                return false;
            if (nuevas == KONAMI[paso]) {
                if (++paso == LARGO_KONAMI) {
                    paso = 0;
                    gKonami = true;
                }
            } else if (nuevas == DPadUp) {
                paso = paso == 1 || paso == 2 ? 2 : 1;  // ↑ ↑ ↑ ↓ ... sigue valiendo
            } else {
                paso = 0;
            }
            return false;  // no se dibuja nada
        }
    }

    void IniciarSecretos() {
        if (gCuaderno && !gDespierto)
            gCuaderno->Hide();
        OSD::Run(Vigia);
    }

    void SecretosPendientes() {
        static u32 llamadas = 0;
        if (++llamadas == 100)  // unos diez segundos después de arrancar, con el juego ya en pantalla
            Fechas();
        if (gKonami) {
            gKonami = false;
            Despertar();
        }
    }

}
