#include "TablasJuego.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <memory>

#include <3ds.h>

#include <CTRPluginFramework.hpp>

#include "Nucleo/LectorFA.hpp"
#include "Nucleo/Eventos.hpp"
#include "Nucleo/NombreSjis.hpp"
#include "Nucleo/Rivales.hpp"

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        const char *RUTA_SD = "/luma/titles/00040000000BB800/romfs/archive.fa";

        bool LeerDeArchivoSD(void *ctx, uint64_t off, uint32_t n, uint8_t *out) {
            File *f = static_cast<File *>(ctx);
            return f->Seek(static_cast<s64>(off), File::SET) == 0 && f->Read(out, n) == 0;
        }

        bool LeerDeRomFS(void *ctx, uint64_t off, uint32_t n, uint8_t *out) {
            FILE *f = static_cast<FILE *>(ctx);
            return fseek(f, static_cast<long>(off), SEEK_SET) == 0 && fread(out, 1, n, f) == n;
        }

        struct Fuente {
            File sd;
            FILE *romfs = nullptr;
            LectorFA fa;
            bool abierta = false;

            bool Abrir() {
                if (abierta)
                    return true;
                if (File::Exists(RUTA_SD) == 1 && File::Open(sd, RUTA_SD, File::READ) == 0
                    && fa.Abrir(LeerDeArchivoSD, &sd))
                    return abierta = true;
                if (R_SUCCEEDED(romfsMountSelf("romfs"))) {
                    romfs = fopen("romfs:/archive.fa", "rb");
                    if (romfs && fa.Abrir(LeerDeRomFS, romfs))
                        return abierta = true;
                }
                return false;
            }
        };

        Fuente &LaFuente() {
            static Fuente fuente;
            return fuente;
        }

    }

    namespace {
        // El menú (Continuidad, Fichajes) y el hilo de las ayudas piden tablas a la vez.
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
    }

    const JuegoDatos *TablasDe(const std::string &juego, const PerfilJuego &perfil) {
        Bloqueo bloqueo;
        static std::map<std::string, std::unique_ptr<JuegoDatos>> cache;
        auto it = cache.find(juego);
        if (it != cache.end())
            return it->second.get();
        Fuente &fuente = LaFuente();
        if (!fuente.Abrir())
            return nullptr;
        std::vector<uint8_t> b, s, l, n;
        std::string base = juego + "/data_iz/logic/";
        std::unique_ptr<JuegoDatos> datos(new JuegoDatos());
        if (!fuente.fa.Leer(base + "unitbase.dat", b) || !fuente.fa.Leer(base + "unitstat.dat", s)
            || !fuente.fa.Leer(base + "lvup_exp.dat", l) || !fuente.fa.Leer(base + "unitno.dat", n)
            || !datos->Construir(perfil, b, s, l, n))
            return nullptr;
        return (cache[juego] = std::move(datos)).get();
    }

    const std::vector<std::string> *TecnicasDe(const std::string &juego) {
        Bloqueo bloqueo;
        static std::map<std::string, std::vector<std::string>> cache;
        auto it = cache.find(juego);
        if (it != cache.end())
            return &it->second;
        // command.dat: IE1 registros de 24 B (nombre en +16), IE2 de 28 B (+20), IE3 y Ogro de 36 B (+24); el
        // nombre es un índice × 32 en command.STR.
        u32 paso = juego == "inazuma1" ? 24 : juego == "inazuma2" ? 28 : 36;
        u32 offNombre = juego == "inazuma1" ? 16 : juego == "inazuma2" ? 20 : 24;
        Fuente &fuente = LaFuente();
        std::vector<uint8_t> dat, str;
        std::string base = juego + "/data_iz/logic/";
        if (!fuente.Abrir() || !fuente.fa.Leer(base + "command.dat", dat) || !fuente.fa.Leer(base + "command.STR", str))
            return nullptr;
        std::vector<std::string> nombres(dat.size() / paso);
        for (size_t i = 0; i < nombres.size(); ++i) {
            u32 indice = dat[i * paso + offNombre] | dat[i * paso + offNombre + 1] << 8;
            size_t desde = static_cast<size_t>(indice) * 32;
            if (indice && desde < str.size())
                nombres[i] = DecodificarNombre(&str[desde], std::min<size_t>(64, str.size() - desde));
        }
        return &(cache[juego] = std::move(nombres));
    }

    const std::vector<uint8_t> *ElementosDeTecnicas() {
        Bloqueo bloqueo;
        static std::vector<uint8_t> elementos;
        static bool leidos = false;
        if (leidos)
            return &elementos;
        Fuente &fuente = LaFuente();
        std::vector<uint8_t> dat;
        if (!fuente.Abrir() || !fuente.fa.Leer("inazuma3/data_iz/logic/command.dat", dat))
            return nullptr;
        elementos.resize(dat.size() / 36);
        for (size_t i = 0; i < elementos.size(); ++i)
            elementos[i] = dat[i * 36 + 8];
        leidos = true;
        return &elementos;
    }

    const Partidos *PartidosDe(const std::string &juego) {
        Bloqueo bloqueo;
        static std::map<std::string, std::unique_ptr<Partidos>> cache;
        auto it = cache.find(juego);
        if (it != cache.end())
            return it->second.get();
        constexpr uint32_t EQUIPOS = 1024;  // el número de equipo más alto es 667 (Ogro)
        Fuente &fuente = LaFuente();
        std::unique_ptr<Partidos> p(new Partidos());
        std::vector<uint8_t> pkb;
        std::string base = juego + "/data_iz/logic/";
        if (!fuente.Abrir() || !fuente.fa.Leer(base + "games.dat", p->tabla) || !fuente.fa.Leer(base + "team.pkb", pkb)
            || p->tabla.size() < PARTIDO_PASO)
            return nullptr;
        uint32_t paso = juego == "inazuma1" || juego == "inazuma2" ? 320 : 352;
        p->extra.resize(EQUIPOS);
        ExtraDeEquipos(pkb.data(), static_cast<uint32_t>(pkb.size()), paso, p->extra.data(), EQUIPOS);
        return (cache[juego] = std::move(p)).get();
    }

    const std::vector<uint16_t> *AltasDe(const std::string &juego) {
        Bloqueo bloqueo;
        static std::map<std::string, std::vector<uint16_t>> cache;
        auto it = cache.find(juego);
        if (it != cache.end())
            return &it->second;
        Fuente &fuente = LaFuente();
        if (!fuente.Abrir())
            return nullptr;
        std::vector<uint16_t> altas;
        bool leidas = false;
        if (juego == "inazuma2") {  // los eventos de Ventisca van aparte
            leidas |= AltasDeEventos(fuente.fa, "inazuma2/data_iz/script", altas);
            leidas |= AltasDeEventos(fuente.fa, "inazuma2/data_iz_blizzard/script", altas);
        } else {
            leidas = AltasDeEventos(fuente.fa, juego + "/data_iz/script", altas);
        }
        if (!leidas)
            return nullptr;
        Normalizar(altas);
        return &(cache[juego] = std::move(altas));
    }

    std::string NombreDeJugador(const std::string &juego, const PerfilJuego &perfil, uint16_t numero) {
        const JuegoDatos *t = TablasDe(juego, perfil);
        int ficha = t ? t->Ficha(numero) : -1;
        if (ficha < 0)
            return Utils::Format("Jugador %u", numero);
        // Los nombres del IE1 y el IE2 llevan glifos de dos letras de la traducción: se enseña el del mismo
        // personaje (misma cara) en el IE3, que va en letras normales.
        if (juego == "inazuma1" || juego == "inazuma2") {
            const JuegoDatos *ie3 = TablasDe("inazuma3", PERFIL_IE3);
            for (size_t i = 0; ie3 && i < ie3->Fichas(); ++i)
                if (ie3->Jugable(i) && ie3->Cara(i) == t->Cara(static_cast<size_t>(ficha)))
                    return DecodificarNombre(ie3->NombreCrudo(i), ie3->AnchoNombre());
        }
        return DecodificarNombre(t->NombreCrudo(static_cast<size_t>(ficha)), t->AnchoNombre());
    }

}
