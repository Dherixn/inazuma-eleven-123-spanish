#include "Traspaso.hpp"

#include <map>

namespace Definitiva {

    bool Mapa::Buscar(uint16_t origen, uint16_t *destino) const {
        if (!pares) {
            *destino = origen;
            return true;
        }
        size_t lo = 0, hi = cantidad;
        while (lo < hi) {
            size_t medio = (lo + hi) / 2;
            if (pares[medio].origen < origen)
                lo = medio + 1;
            else
                hi = medio;
        }
        if (lo < cantidad && pares[lo].origen == origen) {
            *destino = pares[lo].destino;
            return true;
        }
        return false;
    }

    namespace {

        bool OpcionesCompletas(const Opciones &op) {
            if (op.modo == Modo::SoloFichajes && op.nivelFichaje < 0)
                return false;
            bool necesitaExp = op.modo == Modo::SoloFichajes || (op.modo == Modo::Equilibrada && op.nivelTope >= 0);
            return !necesitaExp || op.expDeNivel;
        }

        void NivelYExp(const JugadorPartida &j, uint16_t idDestino, const Opciones &op, uint8_t *nivel, uint32_t *exp) {
            *nivel = j.nivel;
            *exp = j.exp;
            if (op.modo == Modo::Equilibrada && op.nivelTope >= 0 && j.nivel > op.nivelTope) {
                *nivel = static_cast<uint8_t>(op.nivelTope);
                *exp = op.expDeNivel(idDestino, *nivel, op.contexto);
            } else if (op.modo == Modo::SoloFichajes) {
                *nivel = static_cast<uint8_t>(op.nivelFichaje);
                *exp = op.expDeNivel(idDestino, *nivel, op.contexto);
            }
        }

        // El jugador ya estaba: más EXP gana, y sus huecos de técnica vacíos se rellenan.
        JugadorPartida Fundir(JugadorPartida actual, const JugadorPartida &nuevo) {
            for (uint32_t k = 0; k < NUM_TECNICAS; ++k) {
                uint16_t tec = nuevo.tecnicas[k];
                if (!tec)
                    continue;
                int esta = -1, libre = -1;
                for (uint32_t m = 0; m < NUM_TECNICAS; ++m) {
                    if (actual.tecnicas[m] == tec && esta < 0)
                        esta = m;
                    if (actual.tecnicas[m] == 0 && libre < 0)
                        libre = m;
                }
                if (esta >= 0) {
                    if (nuevo.nivelesTecnica[k] > actual.nivelesTecnica[esta])
                        actual.nivelesTecnica[esta] = nuevo.nivelesTecnica[k];
                } else if (libre >= 0) {
                    actual.tecnicas[libre] = tec;
                    actual.nivelesTecnica[libre] = nuevo.nivelesTecnica[k];
                }
            }
            if (nuevo.exp > actual.exp) {
                actual.exp = nuevo.exp;
                actual.nivel = nuevo.nivel;
            }
            return actual;
        }

    }

    Informe Traspasar(const Partida &origen, Partida &destino, const Mapa &mapa, const Opciones &op) {
        Informe informe;
        if (!OpcionesCompletas(op)) {
            informe.error = Error::OpcionesIncompletas;
            return informe;
        }
        const bool mismoFormato = origen.Formato()->jugadorPaso == destino.Formato()->jugadorPaso;
        std::map<uint16_t, uint32_t> posicion;
        for (uint32_t k = 0; k < destino.NumJugadores(); ++k)
            posicion[destino.Jugador(k).id] = k;  // como el dict de Python: gana la última
        const bool hayPlantilla = destino.NumJugadores() > 0;
        const std::vector<uint8_t> plantilla = hayPlantilla ? destino.Jugador(0).crudo : std::vector<uint8_t>();

        for (uint32_t i = 0; i < origen.NumJugadores(); ++i) {
            JugadorPartida j = origen.Jugador(i);
            uint16_t idDestino;
            if (!mapa.Buscar(j.id, &idDestino)) {
                informe.sinPareja.push_back(j.id);
                continue;
            }
            uint8_t nivel;
            uint32_t exp;
            NivelYExp(j, idDestino, op, &nivel, &exp);

            auto existente = posicion.find(idDestino);
            if (existente != posicion.end()) {
                JugadorPartida nuevo = j;
                nuevo.id = idDestino;
                nuevo.nivel = nivel;
                nuevo.exp = exp;
                destino.PonerJugador(existente->second, Fundir(destino.Jugador(existente->second), nuevo));
                informe.mejorados.push_back({j.id, idDestino});
                continue;
            }
            if (destino.NumJugadores() >= MAX_JUGADORES) {
                informe.sinSitio.push_back(j.id);
                continue;
            }
            JugadorPartida ficha = j;
            if (!mismoFormato) {
                if (!hayPlantilla) {
                    // Destino vacío y formatos distintos: los campos sin interpretar no se inventan.
                    informe.sinPlantilla.push_back(j.id);
                    continue;
                }
                ficha.crudo = plantilla;
                informe.conPlantilla.push_back(j.id);
            }
            ficha.id = idDestino;
            ficha.nivel = nivel;
            ficha.exp = exp;
            uint32_t indice;
            destino.AnadirJugador(ficha, &indice);
            posicion.emplace(idDestino, indice);
            informe.traspasados.push_back({j.id, idDestino});
        }
        return informe;
    }

}
