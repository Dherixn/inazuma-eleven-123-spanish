#include "DatosDestino.hpp"

namespace Definitiva {

#include "DatosDestino.inc"

    namespace {
        constexpr uint8_t NIVEL_NO_POR_NIVEL = 100;

        uint32_t ExpCb(uint16_t numero, uint8_t nivel, void *ctx) {
            return static_cast<const JuegoDestino *>(ctx)->ExpMinima(numero, nivel);
        }
        int AprendidasCb(uint16_t numero, uint8_t nivel, uint16_t salida[4], void *ctx) {
            return static_cast<const JuegoDestino *>(ctx)->Aprendidas(numero, nivel, salida);
        }
        uint32_t ClaveCb(uint16_t numero, void *ctx) {
            return static_cast<const JuegoDestino *>(ctx)->Clave(numero);
        }
    }

    const FichaDestino *JuegoDestino::Buscar(uint16_t numero) const {
        size_t lo = 0, hi = cantidad;
        while (lo < hi) {
            size_t medio = (lo + hi) / 2;
            if (fichas[medio].numero < numero)
                lo = medio + 1;
            else
                hi = medio;
        }
        return (lo < cantidad && fichas[lo].numero == numero) ? &fichas[lo] : nullptr;
    }

    uint32_t JuegoDestino::ExpMinima(uint16_t numero, uint8_t nivel) const {
        const FichaDestino *f = Buscar(numero);
        if (!f || nivel <= 1 || f->curva >= numCurvas)
            return 0;
        return curvas[f->curva][(nivel > 99 ? 99 : nivel) - 2];
    }

    int JuegoDestino::Aprendidas(uint16_t numero, uint8_t nivel, uint16_t salida[4]) const {
        const FichaDestino *f = Buscar(numero);
        int n = 0;
        if (!f)
            return 0;
        for (int k = 0; k < 4; ++k)
            if (f->tec[k] && f->nv[k] < NIVEL_NO_POR_NIVEL && f->nv[k] <= nivel)
                salida[n++] = f->tec[k];
        return n;
    }

    uint32_t JuegoDestino::Clave(uint16_t numero) const {
        const FichaDestino *f = Buscar(numero);
        return f ? f->clave : 0x80000000u | numero;
    }

    void JuegoDestino::Preparar(Opciones &op) const {
        op.expDeNivel = ExpCb;
        op.aprendidas = AprendidasCb;
        op.clave = ClaveCb;
        op.contexto = const_cast<JuegoDestino *>(this);
    }

    const JuegoDestino DESTINO_IE2 = {FICHAS_IE2, sizeof(FICHAS_IE2) / sizeof(FICHAS_IE2[0]), CURVAS_IE2,
                                      sizeof(CURVAS_IE2) / sizeof(CURVAS_IE2[0])};
    const JuegoDestino DESTINO_IE3 = {FICHAS_IE3, sizeof(FICHAS_IE3) / sizeof(FICHAS_IE3[0]), CURVAS_IE3,
                                      sizeof(CURVAS_IE3) / sizeof(CURVAS_IE3[0])};
    const JuegoDestino DESTINO_OGRO = {FICHAS_OGRO, sizeof(FICHAS_OGRO) / sizeof(FICHAS_OGRO[0]), CURVAS_OGRO,
                                       sizeof(CURVAS_OGRO) / sizeof(CURVAS_OGRO[0])};

}
