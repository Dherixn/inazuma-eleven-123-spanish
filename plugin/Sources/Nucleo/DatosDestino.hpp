#pragma once

// Tablas de un juego de la recopilación construidas en tiempo de ejecución a partir de sus archivos
// (unitbase.dat, unitstat.dat, lvup_exp.dat, unitno.dat de archive.fa): así el plugin no lleva datos
// del juego y ve lo que añada la capa de datos del mod. Gemelo de ie123kit.definitiva.datos_juego y
// de identidades.emparejar_por_cara.

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "Traspaso.hpp"

namespace Definitiva {

    struct PerfilJuego {
        uint32_t basePaso;
        uint32_t offIdentidad;  // primer byte = curva de EXP
        uint32_t offCara;
        uint32_t offPosicion;
        uint32_t offNombre;     // nombre completo (clave del personaje)
        uint32_t anchoNombre;
        uint32_t statPaso;
        bool statOfuscado;      // IE3: transformación de item.dat
        bool tecnicaU8;         // IE1 de la recopilación: técnica en 2 B
        bool nivelU8;           // IE3: nivel de técnica en 1 B
    };

    extern const PerfilJuego PERFIL_IE1;
    extern const PerfilJuego PERFIL_IE2;
    extern const PerfilJuego PERFIL_IE3;

    class JuegoDatos {
    public:
        bool Construir(const PerfilJuego &perfil, const std::vector<uint8_t> &unitbase,
                       const std::vector<uint8_t> &unitstat, const std::vector<uint8_t> &lvup,
                       const std::vector<uint8_t> &unitno);

        size_t Fichas() const { return caras_.size(); }
        uint16_t Cara(size_t i) const { return caras_[i]; }
        uint8_t Posicion(size_t i) const { return posiciones_[i]; }
        bool Jugable(size_t i) const { return jugables_[i]; }
        uint16_t Numero(size_t i) const { return numeros_[i]; }
        // Campo del nombre completo de la ficha, tal cual (Shift-JIS de la traducción; ver NombreSjis).
        const uint8_t *NombreCrudo(size_t i) const { return &nombres_[i * anchoNombre_]; }
        size_t AnchoNombre() const { return anchoNombre_; }

        // Por número de jugador (el de la partida). -1 si no tiene ficha.
        int Ficha(uint16_t numero) const;
        uint32_t ExpMinima(uint16_t numero, uint8_t nivel) const;
        int Aprendidas(uint16_t numero, uint8_t nivel, uint16_t salida[4]) const;
        uint32_t Clave(uint16_t numero) const;

        // Rellena las funciones del traspaso con este juego como destino.
        void Preparar(Opciones &op) const;

    private:
        std::vector<std::vector<uint32_t>> curvas_;
        std::vector<uint8_t> curva_;
        std::vector<uint16_t> caras_;
        std::vector<uint8_t> posiciones_;
        std::vector<bool> jugables_;
        std::vector<uint16_t> numeros_;
        std::vector<uint32_t> claves_;
        std::vector<uint16_t> tecnicas_;  // 4 por ficha
        std::vector<uint8_t> niveles_;    // 4 por ficha
        std::vector<uint8_t> nombres_;    // anchoNombre_ por ficha
        size_t anchoNombre_ = 0;
        std::vector<std::pair<uint16_t, uint32_t>> indice_;  // (número, primera ficha), ordenado
    };

    // Origen → destino por cara entre jugables, en números de jugador y ordenado por origen.
    std::vector<Par> EmparejarPorCara(const JuegoDatos &origen, const JuegoDatos &destino);

    // Si la ficha de destino no es una de las que la historia de ese juego une al equipo (`altas`, ordenadas:
    // ver Eventos.hpp) y otra jugable del mismo personaje sí, se usa esa (la de la misma posición y, si no,
    // la primera). Así el traspaso no duplica a quien la historia vuelva a fichar: en el IE3, Jude es la 30 y
    // no la 1097 de su cara del IE2. Gemelo de identidades.preferir_altas.
    std::vector<Par> PreferirAltas(const std::vector<Par> &pares, const JuegoDatos &destino,
                                   const std::vector<uint16_t> &altas);

}
