#include "PlantillaEnJuego.hpp"

#include <CTRPluginFramework.hpp>

#include <cstring>

#include "Nucleo/MemoriaEquipo.hpp"
#include "Nucleo/Partida.hpp"
#include "Nucleo/Plantilla.hpp"
#include "Partidas.hpp"
#include "TablasJuego.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        struct Info {
            const char *tablas;
            const PerfilJuego *perfil;
            const GeometriaPlantilla *geo;
            const FormatoPartida *formato;  // el registro en memoria es el mismo que en la partida
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
            case JuegoActivo::IE1: return {"inazuma1", &PERFIL_IE1, &GEO_IE1, &FORMATO_IE1, HEAP, HEAP_LARGO};
            case JuegoActivo::IE2: return {"inazuma2", &PERFIL_IE2, &GEO_IE2, &FORMATO_IE2, HEAP, HEAP_LARGO};
            case JuegoActivo::IE3: return {"inazuma3", &PERFIL_IE3, &GEO_IE3, &FORMATO_IE3, LINEAL, LINEAL_LARGO};
            case JuegoActivo::Ogro: return {"inazuma3_ogre", &PERFIL_IE3, &GEO_IE3, &FORMATO_IE3, LINEAL, LINEAL_LARGO};
            default: return {nullptr, nullptr, nullptr, nullptr, 0, 0};
            }
        }

        JuegoActivo gJuego = JuegoActivo::Ninguno;
        u32 gBase = 0, gCursor = 0, gPausa = 0;
        MultiplicadorExp gMultiplicador;

        // Memoria del equipo de la partida que corre (una por archivo de partida, en la SD; ver
        // Nucleo/MemoriaEquipo.hpp). La partida se reconoce por la cabecera de su archivo, que el juego tiene en el
        // heap (con su suma bien): su identificador. Solo la cabecera: el tiempo de juego sale del archivo.
        constexpr u32 OFF_TECNICAS = 0x38, OFF_NIVELES_TECNICA = 0x44, NUM_HUECOS = 6;
        constexpr u32 GUARDAR_CADA = 100;  // llamadas (~10 s)
        MemoriaEquipo gMemoria;
        u32 gPartidaMemoria = 0;  // la de gMemoria; 0 = ninguna cargada
        bool gMemoriaSucia = false;
        u32 gLlamadasSinGuardar = 0;
        u32 gCabecera = 0, gCursorCabecera = 0, gPausaCabecera = 0;
        std::vector<u32> gCandidatas;

        // Pasión y amistad (u32 seguidos). IE3 y Ogro: en la copia de los datos de la partida del heap, que lleva el
        // identificador de la partida en +0xBA8 (como el archivo).
        constexpr u32 OFF_PUNTOS_DATOS = 0x4D4, OFF_IDENTIFICADOR_DATOS = 0xBA8, TOPE_PUNTOS = 99999;
        u32 gEspejo = 0, gPausaEspejo = 0;
        u32 gUltimosPuntos[2] = {};
        bool gPuntosVistos = false;

        // El menú (recuperar técnicas) y el hilo de las ayudas tocan la plantilla y la memoria a la vez.
        struct Cerrojo {
            LightLock cerrojo;
            Cerrojo() { LightLock_Init(&cerrojo); }
        };
        LightLock &ElCerrojo() {
            static Cerrojo c;
            return c.cerrojo;
        }
        struct Bloqueo {
            Bloqueo() { LightLock_Lock(&ElCerrojo()); }
            ~Bloqueo() { LightLock_Unlock(&ElCerrojo()); }
        };

        std::string RutaMemoria(u32 partida) {
            return Utils::Format("/Inazuma123Definitiva/equipo_%08lX.txt", static_cast<unsigned long>(partida));
        }

        bool EscribirArchivo(const MemoriaEquipo &m) {
            Directory::Create("/Inazuma123Definitiva");
            File f;
            if (File::Open(f, RutaMemoria(m.partida), File::RWC | File::TRUNCATE) != 0)
                return false;
            std::string texto = m.Serializar();
            bool ok = f.Write(texto.data(), texto.size()) == 0;
            f.Flush();
            f.Close();
            return ok;
        }

        // La de la SD, si es de esta partida: con un tiempo de juego mayor que el del último guardado es de una
        // partida anterior sobre el mismo archivo y se aparta (equipo_XXXXXXXX.txt.anterior). Tiempo 0 = no se sabe.
        MemoriaEquipo LeerArchivo(u32 partida, u32 tiempo) {
            MemoriaEquipo m;
            File f;
            std::string ruta = RutaMemoria(partida);
            if (File::Open(f, ruta, File::READ) == 0) {
                u64 largo = f.GetSize();
                std::string texto(largo > 0x40000 ? 0x40000 : static_cast<size_t>(largo), '\0');
                if (!texto.empty() && f.Read(&texto[0], texto.size()) == 0)
                    m.Cargar(texto);
                f.Close();
            }
            if (tiempo && m.tiempo > tiempo) {
                File::Remove(ruta + ".anterior");
                File::Rename(ruta, ruta + ".anterior");
                m = MemoriaEquipo();
            }
            m.partida = partida;
            if (tiempo > m.tiempo)
                m.tiempo = tiempo;
            return m;
        }

        void GuardarMemoria() {
            if (!gMemoriaSucia || !gPartidaMemoria)
                return;
            if (EscribirArchivo(gMemoria))
                gMemoriaSucia = false;
        }

        void CambiarMemoria(u32 partida, u32 tiempo) {
            if (partida == gPartidaMemoria)
                return;
            GuardarMemoria();
            gMemoriaSucia = false;
            gPartidaMemoria = partida;
            gMemoria = partida ? LeerArchivo(partida, tiempo) : MemoriaEquipo();
        }

        // El archivo de la partida con ese identificador (de los que puede tener ese juego). false si no está; en el
        // IE1 también (tres espacios en un archivo).
        bool ArchivoDePartida(JuegoActivo juego, u32 partida, std::vector<u8> &d) {
            static const char *const IE2[] = {"/ina2F_save.dat", "/ina2B_save.dat"};
            static const char *const IE3[] = {"/ina3S_save.dat", "/ina3B_save.dat"};
            static const char *const OGRO[] = {"/ina3O_save.dat"};
            const char *const *archivos = juego == JuegoActivo::IE2 ? IE2 : juego == JuegoActivo::IE3 ? IE3 : OGRO;
            size_t cuantos = juego == JuegoActivo::IE2 || juego == JuegoActivo::IE3 ? 2 : juego == JuegoActivo::Ogro ? 1 : 0;
            for (size_t k = 0; k < cuantos; ++k)
                if (LeerPartida(archivos[k], d) && d.size() >= DATOS_INICIO
                    && *reinterpret_cast<const u32 *>(&d[OFF_IDENTIFICADOR]) == partida)
                    return true;
            return false;
        }

        // Tiempo de juego del último guardado (en la RAM solo está la cabecera); 0 si no se sabe.
        u32 TiempoGuardado(JuegoActivo juego, u32 partida) {
            std::vector<u8> d;
            return ArchivoDePartida(juego, partida, d) ? *reinterpret_cast<const u32 *>(&d[OFF_TIEMPO]) : 0;
        }

        void LeerTecnicas(const u8 *r, bool u8Tecnicas, u16 salida[NUM_HUECOS]) {
            for (u32 k = 0; k < NUM_HUECOS; ++k)
                salida[k] = u8Tecnicas ? r[OFF_TECNICAS + k]
                                       : static_cast<u16>(r[OFF_TECNICAS + 2 * k] | r[OFF_TECNICAS + 2 * k + 1] << 8);
        }

        // Estado legible desde fuera (Azahar RPC, plugin/pruebas) para las pruebas: firma «PLNT», juego, opciones,
        // fichas de sus tablas, cursor de búsqueda, plantilla, jugadores, llamadas, partida de la memoria del equipo
        // y copia del archivo de partida en el heap.
        volatile u32 gEstado[10] = {0x544E4C50, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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

        // Fichas de tu plantilla desde `base`. En los partidos, el IE1 y el IE2 ponen detrás de tu plantilla, en la
        // misma tabla, las fichas de quien juega contigo sin ser del equipo y las de los rivales. En el IE2 llevan un
        // byte de rival (GeometriaPlantilla::offRival) y la cuenta se para sola. En el IE1 no llevan marca, pero cada
        // ficha tiene su puesto en la tabla en +0x7A y el número de jugadores de tu plantilla va 0x20 bytes antes de
        // la tabla (medido en un duelo: 6, los de la partida, con Nathan y seis rivales en los puestos 6 a 12). Si la
        // búsqueda cayó en una ficha de detrás (pasa si las tuyas no cuadran), no hay plantilla.
        constexpr u32 OFF_PUESTO_IE1 = 0x7A, OFF_CUENTA_IE1 = 0x20;

        u32 Contar(u32 base, const GeometriaPlantilla &g, const JuegoDatos &t, bool estricto, u32 largo) {
            u32 n = ContarRegistros(reinterpret_cast<const u8 *>(base), TramoLegible(base, largo), g, t, estricto);
            if (&g != &GEO_IE1 || !n)
                return n;
            u32 puesto = reinterpret_cast<const u8 *>(base)[OFF_PUESTO_IE1];
            u32 tabla = base - puesto * g.paso;
            if (puesto >= g.maxJugadores || TramoLegible(tabla - OFF_CUENTA_IE1, OFF_CUENTA_IE1) != OFF_CUENTA_IE1)
                return n;
            u32 cuenta = *reinterpret_cast<const u8 *>(tabla - OFF_CUENTA_IE1);
            if (cuenta < 1 || cuenta > g.maxJugadores)
                return n;
            if (puesto >= cuenta)
                return 0;
            return n < cuenta - puesto ? n : cuenta - puesto;
        }

        // La partida cambia (otra cargada, vuelta al título...): se guarda la memoria y se vuelve a buscar todo.
        void Olvidar() {
            gBase = 0;
            gMultiplicador.Olvidar();
            gEspejo = gPausaEspejo = 0;
            gPuntosVistos = false;
            gCabecera = gCursorCabecera = gPausaCabecera = 0;
            gCandidatas.clear();
            CambiarMemoria(0, 0);
        }

        u32 U32En(u32 dir) { return *reinterpret_cast<const u32 *>(dir); }

        // ¿Hay en `dir` una cabecera de partida de este juego con su suma bien? (IE1 «INAZUMA_ELEVEN», IE2
        // «…2», IE3 «…3»; el Ogro, cualquiera de las otras).
        bool EsCabecera(u32 dir, JuegoActivo juego) {
            const char *p = reinterpret_cast<const char *>(dir);
            if (std::memcmp(p + 4, "INAZUMA_ELEVEN", 14))
                return false;
            char v = p[18];
            bool version = juego == JuegoActivo::IE1 ? v == 0 : juego == JuegoActivo::IE2 ? v == '2' : v != 0 && v != '2';
            return version && Crc32(reinterpret_cast<const uint8_t *>(dir + 4), CABECERA_LARGO - 4) == U32En(dir);
        }

        // Cuántos de los primeros de la plantilla están entre los registros del archivo de esa partida (espacio 1).
        u32 Parecido(JuegoActivo juego, u32 partida, const Info &inf, u32 n) {
            std::vector<u8> d;
            const FormatoPartida &f = *inf.formato;
            if (!ArchivoDePartida(juego, partida, d) || d.size() < f.jugadoresInicio + f.maxJugadores * f.jugadorPaso)
                return 0;
            u32 total = d[OFF_NUM_JUGADORES] <= f.maxJugadores ? d[OFF_NUM_JUGADORES] : 0, iguales = 0;
            for (u32 i = 0; i < n && i < 16; ++i) {
                u16 numero = *reinterpret_cast<const u16 *>(gBase + i * inf.geo->paso + 4);
                for (u32 k = 0; k < total; ++k)
                    if (*reinterpret_cast<const u16 *>(&d[f.jugadoresInicio + k * f.jugadorPaso + 4]) == numero) {
                        ++iguales;
                        break;
                    }
            }
            return iguales;
        }

        // Un trozo más de la búsqueda de la copia del archivo de partida en el heap (con la plantilla ya encontrada).
        // Al acabar la vuelta se queda la que más se parece a la plantilla y se carga su memoria.
        void BuscarCabecera(JuegoActivo juego, const Info &inf, u32 n) {
            if (gCabecera) {
                if (TramoLegible(gCabecera, CABECERA_LARGO + 0x80) == CABECERA_LARGO + 0x80 && EsCabecera(gCabecera, juego))
                    return;
                gCabecera = 0;  // se ha movido (al guardar, por ejemplo): se vuelve a buscar sin soltar la memoria
            }
            if (gPausaCabecera) {
                --gPausaCabecera;
                return;
            }
            if (gCursorCabecera < HEAP_LARGO) {
                u32 inicio = HEAP + gCursorCabecera;
                u32 legible = TramoLegible(inicio, TROZO + CABECERA_LARGO);
                for (u32 off = 0; off + CABECERA_LARGO + 0x80 <= legible; off += 4)
                    if (U32En(inicio + off + 4) == 0x5A414E49 && EsCabecera(inicio + off, juego))  // «INAZ»
                        gCandidatas.push_back(inicio + off);
                if (legible) {
                    gCursorCabecera += legible >= TROZO ? TROZO : legible;
                } else {
                    MemInfo info;
                    PageInfo pagina;
                    u32 salto = R_SUCCEEDED(svcQueryMemory(&info, &pagina, inicio)) ? info.base_addr + info.size - inicio : 0;
                    gCursorCabecera += salto ? salto : 0x1000;
                }
                return;
            }
            // Varias partidas distintas (las cabeceras de las dos versiones, por ejemplo): la del archivo que más se
            // parece a la plantilla.
            std::vector<u32> validas;
            for (u32 c : gCandidatas)
                if (TramoLegible(c, CABECERA_LARGO + 0x80) == CABECERA_LARGO + 0x80 && EsCabecera(c, juego))
                    validas.push_back(c);
            u32 mejor = 0, puntos = 0;
            for (u32 c : validas) {
                bool otra = !mejor || U32En(c + OFF_IDENTIFICADOR) != U32En(mejor + OFF_IDENTIFICADOR);
                u32 p = otra && validas.size() > 1 ? Parecido(juego, U32En(c + OFF_IDENTIFICADOR), inf, n) : 0;
                if (!mejor || p > puntos) {
                    mejor = c;
                    puntos = p;
                }
            }
            bool varias = false;
            for (u32 c : validas)
                varias |= U32En(c + OFF_IDENTIFICADOR) != U32En(validas[0] + OFF_IDENTIFICADOR);
            gCandidatas.clear();
            gCursorCabecera = 0;
            if (!mejor || (varias && !puntos)) {
                gPausaCabecera = PAUSA_TRAS_VUELTA;
                return;
            }
            gCabecera = mejor;
        }

        // IE3 y Ogro: la copia de los datos de la partida en el heap, por el identificador en su +0xBA8 (con la
        // cabecera ya encontrada). Una vuelta entera al heap la primera vez; después solo se comprueba.
        u32 PuntosIE3() {
            if (!gCabecera)
                return 0;
            u32 partida = U32En(gCabecera + OFF_IDENTIFICADOR);
            if (gEspejo && TramoLegible(gEspejo + OFF_IDENTIFICADOR_DATOS, 4) == 4
                && U32En(gEspejo + OFF_IDENTIFICADOR_DATOS) == partida)
                return gEspejo + OFF_PUNTOS_DATOS;
            gEspejo = 0;
            if (gPausaEspejo) {
                --gPausaEspejo;
                return 0;
            }
            gPausaEspejo = PAUSA_TRAS_VUELTA;
            for (u32 cursor = 0; cursor < HEAP_LARGO;) {
                u32 inicio = HEAP + cursor, legible = TramoLegible(inicio, TROZO);
                for (u32 off = 0; off + 4 <= legible; off += 4) {
                    u32 dir = inicio + off;
                    if (U32En(dir) != partida || dir == gCabecera + OFF_IDENTIFICADOR || dir < HEAP + OFF_IDENTIFICADOR_DATOS)
                        continue;
                    u32 espejo = dir - OFF_IDENTIFICADOR_DATOS;
                    if (TramoLegible(espejo + OFF_PUNTOS_DATOS, 8) == 8 && U32En(espejo + OFF_PUNTOS_DATOS) <= TOPE_PUNTOS
                        && U32En(espejo + OFF_PUNTOS_DATOS + 4) <= TOPE_PUNTOS) {
                        gEspejo = espejo;
                        return gEspejo + OFF_PUNTOS_DATOS;
                    }
                }
                if (legible) {
                    cursor += legible >= TROZO ? TROZO : legible;
                } else {
                    MemInfo info;
                    PageInfo pagina;
                    u32 salto = R_SUCCEEDED(svcQueryMemory(&info, &pagina, inicio)) ? info.base_addr + info.size - inicio : 0;
                    cursor += salto ? salto : 0x1000;
                }
            }
            return 0;
        }

        u32 PuntosDe(JuegoActivo juego) {
            if (juego == JuegoActivo::IE3 || juego == JuegoActivo::Ogro)
                return PuntosIE3();
            s32 distancia = juego == JuegoActivo::IE1 ? -0xCE4 : juego == JuegoActivo::IE2 ? -0xF0C : 0;
            u32 puntos = distancia && gBase ? static_cast<u32>(static_cast<s32>(gBase) + distancia) : 0;
            return puntos && Escribible(puntos, 8) ? puntos : 0;
        }

        // Como el multiplicador de EXP: lo que sube la pasión o la amistad se multiplica (lo que baja, al fichar,
        // no se toca).
        void MultiplicarPuntos(JuegoActivo juego, int factor) {
            u32 dir = PuntosDe(juego);
            if (!dir || !Escribible(dir, 8)) {
                gPuntosVistos = false;
                return;
            }
            u32 *puntos = reinterpret_cast<u32 *>(dir);
            for (u32 k = 0; k < 2; ++k) {
                u32 ahora = puntos[k];
                if (gPuntosVistos && ahora > gUltimosPuntos[k] && ahora <= TOPE_PUNTOS) {
                    u64 nuevo = ahora + static_cast<u64>(ahora - gUltimosPuntos[k]) * static_cast<u64>(factor - 1);
                    ahora = static_cast<u32>(nuevo > TOPE_PUNTOS ? TOPE_PUNTOS : nuevo);
                    puntos[k] = ahora;
                }
                gUltimosPuntos[k] = ahora;
            }
            gPuntosVistos = true;
        }

        void EscribirRegistro(u8 *r, const JugadorPartida &j, const FormatoPartida &f) {
            std::vector<uint8_t> bytes = EscribirJugador(j, f);
            std::memcpy(r, bytes.data(), bytes.size());
        }

        void Aplicar(JuegoActivo juego, const Info &inf, const JuegoDatos &t, u32 n, bool llenar, int factor, bool recordar) {
            const GeometriaPlantilla &g = *inf.geo;
            const FormatoPartida &f = *inf.formato;
            // Devolver lo perdido solo en IE2, IE3 y Ogro: el IE1 tiene tres espacios en un archivo y la memoria no
            // sabría de cuál es.
            bool restaurar = recordar && gPartidaMemoria && juego != JuegoActivo::IE1;
            for (u32 i = 0; i < n; ++i) {
                u8 *r = reinterpret_cast<u8 *>(gBase + i * g.paso);
                if (recordar && gPartidaMemoria) {
                    u16 numero = static_cast<u16>(r[4] | r[5] << 8), tecnicas[NUM_HUECOS];
                    u8 niveles[NUM_HUECOS] = {}, nivel = r[g.offNivel];
                    u32 exp = r[0] | r[1] << 8 | r[2] << 16 | static_cast<u32>(r[3]) << 24;
                    LeerTecnicas(r, f.tecnicasU8, tecnicas);
                    if (!f.tecnicasU8)
                        std::memcpy(niveles, r + OFF_NIVELES_TECNICA, NUM_HUECOS);
                    uint32_t clave = t.Clave(numero);
                    const Recuerdo *antes = gMemoria.Buscar(clave);
                    if (restaurar && antes && (antes->pendiente || antes->nivel > nivel)
                        && Escribible(reinterpret_cast<u32>(r), g.paso)) {
                        JugadorPartida j = LeerJugador(r, f);
                        if (Restaurar(j, *antes)) {
                            EscribirRegistro(r, j, f);
                            gMultiplicador.Olvidar();  // la EXP que sube aquí no es ganada en un partido
                            nivel = j.nivel;
                            exp = j.exp;
                            std::memcpy(tecnicas, j.tecnicas, sizeof tecnicas);
                            std::memcpy(niveles, j.nivelesTecnica, sizeof niveles);
                        }
                    }
                    if (gMemoria.Apuntar(clave, numero, nivel, exp, tecnicas, f.tecnicasU8 ? nullptr : niveles, NUM_HUECOS))
                        gMemoriaSucia = true;
                }
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

    std::vector<TecnicasPerdidas> BuscarTecnicasPerdidas() {
        Bloqueo bloqueo;
        std::vector<TecnicasPerdidas> salida;
        Info inf = InfoDe(gJuego);
        const JuegoDatos *t = inf.tablas ? TablasDe(inf.tablas, *inf.perfil) : nullptr;
        if (!gBase || !t || !gPartidaMemoria)
            return salida;
        const GeometriaPlantilla &g = *inf.geo;
        u32 n = Contar(gBase, g, *t, false, g.paso * g.maxJugadores);
        for (u32 i = 0; i < n; ++i) {
            const u8 *r = reinterpret_cast<const u8 *>(gBase + i * g.paso);
            u16 tecnicas[NUM_HUECOS];
            LeerTecnicas(r, &g == &GEO_IE1, tecnicas);
            u16 numero = static_cast<u16>(r[4] | r[5] << 8);
            std::vector<u16> perdidas = gMemoria.Perdidas(t->Clave(numero), tecnicas, NUM_HUECOS);
            if (!perdidas.empty())
                salida.push_back({numero, perdidas});
        }
        return salida;
    }

    int DevolverTecnicas(uint16_t numero, const std::vector<uint16_t> &tecnicas) {
        Bloqueo bloqueo;
        Info inf = InfoDe(gJuego);
        const JuegoDatos *t = inf.tablas ? TablasDe(inf.tablas, *inf.perfil) : nullptr;
        if (!gBase || !t)
            return -1;
        const GeometriaPlantilla &g = *inf.geo;
        bool u8Tecnicas = &g == &GEO_IE1;
        u32 n = Contar(gBase, g, *t, false, g.paso * g.maxJugadores);
        for (u32 i = 0; i < n; ++i) {
            u8 *r = reinterpret_cast<u8 *>(gBase + i * g.paso);
            if (static_cast<u16>(r[4] | r[5] << 8) != numero || !Escribible(reinterpret_cast<u32>(r), g.paso))
                continue;
            int puestas = 0;
            for (u16 tecnica : tecnicas) {
                u16 actuales[NUM_HUECOS];
                LeerTecnicas(r, u8Tecnicas, actuales);
                for (u32 k = 0; k < NUM_HUECOS; ++k) {
                    if (actuales[k])
                        continue;
                    if (u8Tecnicas) {
                        r[OFF_TECNICAS + k] = static_cast<u8>(tecnica);
                    } else {
                        r[OFF_TECNICAS + 2 * k] = static_cast<u8>(tecnica);
                        r[OFF_TECNICAS + 2 * k + 1] = static_cast<u8>(tecnica >> 8);
                        r[OFF_NIVELES_TECNICA + k] = 0;  // como las aprendidas en el traspaso
                    }
                    ++puestas;
                    break;
                }
            }
            return puestas;
        }
        return -1;
    }

    MemoriaEquipo LeerMemoriaEquipo(uint32_t partida, uint32_t tiempo) {
        Bloqueo bloqueo;
        if (partida && partida == gPartidaMemoria)
            return gMemoria;
        return LeerArchivo(partida, tiempo);
    }

    bool EscribirMemoriaEquipo(const MemoriaEquipo &m) {
        Bloqueo bloqueo;
        if (!m.partida)
            return false;
        if (m.partida == gPartidaMemoria) {
            gMemoria = m;
            gMemoriaSucia = false;
        }
        return EscribirArchivo(m);
    }

    void OlvidarPuntos() {
        Bloqueo bloqueo;
        gPuntosVistos = false;
    }

    u32 DireccionPuntos() {
        JuegoActivo juego = gJuego;
        if (juego == JuegoActivo::IE3 || juego == JuegoActivo::Ogro) {
            Bloqueo bloqueo;
            gPausaEspejo = 0;  // desde el menú se busca ya
            return PuntosIE3();
        }
        s32 distancia = juego == JuegoActivo::IE1 ? -0xCE4 : juego == JuegoActivo::IE2 ? -0xF0C : 0;
        if (!distancia)
            return 0;
        Info inf = InfoDe(juego);
        const JuegoDatos *t = TablasDe(inf.tablas, *inf.perfil);
        if (!t)
            return 0;
        const GeometriaPlantilla &g = *inf.geo;
        u32 base = gBase;
        if (!base || Contar(base, g, *t, true, g.paso * MINIMO) < MINIMO) {
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

    void CuidarPlantilla(JuegoActivo juego, bool llenarPePt, int factorExp, bool recordar, int factorPuntos) {
        Bloqueo bloqueo;
        gEstado[1] = static_cast<u32>(juego);
        gEstado[2] = (llenarPePt ? 1 : 0) | static_cast<u32>(factorExp) << 8 | (recordar ? 1u : 0u) << 16;
        gEstado[7] = gEstado[7] + 1;
        if (++gLlamadasSinGuardar >= GUARDAR_CADA) {
            gLlamadasSinGuardar = 0;
            GuardarMemoria();
        }
        if (!recordar)
            CambiarMemoria(0, 0);
        if (juego == JuegoActivo::Ninguno || (!llenarPePt && factorExp <= 1 && !recordar && factorPuntos <= 1)) {
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
            u32 n = Contar(gBase, g, *t, false, g.paso * g.maxJugadores);
            gEstado[6] = n;
            if (n >= MINIMO) {
                if (recordar) {
                    BuscarCabecera(juego, inf, n);
                    u32 partida = gCabecera ? U32En(gCabecera + OFF_IDENTIFICADOR) : 0;
                    if (partida && partida != gPartidaMemoria)
                        CambiarMemoria(partida, TiempoGuardado(juego, partida));
                }
                gEstado[8] = gPartidaMemoria;
                gEstado[9] = gCabecera;
                Aplicar(juego, inf, *t, n, llenarPePt, factorExp, recordar);
                if (factorPuntos > 1) {
                    if (juego == JuegoActivo::IE3 || juego == JuegoActivo::Ogro)
                        BuscarCabecera(juego, inf, n);  // la copia de los datos se reconoce por su identificador
                    MultiplicarPuntos(juego, factorPuntos);
                } else {
                    gPuntosVistos = false;
                }
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
