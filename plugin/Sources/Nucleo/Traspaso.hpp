#pragma once

// Traspaso de fichajes al juego siguiente. Gemelo de tools/src/ie123kit/definitiva/traspaso.py:
// se hereda el NIVEL (cada juego tiene sus curvas de EXP), la EXP se recalcula con la curva del juego
// nuevo y las técnicas son las que el jugador sabe a su nivel en ese juego más las que traía.
// Los números son números de jugador (los de la partida), no índices de ficha.
// A quien la historia del juego nuevo va a unir al equipo (Opciones::aplazar) no se le crea ficha: la
// historia no lo pasaría de la reserva al grupo. Sale en Informe::aplazados y el plugin se lo devuelve
// cuando la historia lo une (MemoriaEquipo). El resto llega a la reserva: el grupo lo forma la historia.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Partida.hpp"

namespace Definitiva {

    enum class Modo { Completa, Equilibrada, SoloFichajes };

    // Datos del juego de destino, por número de jugador.
    typedef uint32_t (*ExpDeNivel)(uint16_t numero, uint8_t nivel, void *contexto);
    // Escribe en `salida` (hasta 4) las técnicas que sabe a ese nivel; devuelve cuántas.
    typedef int (*Aprendidas)(uint16_t numero, uint8_t nivel, uint16_t salida[4], void *contexto);
    // Clave del personaje: igual para las variantes de uno mismo con otra cara.
    typedef uint32_t (*ClavePersonaje)(uint16_t numero, void *contexto);

    struct Opciones {
        Modo modo = Modo::Equilibrada;
        int nivelTope = -1;     // equilibrada: -1 = sin tope
        int nivelFichaje = -1;  // solo fichajes: obligatorio
        ExpDeNivel expDeNivel = nullptr;      // sin él se conserva la EXP (solo pruebas de un mismo juego)
        Aprendidas aprendidas = nullptr;
        ClavePersonaje clave = nullptr;       // sin ella, «ya está» va por el número
        void *contexto = nullptr;
        // Números de destino que la historia une al equipo (sus altas, ordenados); nullptr = ninguno.
        const std::vector<uint16_t> *aplazar = nullptr;
    };

    // Un jugador que llega con la historia: lo que trae, ya en el juego de destino.
    struct Aplazado {
        uint16_t origen;
        uint16_t destino;
        uint8_t nivel;
        uint32_t exp;
        uint16_t tecnicas[NUM_TECNICAS];
        uint8_t nivelesTecnica[NUM_TECNICAS];
    };

    struct Par {
        uint16_t origen;
        uint16_t destino;
    };

    // Tabla de números de jugador del juego de origen al de destino, ordenada por origen.
    struct Mapa {
        const Par *pares = nullptr;
        size_t cantidad = 0;
        bool Buscar(uint16_t origen, uint16_t *destino) const;
    };

    struct Informe {
        Error error = Error::Ninguno;
        std::vector<Par> traspasados;
        std::vector<Par> mejorados;
        std::vector<uint16_t> sinPareja;
        std::vector<uint16_t> sinSitio;
        std::vector<uint16_t> sinPlantilla;
        std::vector<uint16_t> conPlantilla;
        std::vector<Aplazado> aplazados;
    };

    // Copia los fichajes de `origen` en `destino` (en memoria; después hay que sellar).
    Informe Traspasar(const Partida &origen, Partida &destino, const Mapa &mapa, const Opciones &op);

    enum class Fichaje { Hecho, YaEsta, SinSitio, SinPlantilla, OpcionesIncompletas };

    // Fichaje especial (Aiden Froste): un jugador nuevo con ese nivel, la EXP mínima de ese nivel y las
    // técnicas que sabe a ese nivel. Los campos sin interpretar salen de `plantilla` (un jugador de la
    // misma partida; nullptr = el primero). Entra en el grupo si cabe (`alGrupo`) o en la reserva.
    // Gemelo de traspaso.fichar.
    Fichaje Fichar(Partida &destino, uint16_t numero, uint8_t nivel, const Opciones &op,
                   const std::vector<uint8_t> *plantilla = nullptr, uint32_t *indice = nullptr, bool alGrupo = true);

}
