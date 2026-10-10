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

        struct Tecnica {
            uint16_t id;
            uint8_t nivel;
        };

        bool OpcionesCompletas(const Opciones &op) {
            if (op.modo == Modo::SoloFichajes && op.nivelFichaje < 0)
                return false;
            bool necesitaExp = op.modo == Modo::SoloFichajes || (op.modo == Modo::Equilibrada && op.nivelTope >= 0);
            return !necesitaExp || op.expDeNivel;
        }

        void NivelYExp(const JugadorPartida &j, uint16_t numeroDestino, const Opciones &op, uint8_t *nivel,
                       uint32_t *exp) {
            if (op.modo == Modo::Completa)
                *nivel = j.nivel;
            else if (op.modo == Modo::Equilibrada)
                *nivel = (op.nivelTope >= 0 && j.nivel > op.nivelTope) ? static_cast<uint8_t>(op.nivelTope) : j.nivel;
            else
                *nivel = static_cast<uint8_t>(op.nivelFichaje);
            *exp = op.expDeNivel ? op.expDeNivel(numeroDestino, *nivel, op.contexto) : j.exp;
        }

        // Las aprendidas en el juego nuevo (nivel de técnica 0) y después las que traía.
        std::vector<Tecnica> Heredadas(const Opciones &op, uint16_t numeroDestino, uint8_t nivel,
                                       const JugadorPartida &j) {
            std::vector<Tecnica> salida;
            if (op.aprendidas) {
                uint16_t tecs[4] = {};
                int n = op.aprendidas(numeroDestino, nivel, tecs, op.contexto);
                for (int k = 0; k < n && k < 4; ++k)
                    salida.push_back({tecs[k], 0});
            }
            for (uint32_t k = 0; k < NUM_TECNICAS; ++k)
                salida.push_back({j.tecnicas[k], j.nivelesTecnica[k]});
            return salida;
        }

        // Huecos libres; si ya la sabe, se queda el nivel de técnica mayor.
        void Anadir(uint16_t tecnicas[NUM_TECNICAS], uint8_t niveles[NUM_TECNICAS], const std::vector<Tecnica> &nuevas) {
            for (const Tecnica &t : nuevas) {
                if (!t.id)
                    continue;
                int esta = -1, libre = -1;
                for (uint32_t m = 0; m < NUM_TECNICAS; ++m) {
                    if (tecnicas[m] == t.id && esta < 0)
                        esta = m;
                    if (tecnicas[m] == 0 && libre < 0)
                        libre = m;
                }
                if (esta >= 0) {
                    if (t.nivel > niveles[esta])
                        niveles[esta] = t.nivel;
                } else if (libre >= 0) {
                    tecnicas[libre] = t.id;
                    niveles[libre] = t.nivel;
                }
            }
        }

    }

    Informe Traspasar(const Partida &origen, Partida &destino, const Mapa &mapa, const Opciones &op) {
        Informe informe;
        if (!OpcionesCompletas(op)) {
            informe.error = Error::OpcionesIncompletas;
            return informe;
        }
        auto clave = [&op](uint16_t numero) -> uint32_t { return op.clave ? op.clave(numero, op.contexto) : numero; };
        const bool mismoFormato = origen.Formato()->jugadorPaso == destino.Formato()->jugadorPaso;
        std::map<uint32_t, uint32_t> posicion;
        for (uint32_t k = 0; k < destino.NumJugadores(); ++k)
            posicion[clave(destino.Jugador(k).id)] = k;  // como el dict de Python: gana la última
        const bool hayPlantilla = destino.NumJugadores() > 0;
        const std::vector<uint8_t> plantilla = hayPlantilla ? destino.Jugador(0).crudo : std::vector<uint8_t>();

        for (uint32_t i = 0; i < origen.NumJugadores(); ++i) {
            JugadorPartida j = origen.Jugador(i);
            uint16_t numeroDestino;
            if (!mapa.Buscar(j.id, &numeroDestino)) {
                informe.sinPareja.push_back(j.id);
                continue;
            }
            uint8_t nivel;
            uint32_t exp;
            auto existente = posicion.find(clave(numeroDestino));
            if (existente != posicion.end()) {
                // Se mejora la ficha que ya usa el juego (su número), no la que dice la tabla.
                JugadorPartida actual = destino.Jugador(existente->second);
                NivelYExp(j, actual.id, op, &nivel, &exp);
                uint8_t final = nivel > actual.nivel ? nivel : actual.nivel;
                Anadir(actual.tecnicas, actual.nivelesTecnica, Heredadas(op, actual.id, final, j));
                if (nivel > actual.nivel || (nivel == actual.nivel && exp > actual.exp)) {
                    actual.nivel = nivel;
                    actual.exp = exp;
                }
                destino.PonerJugador(existente->second, actual);
                informe.mejorados.push_back({j.id, numeroDestino});
                continue;
            }
            NivelYExp(j, numeroDestino, op, &nivel, &exp);
            if (destino.NumJugadores() >= destino.Formato()->maxJugadores) {
                informe.sinSitio.push_back(j.id);
                continue;
            }
            JugadorPartida ficha;
            if (mismoFormato) {
                ficha.crudo = j.crudo;
            } else if (hayPlantilla) {
                // Formatos distintos: los campos sin interpretar salen de una ficha del juego de destino.
                ficha.crudo = plantilla;
                informe.conPlantilla.push_back(j.id);
            } else {
                informe.sinPlantilla.push_back(j.id);
                continue;
            }
            NormalizarFichaNueva(ficha.crudo, *destino.Formato());
            ficha.id = numeroDestino;
            ficha.nivel = nivel;
            ficha.exp = exp;
            Anadir(ficha.tecnicas, ficha.nivelesTecnica, Heredadas(op, numeroDestino, nivel, j));
            uint32_t indice;
            destino.AnadirJugador(ficha, &indice);
            posicion.emplace(clave(numeroDestino), indice);
            informe.traspasados.push_back({j.id, numeroDestino});
            if (destino.Formato()->grupoAlTraspasar && destino.MeterEnGrupo(indice))
                informe.alGrupo.push_back(numeroDestino);
        }
        return informe;
    }

    Fichaje Fichar(Partida &destino, uint16_t numero, uint8_t nivel, const Opciones &op,
                   const std::vector<uint8_t> *plantilla, uint32_t *indice) {
        if (!op.expDeNivel)
            return Fichaje::OpcionesIncompletas;
        auto clave = [&op](uint16_t n) -> uint32_t { return op.clave ? op.clave(n, op.contexto) : n; };
        for (uint32_t k = 0; k < destino.NumJugadores(); ++k)
            if (clave(destino.Jugador(k).id) == clave(numero))
                return Fichaje::YaEsta;
        if (destino.NumJugadores() >= destino.Formato()->maxJugadores)
            return Fichaje::SinSitio;
        if (!plantilla && destino.NumJugadores() == 0)
            return Fichaje::SinPlantilla;
        JugadorPartida ficha;
        ficha.crudo = plantilla ? *plantilla : destino.Jugador(0).crudo;
        NormalizarFichaNueva(ficha.crudo, *destino.Formato());
        ficha.id = numero;
        ficha.nivel = nivel;
        ficha.exp = op.expDeNivel(numero, nivel, op.contexto);
        Anadir(ficha.tecnicas, ficha.nivelesTecnica, Heredadas(op, numero, nivel, JugadorPartida()));
        uint32_t i;
        if (destino.AnadirJugador(ficha, &i) != Error::Ninguno)
            return Fichaje::SinSitio;
        destino.MeterEnGrupo(i);
        if (indice)
            *indice = i;
        return Fichaje::Hecho;
    }

}
