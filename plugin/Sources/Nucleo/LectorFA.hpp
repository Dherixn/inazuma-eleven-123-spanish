#pragma once

// Lector mínimo del contenedor archive.fa (magic B123, ver tools/src/ie123kit/ie3/comun/b123.py):
// localiza un archivo por su ruta y lo lee. Sin dependencias de la 3DS: la lectura del archivo real se
// inyecta (SD, RomFS del juego o un archivo del PC en las pruebas).

#include <cstdint>
#include <string>
#include <vector>

namespace Definitiva {

    // Lee `largo` bytes desde `offset` en `salida`; false si no puede.
    typedef bool (*LeerBytes)(void *contexto, uint64_t offset, uint32_t largo, uint8_t *salida);

    class LectorFA {
    public:
        bool Abrir(LeerBytes leer, void *contexto);
        // Lee el archivo `ruta` ("inazuma2/data_iz/logic/unitbase.dat"); false si no está.
        bool Leer(const std::string &ruta, std::vector<uint8_t> &salida) const;

    private:
        LeerBytes leer_ = nullptr;
        void *contexto_ = nullptr;
        uint32_t offDirs_ = 0, offArchivos_ = 0, offNombres_ = 0, offDatos_ = 0, numDirs_ = 0;
        std::vector<uint8_t> dirs_;
        std::vector<uint8_t> nombres_;
        std::string Nombre(uint32_t offset) const;
    };

}
