// Prueba en el PC de Nucleo/MemoriaEquipo (los mismos casos que tools/tests/unidad/test_definitiva_memoria.py).
//     g++ -std=c++17 -Wall -Wextra -D_GLIBCXX_ASSERTIONS -o /tmp/me pruebas/memoria_pc.cpp Sources/Nucleo/MemoriaEquipo.cpp Sources/Nucleo/Partida.cpp && /tmp/me

#include <cassert>
#include <cstdio>
#include <cstring>

#include "../Sources/Nucleo/MemoriaEquipo.hpp"

using namespace Definitiva;

static const char *TEXTO = "#partida=BF10331F\n#tiempo=17823\n"
                           "1=162:3,512;n=1;nv=60;xp=50000;p=1\n"
                           "2337=261:2,102,159:1;n=2337;nv=45;xp=18192\n";

static MemoriaEquipo Memoria() {
    MemoriaEquipo m;
    m.partida = 0xBF10331F;
    m.tiempo = 17823;
    const uint16_t tec[6] = {0x105, 0x66, 0x9F, 0, 0, 0};
    const uint8_t niv[6] = {2, 0, 1, 0, 0, 0};
    assert(m.Apuntar(2337, 2337, 45, 18192, tec, niv, 6));
    assert(!m.Apuntar(2337, 2337, 45, 18192, tec, niv, 6));
    Recuerdo r;
    r.numero = 1;
    r.nivel = 60;
    r.exp = 50000;
    r.tecnicas = {{0xA2, 3}, {0x200, 0}};
    m.Esperar(1, r);
    return m;
}

static JugadorPartida Jugador(uint8_t nivel, uint32_t exp, std::initializer_list<uint16_t> tecnicas, uint16_t numero = 2338) {
    JugadorPartida j;
    j.nivel = nivel;
    j.exp = exp;
    j.id = numero;
    size_t k = 0;
    for (uint16_t t : tecnicas)
        j.tecnicas[k++] = t;
    return j;
}

int main() {
    // Texto ida y vuelta.
    MemoriaEquipo m = Memoria();
    assert(m.Serializar() == TEXTO);
    MemoriaEquipo copia;
    copia.Cargar(TEXTO);
    assert(copia.Serializar() == TEXTO);

    // Se salta lo que no entiende.
    copia.Cargar("basura\n=1\n7=,,3,x:2,4:9\n#tiempo=abc\n8=;n=5;nv=120;xp=-3;p=2\n12=1,2\n");
    assert(copia.Serializar() == "#partida=00000000\n#tiempo=0\n7=3,4:9\n8=;n=5;nv=0;xp=0\n12=1,2\n");

    // Se queda el nivel más alto y quita el pendiente.
    const uint16_t recreado[6] = {0x105, 0, 0, 0, 0, 0};
    const uint8_t ceros[6] = {};
    assert(!m.Apuntar(2337, 2337, 30, 9000, recreado, ceros, 6));
    assert(m.Buscar(2337)->nivel == 45 && m.Buscar(2337)->exp == 18192);
    const uint16_t mark[6] = {0x10, 0, 0, 0, 0, 0};
    assert(m.Apuntar(1, 1, 25, 7000, mark, nullptr, 6));
    const Recuerdo *r1 = m.Buscar(1);
    assert(!r1->pendiente && r1->nivel == 60 && r1->exp == 50000 && r1->tecnicas.back().id == 0x10);
    const uint16_t ahora[6] = {0x10, 0xA2, 0, 0, 0, 0};
    auto perdidas = m.Perdidas(1, ahora, 6);
    assert(perdidas.size() == 1 && perdidas[0] == 0x200);

    // Como mucho 16 técnicas: se olvida la más antigua. Claves de 32 bits.
    MemoriaEquipo llena;
    for (uint16_t t = 1; t <= 20; ++t)
        llena.Apuntar(5, 5, 1, 0, &t, nullptr, 1);
    assert(llena.Buscar(5)->tecnicas.size() == 16 && llena.Buscar(5)->tecnicas.front().id == 5
           && llena.Buscar(5)->tecnicas.back().id == 20);
    const uint16_t siete = 7;
    llena.Apuntar(0xF00DCAFEu, 9, 1, 0, &siete, nullptr, 1);
    MemoriaEquipo releida;
    releida.Cargar(llena.Serializar());
    assert(releida.Buscar(0xF00DCAFEu) && releida.Buscar(0xF00DCAFEu)->tecnicas.front().id == 7);

    // Restaurar a quien la historia vuelve a crear.
    Recuerdo r;
    r.numero = 2338;
    r.nivel = 48;
    r.exp = 21000;
    r.tecnicas = {{0x105, 2}, {0x9F, 1}, {0x200, 0}};
    JugadorPartida j = Jugador(40, 900, {0x105});
    assert(Restaurar(j, r) && j.nivel == 48 && j.exp == 21000);
    const uint16_t esperadas[6] = {0x105, 0x9F, 0x200, 0, 0, 0};
    const uint8_t niveles[6] = {2, 1, 0, 0, 0, 0};
    assert(!std::memcmp(j.tecnicas, esperadas, sizeof esperadas) && !std::memcmp(j.nivelesTecnica, niveles, 6));
    assert(!Restaurar(j, r));
    JugadorPartida lleno = Jugador(40, 900, {1, 2, 3, 4, 5, 6});
    assert(Restaurar(lleno, r) && lleno.tecnicas[0] == 1 && lleno.tecnicas[5] == 6);

    // Al pendiente, aunque la historia lo traiga con más nivel: solo las técnicas.
    Recuerdo p;
    p.numero = 1;
    p.nivel = 20;
    p.exp = 100;
    p.pendiente = true;
    p.tecnicas = {{0xA2, 1}};
    JugadorPartida j2 = Jugador(25, 5000, {0x10}, 1);
    assert(Restaurar(j2, p) && j2.nivel == 25 && j2.exp == 5000 && j2.tecnicas[1] == 0xA2 && j2.nivelesTecnica[1] == 1);
    p.pendiente = false;
    assert(!Restaurar(j2, p));
    std::puts("MemoriaEquipo: OK");
    return 0;
}
