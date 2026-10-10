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
        destino = carpeta + "/" + fecha;
        // Directory::Create no crea carpetas intermedias: se crea la ruta paso a paso.
        for (size_t corte = destino.find('/', 1); ; corte = destino.find('/', corte + 1)) {
            Directory::Create(destino.substr(0, corte));
            if (corte == std::string::npos)
                break;
        }
        if (!Directory::IsExists(destino)) {
            FSUSER_CloseArchive(archivo);
            return -1;
        }
        int copiados = 0;
        for (const ArchivoPartida &info : archivos)
            if (Copiar(archivo, info, destino))
                ++copiados;
        FSUSER_CloseArchive(archivo);
        return copiados;
    }

    bool LeerPartida(const std::string &nombre, std::vector<u8> &datos) {
        FS_Archive archivo;
        if (!AbrirArchivoPartida(archivo))
            return false;
        Handle h;
        bool ok = false;
        if (R_SUCCEEDED(FSUSER_OpenFile(&h, archivo, fsMakePath(PATH_ASCII, nombre.c_str()), FS_OPEN_READ, 0))) {
            u64 tamano = 0;
            if (R_SUCCEEDED(FSFILE_GetSize(h, &tamano)) && tamano > 0 && tamano <= 0x100000) {
                datos.resize(static_cast<size_t>(tamano));
                u32 leidos = 0;
                ok = R_SUCCEEDED(FSFILE_Read(h, &leidos, 0, datos.data(), static_cast<u32>(tamano)))
                     && leidos == tamano;
            }
            FSFILE_Close(h);
        }
        FSUSER_CloseArchive(archivo);
        return ok;
    }

    bool EscribirPartida(const std::string &nombre, const std::vector<u8> &datos) {
        FS_Archive archivo;
        if (!AbrirArchivoPartida(archivo))
            return false;
        Handle h;
        bool ok = false;
        // Solo se sobrescriben archivos que ya existen y con su mismo tamaño: nada de crear ni truncar.
        if (R_SUCCEEDED(FSUSER_OpenFile(&h, archivo, fsMakePath(PATH_ASCII, nombre.c_str()), FS_OPEN_WRITE, 0))) {
            u64 tamano = 0;
            if (R_SUCCEEDED(FSFILE_GetSize(h, &tamano)) && tamano == datos.size()) {
                u32 escritos = 0;
                ok = R_SUCCEEDED(FSFILE_Write(h, &escritos, 0, datos.data(), static_cast<u32>(datos.size()),
                                              FS_WRITE_FLUSH))
                     && escritos == datos.size();
            }
            FSFILE_Close(h);
        }
        // Las partidas de 3DS no quedan escritas hasta confirmar el archive.
        if (ok)
            ok = R_SUCCEEDED(FSUSER_ControlArchive(archivo, ARCHIVE_ACTION_COMMIT_SAVE_DATA, nullptr, 0, nullptr, 0));
        FSUSER_CloseArchive(archivo);
        return ok;
    }

}
