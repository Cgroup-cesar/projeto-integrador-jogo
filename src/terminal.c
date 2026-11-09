// terminal.c — interface do ANALISADOR SEMÂNTICO SNI-85. (M2: versão mínima.)
#include "terminal.h"
#include "ui.h"
#include "dialogo.h"
#include <stdio.h>

void terminal_atualiza(Estado *e, float dt) {
    (void)e;
    (void)dt;
}

Rectangle terminal_moldura(const Estado *e, const char *cabecalho) {
    float s = ui_escala();
    Rectangle r = {GetScreenWidth() - 600 * s, 80 * s, 570 * s, 470 * s};
    ui_painel(r, COR_FOSFORO);
    ui_texto(e, dialogo_texto(TXT_TERMINAL_TITULO), (Vector2){r.x + 18 * s, r.y + 14 * s}, 22 * s, COR_FOSFORO);
    ui_texto(e, cabecalho, (Vector2){r.x + 18 * s, r.y + 42 * s}, 18 * s, Fade(COR_FOSFORO, 0.75f));
    DrawLine((int)(r.x + 12 * s), (int)(r.y + 68 * s), (int)(r.x + r.width - 12 * s), (int)(r.y + 68 * s), Fade(COR_FOSFORO, 0.4f));
    return (Rectangle){r.x + 18 * s, r.y + 80 * s, r.width - 36 * s, r.height - 96 * s};
}

void terminal_desenha(const Estado *e) {
    if (e->jog.modo != MODO_TERMINAL || !regras_fase_eh_rodada(e->fase)) return;
    char cabecalho[96];
    snprintf(cabecalho, sizeof(cabecalho), "RODADA %d/3 · PLACAR: %03d", e->rod.numero, e->placar);
    Rectangle area = terminal_moldura(e, cabecalho);
    float s = ui_escala();
    ui_texto(e, "(versão mínima do M2) ENTER conclui a rodada", (Vector2){area.x, area.y}, 18 * s, COR_FOSFORO);
    ui_texto(e, dialogo_texto(TXT_TERMINAL_SAIR), (Vector2){area.x, area.y + area.height - 22 * s}, 16 * s, Fade(COR_FOSFORO, 0.5f));
}
