#pragma once

// Memoria del equipo de una partida: lo que llegó a tener cada personaje y quién llega con la historia. Gemelo de
// tools/src/ie123kit/definitiva/memoria_equipo.py (mismo texto y mismas reglas; pruebas/memoria_pc.cpp).
//
// La historia une jugadores con la instrucción 16446 y, con alguien que ya está, no hace nada: ni lo duplica ni le
// baja el nivel, pero tampoco lo pasa de la reserva al grupo. La 16447 borra el registro: si lo vuelve a unir,
// llega nuevo. Por eso el traspaso no crea a quien la historia va a unir: lo deja aquí como pendiente y el plugin,
// al verlo aparecer en la plantilla, le devuelve nivel, EXP y técnicas (#13: Shawn y Jordan, tras el partido
// contra Corea). Igual con quien la historia borra y vuelve a unir con menos nivel del que tuvo.
//
// Va por la clave del personaje (JuegoDatos::Clave, la del nombre) y no por número: Shawn tiene tres fichas
// jugables en el IE3. Una por archivo de partida (el identificador de la cabecera, +0x20, fijo desde que se crea
// el archivo); el tiempo de juego separa una partida nueva sobre el mismo archivo: si baja, es otra.
// Texto: «#partida=XXXXXXXX», «#tiempo=N» y una línea por personaje, «clave=t1:n1,t2,...;n=número;nv=nivel;
// xp=EXP;p=1» (:n es el nivel de la técnica, si no es 0; p=1, pendiente).

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "Partida.hpp"

namespace Definitiva {

    struct TecnicaRecordada {
        uint16_t id;
        uint8_t nivel;
    };

    struct Recuerdo {
        uint16_t numero = 0;
        uint8_t nivel = 0;
        uint32_t exp = 0;
        // Llega con la historia (traspaso): al aparecer se le devuelve todo, aunque la historia lo traiga con más
        // nivel.
        bool pendiente = false;
        std::vector<TecnicaRecordada> tecnicas;  // en el orden en que las aprendió
    };

    class MemoriaEquipo {
    public:
        static constexpr size_t MAX_POR_JUGADOR = 16;

        uint32_t partida = 0;  // identificador del archivo de partida (cabecera + 0x20)
        uint32_t tiempo = 0;   // tiempo de juego (fotogramas) la última vez que se vio la partida

        // Lo que tiene ahora un jugador de la plantilla (técnica 0 = hueco libre; `niveles` puede ser nullptr). Se
        // queda el nivel más alto que ha tenido y deja de estar pendiente. true si cambia algo que haya que guardar.
        bool Apuntar(uint32_t clave, uint16_t numero, uint8_t nivel, uint32_t exp, const uint16_t *tecnicas,
                     const uint8_t *niveles, size_t cuantas);

        // Llega con la historia (traspaso): sustituye lo que hubiera de ese personaje.
        void Esperar(uint32_t clave, const Recuerdo &r);

        // Las que recuerda y no tiene ahora, en el orden en que las aprendió.
        std::vector<uint16_t> Perdidas(uint32_t clave, const uint16_t *tecnicas, size_t cuantas) const;

        const Recuerdo *Buscar(uint32_t clave) const;
        const std::map<uint32_t, Recuerdo> &Todo() const { return mem_; }
        bool Vacia() const { return mem_.empty(); }

        std::string Serializar() const;
        // Tolerante: se salta lo que no entiende (las líneas antiguas «clave=t1,t2» se siguen leyendo).
        void Cargar(const std::string &texto);

    private:
        std::map<uint32_t, Recuerdo> mem_;
    };

    // Lo que hay que devolver a un jugador que acaba de aparecer: si estaba pendiente o la historia lo ha vuelto a
    // crear con menos nivel del que tuvo, el nivel y la EXP (si eran más) y las técnicas recordadas que no tiene, en
    // sus huecos libres (o su nivel de técnica, si era mayor). true si ha cambiado algo.
    bool Restaurar(JugadorPartida &j, const Recuerdo &r);

}
