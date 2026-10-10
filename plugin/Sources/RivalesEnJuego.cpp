#include "RivalesEnJuego.hpp"

#include <cstring>
#include <vector>

#include <3ds.h>

#include <CTRPluginFramework.hpp>

#include "Nucleo/Rivales.hpp"
#include "TablasJuego.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        // El juego la deja en el heap; en las pruebas, en 0x08952308 y 0x088E00A4 (IE2) y 0x08AF89CC (IE3).
        constexpr u32 ZONA = 0x08000000, ZONA_LARGO = 0x02000000;
        constexpr u32 TROZO = 0x8000;           // lo que se mira en cada llamada: la zona entera en un segundo
        constexpr u32 PAUSA_TRAS_VUELTA = 500;  // llamadas (~0,5 s) de descanso tras recorrerla sin encontrarla
        constexpr u32 ANCLA_LARGO = 16;

        JuegoActivo gJuego = JuegoActivo::Ninguno;
        const Partidos *gPartidos = nullptr;
        bool gSinDatos = false;  // no se pudo leer games.dat de este juego: no se insiste
        std::vector<u8> gDeseada;  // la tabla con los niveles del modo (solo cuentan los niveles)
        int gModo = -1, gNivel = -1;
        u32 gAncla = 0;  // 16 bytes de la tabla sin ningún nivel, para buscarla
        u32 gTabla = 0, gCursor = 0, gPausa = 0;
        bool gTocada = false;  // la tabla en memoria tiene niveles puestos por nosotros

        const char *Tablas(JuegoActivo j) {
            switch (j) {
            case JuegoActivo::IE1: return "inazuma1";
            case JuegoActivo::IE2: return "inazuma2";
            case JuegoActivo::IE3: return "inazuma3";
            case JuegoActivo::Ogro: return "inazuma3_ogre";
            default: return nullptr;
            }
        }

        // Cuántos bytes seguidos se pueden leer desde `dir` (hasta `quiero`): en el heap hay huecos sin mapear.
        u32 TramoLegible(u32 dir, u32 quiero) {
            u32 hecho = 0;
            while (hecho < quiero) {
                MemInfo info;
                PageInfo pagina;
                if (R_FAILED(svcQueryMemory(&info, &pagina, dir + hecho)) || info.state == MEMSTATE_FREE
                    || !(info.perm & MEMPERM_READ))
                    break;
                u32 fin = info.base_addr + info.size;
                hecho = fin - dir >= quiero ? quiero : fin - dir;
            }
            return hecho;
        }

        // El primer partido con equipo, a partir de su +4: va alineado a 4 como la tabla.
        u32 Ancla(const std::vector<u8> &t) {
            for (u32 off = 0; off + PARTIDO_PASO <= t.size(); off += PARTIDO_PASO)
                if (t[off] | t[off + 1])
                    return off + 4;
            return 4;
        }

        bool Escribir(u32 tabla) {
            const std::vector<u8> &original = gPartidos->tabla;
            u32 largo = static_cast<u32>(original.size());
            if (TramoLegible(tabla, largo) != largo || !Process::CheckAddress(tabla + largo - 1)
                || !MismaTablaDePartidos(reinterpret_cast<const u8 *>(tabla), original.data(), largo))
                return false;
            u8 *m = reinterpret_cast<u8 *>(tabla);
            for (u32 off = OFF_PARTIDO_NIVEL; off < largo; off += PARTIDO_PASO)
                m[off] = gDeseada[off];
            gTocada = gModo != RIVALES_ORIGINAL;
            return true;
        }

        // ¿Sigue la tabla donde estaba y con los niveles del modo? Si el juego la ha recargado ahí mismo, se vuelven
        // a poner. Falso si ya no está.
        bool Repasar() {
            const std::vector<u8> &original = gPartidos->tabla;
            u32 largo = static_cast<u32>(original.size());
            if (TramoLegible(gTabla, largo) != largo)
                return false;
            const u8 *m = reinterpret_cast<const u8 *>(gTabla);
            if (std::memcmp(m + gAncla, original.data() + gAncla, ANCLA_LARGO) != 0)
                return false;
            for (u32 off = OFF_PARTIDO_NIVEL; off < largo; off += PARTIDO_PASO)
                if (m[off] != gDeseada[off])
                    return Escribir(gTabla);
            return true;
        }

        // Un trozo más de la búsqueda.
        void Buscar() {
            if (gPausa) {
                --gPausa;
                return;
            }
            const std::vector<u8> &original = gPartidos->tabla;
            const u8 *ancla = original.data() + gAncla;
            u32 primero;
            std::memcpy(&primero, ancla, 4);
            u32 inicio = ZONA + gCursor;
            u32 legible = TramoLegible(inicio, TROZO + ANCLA_LARGO);
            if (legible >= ANCLA_LARGO) {
                for (u32 off = 0; off + ANCLA_LARGO <= legible && off < TROZO; off += 4) {
                    const u8 *p = reinterpret_cast<const u8 *>(inicio + off);
                    if (*reinterpret_cast<const u32 *>(p) != primero || std::memcmp(p, ancla, ANCLA_LARGO) != 0)
                        continue;
                    u32 tabla = inicio + off - gAncla;
                    if (tabla >= ZONA && Escribir(tabla)) {
                        gTabla = tabla;
                        gCursor = 0;
                        return;
                    }
                }
                // Hasta justo después de lo mirado: si el bloque legible se acaba antes, el siguiente empieza ahí.
                gCursor += legible >= TROZO + ANCLA_LARGO ? TROZO : ((legible - ANCLA_LARGO) & ~3u) + 4;
            } else {
                MemInfo info;
                PageInfo pagina;
                u32 salto = R_SUCCEEDED(svcQueryMemory(&info, &pagina, inicio)) && info.base_addr + info.size > inicio
                                ? info.base_addr + info.size - inicio
                                : 0x1000;
                gCursor += salto;
            }
            if (gCursor >= ZONA_LARGO) {
                gCursor = 0;
                gPausa = PAUSA_TRAS_VUELTA;
            }
        }

    }

    void CuidarRivales(JuegoActivo juego, int modo, int nivelEquipo) {
        if (juego != gJuego) {
            gJuego = juego;
            gPartidos = nullptr;
            gSinDatos = false;
            gModo = gNivel = -1;
            gTabla = gCursor = gPausa = 0;
            gTocada = false;
        }
        if (juego == JuegoActivo::Ninguno || juego == JuegoActivo::IE1 || gSinDatos
            || (modo == RIVALES_ORIGINAL && !gTocada))
            return;
        if (!gPartidos) {
            gPartidos = PartidosDe(Tablas(juego));
            if (!gPartidos) {
                gSinDatos = true;
                return;
            }
            gAncla = Ancla(gPartidos->tabla);
        }
        if (modo != RIVALES_A_TU_NIVEL)
            nivelEquipo = 0;
        if (modo != gModo || nivelEquipo != gNivel) {
            gDeseada = gPartidos->tabla;
            AjustarPartidos(gPartidos->tabla.data(), gDeseada.data(), static_cast<u32>(gDeseada.size()), modo, nivelEquipo,
                            gPartidos->extra.data(), static_cast<u32>(gPartidos->extra.size()));
            gModo = modo;
            gNivel = nivelEquipo;
        }
        if (gTabla && Repasar())
            return;
        if (gTabla) {
            gTabla = 0;
            gTocada = false;  // el juego la ha soltado: la que cargue de nuevo viene como el original
        }
        if (modo != RIVALES_ORIGINAL)
            Buscar();
    }

}
