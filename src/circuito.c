// circuito.c — painéis de circuito. (M2: versão mínima, ENTER resolve.)
#include "circuito.h"
#include "nivel.h"
#include "ui.h"
#include "dialogo.h"
#include <string.h>

void circuito_inicia(Estado *e, int numero) {
    memset(&e->painel, 0, sizeof(e->painel));
    e->painel.numero = numero;
}

void circuito_atualiza(Estado *e, float dt) {
    Painel *p = &e->painel;
    if (p->resolvido) {
        p->resolvido_t += dt;
        if (nivel_na_sala_de_observacao(&e->nivel, e->jog.pos)) e->evento = EV_SAIDA_CONCLUIDA;
        return;
    }
    if (e->jog.modo != MODO_PAINEL) return;
    if (e->in.voltar) e->jog.modo = MODO_ANDANDO;
    else if (e->in.confirmar) {
        p->resolvido = true;
        e->jog.modo = MODO_ANDANDO;
    }
}

void circuito_desenha(const Estado *e) {
    if (e->jog.modo != MODO_PAINEL) return;
    float s = ui_escala();
    Rectangle r = {GetScreenWidth() * 0.5f - 300 * s, 140 * s, 600 * s, 300 * s};
    ui_painel(r, COR_AMBAR);
    ui_texto(e, dialogo_texto(e->painel.numero == 1 ? TXT_PAINEL1_TITULO : TXT_PAINEL2_TITULO), (Vector2){r.x + 20 * s, r.y + 20 * s}, 22 * s, COR_AMBAR);
    ui_texto(e, "(versão mínima do M2) ENTER resolve o painel", (Vector2){r.x + 20 * s, r.y + 80 * s}, 18 * s, COR_FOSFORO);
}

void circuito_desenha_3d(const Estado *e) {
    (void)e;
}

void circuito_piloto(const Estado *e, Entrada *in) {
    if (!e->painel.resolvido) in->confirmar = true;
}
