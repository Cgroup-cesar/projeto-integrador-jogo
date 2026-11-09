// rodadas.c — dados e fluxo das 3 rodadas. (M2: versão mínima, ENTER conclui.)
#include "rodadas.h"
#include <string.h>

void rodadas_inicia(Estado *e, int numero) {
    memset(&e->rod, 0, sizeof(e->rod));
    e->rod.numero = numero;
    e->rod.etapa = ETAPA_DECIFRAR;
}

void rodadas_atualiza(Estado *e, float dt) {
    Rodada *r = &e->rod;
    if (r->etapa == ETAPA_CONCLUIDA) {
        r->concluida_t += dt;
        if (r->concluida_t > 1.0f) e->evento = EV_RODADA_CONCLUIDA;
        return;
    }
    if (e->jog.modo != MODO_TERMINAL) return;
    if (e->in.voltar) e->jog.modo = MODO_ANDANDO;
    else if (e->in.confirmar) r->etapa = ETAPA_CONCLUIDA;
}

void rodadas_desenha(const Estado *e) {
    (void)e;
}

void rodadas_desenha_rachaduras(const Estado *e) {
    (void)e;
}

bool rodadas_frase_atual(const Estado *e, FraseGlifos *frase) {
    (void)e;
    frase->n = 0;
    return false;
}

void rodadas_piloto(const Estado *e, Entrada *in) {
    if (e->rod.etapa != ETAPA_CONCLUIDA) in->confirmar = true;
}
