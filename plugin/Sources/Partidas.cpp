#include "Partidas.hpp"

#include <ctime>

#include <CTRPluginFramework.hpp>

using namespace CTRPluginFramework;

namespace Definitiva {

    namespace {

        bool AbrirArchivoPartida(FS_Archive &archivo) {
            return R_SUCCEEDED(FSUSER_OpenArchive(&archivo, ARCHIVE_SAVEDATA, fsMakePath(PATH_EMPTY, "")));
        }

        bool Listar(FS_Archive archivo, std::vector<ArchivoPartida> &salida) {
            Handle dir;
            if (R_FAILED(FSUSER_OpenDirectory(&dir, archivo, fsMakePath(PATH_ASCII, "/"))))
                return false;
            FS_DirectoryEntry entradas[16];
            u32 leidas = 0;
            do {
                if (R_FAILED(FSDIR_Read(dir, &leidas, 16, entradas)))
                    break;
                for (u32 i = 0; i < leidas; ++i) {
                    if (entradas[i].attributes & FS_ATTRIBUTE_DIRECTORY)
                        continue;
                    std::string nombre;
                    Utils::ConvertUTF16ToUTF8(nombre, reinterpret_cast<const char16_t *>(entradas[i].name));
                    salida.push_back({nombre, entradas[i].fileSize});
                }
            } while (leidas > 0);
            FSDIR_Close(dir);
            return true;
        }

        bool Copiar(FS_Archive archivo, const ArchivoPartida &info, const std::string &destino) {
            Handle origen;
            std::string ruta = "/" + info.nombre;
            if (R_FAILED(FSUSER_OpenFile(&origen, archivo, fsMakePath(PATH_ASCII, ruta.c_str()), FS_OPEN_READ, 0)))
                return false;
            File salida;
            if (File::Open(salida, destino + "/" + info.nombre, File::RWC | File::TRUNCATE) != 0) {
                FSFILE_Close(origen);
                return false;
            }
            constexpr u32 TROZO = 0x8000;
            u8 *buffer = new u8[TROZO];
            u64 offset = 0;
            bool ok = true;
            while (offset < info.tamano) {
                u32 pedir = info.tamano - offset > TROZO ? TROZO : static_cast<u32>(info.tamano - offset);
                u32 leidos = 0;
                if (R_FAILED(FSFILE_Read(origen, &leidos, offset, buffer, pedir)) || leidos == 0
                    || salida.Write(buffer, leidos) != 0) {
                    ok = false;
                    break;
                }
                offset += leidos;
            }
            delete[] buffer;
            salida.Flush();
            salida.Close();
            FSFILE_Close(origen);
            return ok;
        }

    }

    bool ListarPartidas(std::vector<ArchivoPartida> &archivos) {
        FS_Archive archivo;
        if (!AbrirArchivoPartida(archivo))
            return false;
        bool ok = Listar(archivo, archivos);
        FSUSER_CloseArchive(archivo);
        return ok;
    }

    int CopiarPartidasASD(const std::string &carpeta, std::string &destino) {
        FS_Archive archivo;
        if (!AbrirArchivoPartida(archivo))
            return -1;
        std::vector<ArchivoPartida> archivos;
        if (!Listar(archivo, archivos)) {
            FSUSER_CloseArchive(archivo);
            return -1;
        }
        time_t ahora = time(nullptr);
        char fecha[32];
        strftime(fecha, sizeof(fecha), "%Y-%m-%d_%H%M%S", localtime(&ahora));
        Directory::Create(carpeta);
        destino = carpeta + "/" + fecha;
        Directory::Create(destino);
        int copiados = 0;
        for (const ArchivoPartida &info : archivos)
            if (Copiar(archivo, info, destino))
                ++copiados;
        FSUSER_CloseArchive(archivo);
        return copiados;
    }

}
