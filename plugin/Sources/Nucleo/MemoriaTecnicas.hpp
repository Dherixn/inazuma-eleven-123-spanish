#pragma once

// Memoria de las técnicas que ha llegado a saber cada personaje, para devolverle las que pierda (#13: en el IE3,
// Shawn y Jordan vuelven tras el partido contra Corea sin sus técnicas de manual). Va por la clave del personaje
// (JuegoDatos::Clave, la del nombre) y no por número: Shawn tiene tres fichas jugables en el IE3 (2329, 2337 y
// 2338) y puede volver con otra. Se guarda en texto, una línea por personaje: «clave=t1,t2,...».

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Definitiva {

    class MemoriaTecnicas {
    public:
        static constexpr size_t MAX_POR_JUGADOR = 16;

        // Apunta las técnicas que tiene ahora (0 = hueco libre). true si ha aprendido alguna que no constaba.
        bool Apuntar(uint32_t clave, const uint16_t *tecnicas, size_t cuantas);

        // Las que recuerda y no tiene ahora, en el orden en que las aprendió.
        std::vector<uint16_t> Perdidas(uint32_t clave, const uint16_t *tecnicas, size_t cuantas) const;

        std::string Serializar() const;
        void Cargar(const std::string &texto);
        bool Vacia() const { return mem_.empty(); }

    private:
        std::map<uint32_t, std::vector<uint16_t>> mem_;
    };

}
