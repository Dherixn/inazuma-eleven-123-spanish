#pragma once

#include <string>
#include <vector>

#include <3ds.h>

namespace Definitiva {

    struct ArchivoPartida {
        std::string nombre;
        u64 tamano;
    };

    // Lista los archivos de partida de la recopilación (los tres juegos comparten el mismo save archive).
    bool ListarPartidas(std::vector<ArchivoPartida> &archivos);

    // Copia todos los archivos de partida a <carpeta>/<fecha>/ en la SD. Devuelve cuántos copió
    // y deja en `destino` la carpeta creada.
    int CopiarPartidasASD(const std::string &carpeta, std::string &destino);

    // Lee un archivo de partida entero ("/ina1_save.dat"). false si no existe.
    bool LeerPartida(const std::string &nombre, std::vector<u8> &datos);

    // Sobrescribe un archivo de partida que ya existe con el mismo tamaño y confirma la escritura.
    bool EscribirPartida(const std::string &nombre, const std::vector<u8> &datos);

}
