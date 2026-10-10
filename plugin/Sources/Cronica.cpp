#include "Cronica.hpp"

#include <cstring>
#include <vector>

#include <CTRPluginFramework.hpp>

#include "Nucleo/Partida.hpp"
#include "Partidas.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        // En la información de cada espacio (desde su comienzo): tiempo en fotogramas a 60 por segundo, episodio
        // y nivel de equipo. Comprobado con partidas de 6 y 35 minutos y con la ficha que enseña el juego.
        constexpr u32 OFF_TIEMPO = 0x50, OFF_EPISODIO = 0x54, OFF_NIVEL_EQUIPO = 0x55;

        struct Temporada {
            const char *titulo;
            const char *archivo;
            const char *version;
            const FormatoPartida *formato;
        };

        const Temporada TEMPORADAS[] = {
            {"Temporada 1 · El Fútbol Frontier", "/ina1_save.dat", "Inazuma Eleven", &FORMATO_IE1},
            {"Temporada 2 · La invasión", "/ina2F_save.dat", "Tormenta de Fuego", &FORMATO_IE2},
            {"Temporada 2 · La invasión", "/ina2B_save.dat", "Ventisca Eterna", &FORMATO_IE2},
            {"Temporada 3 · El Mundial", "/ina3S_save.dat", "Rayo Celeste", &FORMATO_IE3},
            {"Temporada 3 · El Mundial", "/ina3B_save.dat", "Fuego Explosivo", &FORMATO_IE3},
            {"Epílogo · El Ogro", "/ina3O_save.dat", "La amenaza del Ogro", &FORMATO_IE3},
        };

        u32 U32(const std::vector<u8> &d, u32 off) {
            return d[off] | d[off + 1] << 8 | d[off + 2] << 16 | static_cast<u32>(d[off + 3]) << 24;
        }

    }

    std::string TextoCronica() {
        std::string texto;
        u64 totalSegundos = 0;
        int jugadas = 0;
        for (const Temporada &t : TEMPORADAS) {
            std::vector<u8> datos;
            if (!LeerPartida(t.archivo, datos))
                continue;
            // El espacio más avanzado (el IE1 tiene tres): el de más tiempo jugado.
            int mejor = -1;
            u32 mejorTiempo = 0;
            for (u32 k = 0; k < t.formato->numEspacios; ++k) {
                Partida p;
                if (!p.Cargar(datos, t.formato, k) || !p.Ocupado() || p.Validar() != Error::Ninguno)
                    continue;
                u32 tiempo = U32(datos, k * t.formato->EspacioLargo() + OFF_TIEMPO);
                if (mejor < 0 || tiempo > mejorTiempo) {
                    mejor = static_cast<int>(k);
                    mejorTiempo = tiempo;
                }
            }
            if (mejor < 0)
                continue;
            Partida p;
            p.Cargar(datos, t.formato, static_cast<u32>(mejor));
            u32 base = static_cast<u32>(mejor) * t.formato->EspacioLargo();
            u32 segundos = mejorTiempo / 60;
            totalSegundos += segundos;
            ++jugadas;
            texto += Utils::Format("%s\n  %s: episodio %u, nivel %u, %u jugadores, %luh %02lum\n", t.titulo, t.version,
                                   datos[base + OFF_EPISODIO], datos[base + OFF_NIVEL_EQUIPO], p.NumJugadores(),
                                   static_cast<unsigned long>(segundos / 3600),
                                   static_cast<unsigned long>(segundos / 60 % 60));
        }
        if (!jugadas)
            return "Aún no hay ninguna temporada empezada. ¡El balón te espera en la ribera del río!";
        texto += Utils::Format("\nEn total: %lluh %02llum de leyenda.", totalSegundos / 3600, totalSegundos / 60 % 60);
        if (jugadas >= 4)
            texto += "\nDesde el Fútbol Frontier hasta el último partido: esto ya es una leyenda.";
        return texto;
    }

    const char *TextoPuente(const char *destino, bool desdeIE3) {
        if (std::strcmp(destino, "inazuma2") == 0)
            return "Tras ganar el Fútbol Frontier, el Raimon celebra por fin su título... hasta que algo cae del "
                   "cielo sobre el instituto. Unos jugadores que dicen venir del espacio empiezan a destrozar "
                   "colegios de todo Japón. Tus compañeros de la primera temporada vuelven a ponerse las botas.";
        if (std::strcmp(destino, "inazuma3_ogre") == 0 && desdeIE3)
            return "Sois campeones del mundo. Pero un equipo llegado desde ochenta años en el futuro, el Ogro, "
                   "viene decidido a cambiar la historia del fútbol. Solo los de siempre pueden pararlo.";
        if (std::strncmp(destino, "inazuma3", 8) == 0)
            return "La amenaza ha pasado y Japón vuelve a jugar al fútbol. Llega el Fútbol Frontier Internacional: "
                   "se forma la selección y los que lucharon contra la invasión se juegan un puesto en ella.";
        return "";
    }

}
