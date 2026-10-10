#include "TablasJuego.hpp"

#include <cstdio>
#include <map>
#include <memory>

#include <3ds.h>

#include <CTRPluginFramework.hpp>

#include "Nucleo/LectorFA.hpp"

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

}
