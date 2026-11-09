// compositor.c — resposta final por fichas e os dois finais. (M2: versão mínima.)
#include "compositor.h"
#include "terminal.h"
#include "ui.h"
#include "dialogo.h"
#include <string.h>

void compositor_inicia(Estado *e) {
    memset(&e->comp, 0, sizeof(e->comp));
}

// M2: [1] transmite "P E NÃO P" (contradição); [2] transmite "P OU Q" (satisfazível).
void compositor_atualiza(Estado *e, float dt) {
    (void)dt;
    const Ficha P = {FICHA_ATOMO, ATOMO_P}, Q = {FICHA_ATOMO, ATOMO_Q};
    Ficha bom[] = {P, {FICHA_E, 0}, {FICHA_NAO, 0}, P};
    Ficha ruim[] = {P, {FICHA_OU, 0}, Q};
    ErroLogica erro;
    if (e->in.tecla1) e->evento = regras_avalia_transmissao(bom, 4, &erro);
    if (e->in.tecla2) e->evento = regras_avalia_transmissao(ruim, 3, &erro);
}

void compositor_desenha(const Estado *e) {
    if (e->fase != FASE_RESPOSTA_FINAL) return;
    Rectangle area = terminal_moldura(e, dialogo_texto(TXT_TERMINAL_RESPONDA));
    float s = ui_escala();
    ui_texto(e, "(versão mínima do M2) [1] P E NÃO P   [2] P OU Q", (Vector2){area.x, area.y}, 18 * s, COR_FOSFORO);
}

void compositor_piloto(const Estado *e, Entrada *in, bool final_bom) {
    (void)e;
    if (final_bom) in->tecla1 = true;
    else in->tecla2 = true;
}

void compositor_inicia_fim(Estado *e) {
    memset(&e->fim, 0, sizeof(e->fim));
    e->fim.tela_final = true;
}

void compositor_atualiza_fim(Estado *e, float dt) {
    (void)e;
    (void)dt;
}

void compositor_desenha_fim(const Estado *e) {
    (void)e;
}
