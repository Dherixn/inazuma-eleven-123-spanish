#include "PlantillaEnJuego.hpp"

#include <CTRPluginFramework.hpp>

#include "Nucleo/Plantilla.hpp"
#include "TablasJuego.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        struct Info {
            const char *tablas;
            const PerfilJuego *perfil;
            const GeometriaPlantilla *geo;
            u32 zona;   // dónde suele estar: el IE1 y el IE2 en el heap, el IE3 en el heap lineal
            u32 largo;
        };

        constexpr u32 HEAP = 0x08000000, HEAP_LARGO = 0x02000000;
        constexpr u32 LINEAL = 0x14000000, LINEAL_LARGO = 0x04000000;
        constexpr u32 TROZO = 0x00080000;  // lo que se mira en cada llamada
        constexpr u32 PAUSA_TRAS_VUELTA = 30;  // llamadas (~3 s) de descanso tras recorrer la zona sin éxito
        constexpr u32 MINIMO = 4;

        Info InfoDe(JuegoActivo j) {
            switch (j) {
            case JuegoActivo::IE1: return {"inazuma1", &PERFIL_IE1, &GEO_IE1, HEAP, HEAP_LARGO};
            case JuegoActivo::IE2: return {"inazuma2", &PERFIL_IE2, &GEO_IE2, HEAP, HEAP_LARGO};
            case JuegoActivo::IE3: return {"inazuma3", &PERFIL_IE3, &GEO_IE3, LINEAL, LINEAL_LARGO};
            case JuegoActivo::Ogro: return {"inazuma3_ogre", &PERFIL_IE3, &GEO_IE3, LINEAL, LINEAL_LARGO};
            default: return {nullptr, nullptr, nullptr, 0, 0};
            }
        }

        JuegoActivo gJuego = JuegoActivo::Ninguno;
        u32 gBase = 0, gCursor = 0, gPausa = 0;
        MultiplicadorExp gMultiplicador;

        // Estado legible desde fuera (Azahar RPC, plugin/pruebas) para las pruebas: firma «PLNT», juego, opciones,
        // fichas de sus tablas, cursor de búsqueda, plantilla, jugadores y llamadas.
        volatile u32 gEstado[8] = {0x544E4C50, 0, 0, 0, 0, 0, 0, 0};

        // Cuántos bytes seguidos se pueden leer desde `dir` (hasta `quiero`), bloque a bloque: dentro del heap hay
        // módulos CRO con su código en solo lectura, y en medio puede haber huecos sin mapear.
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

        bool Escribible(u32 dir, u32 largo) { return Process::CheckAddress(dir) && Process::CheckAddress(dir + largo - 1); }

        void Olvidar() {
            gBase = 0;
            gMultiplicador.Olvidar();
        }

        void Aplicar(const GeometriaPlantilla &g, const JuegoDatos &t, u32 n, bool llenar, int factor) {
            for (u32 i = 0; i < n; ++i) {
                u8 *r = reinterpret_cast<u8 *>(gBase + i * g.paso);
                if (llenar) {
                    u32 maximos = gBase + g.offPePt + i * g.pasoPePt;
                    if (TramoLegible(maximos, 4) == 4 && Escribible(reinterpret_cast<u32>(r) + OFF_PE_PT_ACTUALES, 4))
                        LlenarPePt(r + OFF_PE_PT_ACTUALES, reinterpret_cast<const u8 *>(maximos));
                }
                u16 numero = static_cast<u16>(r[4] | r[5] << 8);
                u32 exp = r[0] | r[1] << 8 | r[2] << 16 | static_cast<u32>(r[3]) << 24;
                u32 tope = TopeExp(t, numero, r[g.offNivel]);
                u32 nueva = gMultiplicador.Aplicar(numero, exp, factor, tope ? tope - 1 : 0);
                if (nueva != exp && Escribible(reinterpret_cast<u32>(r), 4))
                    *reinterpret_cast<u32 *>(r) = nueva;  // registros alineados a 4
            }
        }

    }

    JuegoActivo JuegoActual() { return gJuego; }

    u32 DireccionPuntos() {
        JuegoActivo juego = gJuego;
        s32 distancia = juego == JuegoActivo::IE1 ? -0xCE4 : juego == JuegoActivo::IE2 ? -0xF0C : 0;
        if (!distancia)
            return 0;
        Info inf = InfoDe(juego);
        const JuegoDatos *t = TablasDe(inf.tablas, *inf.perfil);
        if (!t)
            return 0;
        const GeometriaPlantilla &g = *inf.geo;
        u32 base = gBase;
        if (!base || ContarRegistros(reinterpret_cast<const u8 *>(base), TramoLegible(base, g.paso * MINIMO), g, *t) < MINIMO) {
            base = 0;
            for (u32 cursor = 0; cursor < inf.largo && !base;) {
                u32 inicio = inf.zona + cursor, largo = TROZO + g.paso * MINIMO;
                if (cursor + largo > inf.largo)
                    largo = inf.largo - cursor;
                u32 legible = TramoLegible(inicio, largo);
                if (legible >= g.paso * MINIMO) {
                    long off = BuscarPlantilla(reinterpret_cast<const u8 *>(inicio), legible, g, *t, MINIMO);
                    if (off >= 0)
                        base = inicio + static_cast<u32>(off);
                }
                cursor += legible >= TROZO ? TROZO : (legible ? legible : 0x1000);
            }
        }
        if (!base)
            return 0;
        u32 puntos = static_cast<u32>(static_cast<s32>(base) + distancia);
        return Escribible(puntos, 8) ? puntos : 0;
    }

    void CuidarPlantilla(JuegoActivo juego, bool llenarPePt, int factorExp) {
        gEstado[1] = static_cast<u32>(juego);
        gEstado[2] = (llenarPePt ? 1 : 0) | static_cast<u32>(factorExp) << 8;
        gEstado[7] = gEstado[7] + 1;
        if (juego == JuegoActivo::Ninguno || (!llenarPePt && factorExp <= 1)) {
            gJuego = juego;
            Olvidar();
            return;
        }
        if (juego != gJuego) {
            gJuego = juego;
            gCursor = gPausa = 0;
            Olvidar();
        }
        Info inf = InfoDe(juego);
        const JuegoDatos *t = TablasDe(inf.tablas, *inf.perfil);
        gEstado[3] = t ? static_cast<u32>(t->Fichas()) : 0xFFFFFFFF;
        gEstado[4] = gCursor;
        gEstado[5] = gBase;
        if (!t)
            return;
        const GeometriaPlantilla &g = *inf.geo;
        if (gBase) {
            u32 tramo = TramoLegible(gBase, g.paso * g.maxJugadores);
            u32 n = ContarRegistros(reinterpret_cast<const u8 *>(gBase), tramo, g, *t);
            gEstado[6] = n;
            if (n >= MINIMO) {
                Aplicar(g, *t, n, llenarPePt, factorExp);
                return;
            }
            Olvidar();  // otra partida, vuelta al título o cambio de juego: se vuelve a buscar
        }
        if (gPausa) {
            --gPausa;
            return;
        }
        if (gCursor >= inf.largo) {
            gCursor = 0;
            gPausa = PAUSA_TRAS_VUELTA;
            return;
        }
        u32 inicio = inf.zona + gCursor, largo = TROZO + g.paso * MINIMO;  // se solapa con el trozo siguiente
        if (gCursor + largo > inf.largo)
            largo = inf.largo - gCursor;
        u32 legible = TramoLegible(inicio, largo);
        if (legible >= g.paso * MINIMO) {
            long off = BuscarPlantilla(reinterpret_cast<const u8 *>(inicio), legible, g, *t, MINIMO);
            if (off >= 0) {
                gBase = inicio + static_cast<u32>(off);
                gMultiplicador.Olvidar();
            }
        }
        if (legible) {
            gCursor += legible >= TROZO ? TROZO : legible;
        } else {  // un hueco o algo que no se puede leer: se salta el bloque entero
            MemInfo info;
            PageInfo pagina;
            u32 salto = R_SUCCEEDED(svcQueryMemory(&info, &pagina, inicio)) ? info.base_addr + info.size - inicio : 0;
            gCursor += salto ? salto : 0x1000;
        }
    }

}
