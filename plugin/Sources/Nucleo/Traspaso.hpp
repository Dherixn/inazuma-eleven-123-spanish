#pragma once

// Traspaso de fichajes al juego siguiente. Gemelo de tools/src/ie123kit/definitiva/traspaso.py.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Partida.hpp"

namespace Definitiva {

    enum class Modo { Completa, Equilibrada, SoloFichajes };

    // (id del jugador en el juego de destino, nivel) -> EXP mínima de ese nivel.
    typedef uint32_t (*ExpDeNivel)(uint16_t idDestino, uint8_t nivel, void *contexto);

    struct Opciones {
        Modo modo = Modo::Equilibrada;
        int nivelTope = -1;     // equilibrada: -1 = sin tope
        int nivelFichaje = -1;  // solo fichajes: obligatorio
        ExpDeNivel expDeNivel = nullptr;
        void *contexto = nullptr;
    };

    struct Par {
        uint16_t origen;
        uint16_t destino;
    };

    // Tabla de números de jugador del juego de origen al de destino, ordenada por origen.
    // Sin pares = misma numeración (IE1 -> IE2).
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
    };

    // Copia los fichajes de `origen` en `destino` (en memoria; después hay que sellar).
    Informe Traspasar(const Partida &origen, Partida &destino, const Mapa &mapa, const Opciones &op);

}
