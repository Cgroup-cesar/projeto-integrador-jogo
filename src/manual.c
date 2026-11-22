// manual.c — ícones e cartões do manual dos conectivos, usados pelo terminal (cola) e pelos painéis.
// Os ícones são desenhados (não dependem da fonte ter ✓ e ✗).
#include "manual.h"
#include "ui.h"
#include "dialogo.h"
#include <math.h>

void manual_lampada(Vector2 c, float r, bool acesa) {
    if (acesa) DrawCircleV(c, r * 1.8f, Fade(COR_LAMPADA, 0.25f));
    DrawCircleV(c, r, acesa ? COR_LAMPADA : (Color){24, 30, 26, 255});
    DrawCircleLinesV(c, r, acesa ? COR_LAMPADA : Fade(COR_FOSFORO, 0.5f));
}

// `alfa` < 1 apaga (a coluna que não está em análise).
void manual_marca(Vector2 c, float r, bool verdade, float alfa) {
    float t = fmaxf(2.0f, r * 0.35f);
    Color cor = Fade(verdade ? COR_FOSFORO : COR_ALARME, alfa);
    if (verdade) {
        DrawLineEx((Vector2){c.x - r, c.y}, (Vector2){c.x - r * 0.3f, c.y + r * 0.7f}, t, cor);
        DrawLineEx((Vector2){c.x - r * 0.3f, c.y + r * 0.7f}, (Vector2){c.x + r, c.y - r * 0.8f}, t, cor);
    } else {
        DrawLineEx((Vector2){c.x - r * 0.8f, c.y - r * 0.8f}, (Vector2){c.x + r * 0.8f, c.y + r * 0.8f}, t,
                   cor);
        DrawLineEx((Vector2){c.x + r * 0.8f, c.y - r * 0.8f}, (Vector2){c.x - r * 0.8f, c.y + r * 0.8f}, t,
                   cor);
    }
}

void manual_mini_tabela(const Estado *e, Vector2 pos, bool unario, const int *valores, float s) {
    for (int l = 0; l < (unario ? 2 : 4); l++) { // padrão MAP: cada linha da mini-tabela
        bool a = unario ? (l & 1) : ((l >> 1) & 1), b = l & 1;
        float y = pos.y + l * 25 * s;
        // com uma luz só, a lâmpada fica na coluna ao lado do resultado (sem um buraco no meio)
        if (!unario) manual_lampada((Vector2){pos.x + 8 * s, y + 8 * s}, 6 * s, a);
        manual_lampada((Vector2){pos.x + 28 * s, y + 8 * s}, 6 * s, unario ? a : b);
        if (valores[l] < 0)
            ui_texto(e, "?", (Vector2){pos.x + 57 * s, y}, 15 * s, Fade(COR_FOSFORO, 0.4f));
        else
            manual_marca((Vector2){pos.x + 62 * s, y + 8 * s}, 7 * s, valores[l] != 0, 1.0f);
    }
}

void manual_cartao(const Estado *e, Rectangle b, const char *nome, const char *descricao, bool unario,
                   const int *valores, bool destaque, float s) {
    Color cor = destaque ? COR_AMBAR : COR_FOSFORO;
    if (destaque) DrawRectangleRounded(b, 0.1f, 4, Fade(COR_AMBAR, 0.12f));
    DrawRectangleRoundedLinesEx(b, 0.1f, 4, (destaque ? 2.0f : 1.0f) * s, destaque ? cor : Fade(cor, 0.3f));
    ui_texto(e, nome, (Vector2){b.x + 10 * s, b.y + 8 * s}, 18 * s, cor);
    ui_texto_quebrado(e, descricao, (Vector2){b.x + 10 * s, b.y + 32 * s}, b.width - 104 * s, 13 * s,
                      RAYWHITE);
    manual_mini_tabela(e, (Vector2){b.x + b.width - 84 * s, b.y + 14 * s}, unario, valores, s);
}

void manual_selo_ajuda(const Estado *e, float direita, float topo, float s) {
    const char *selo = dialogo_texto(e->ajuda_aberta ? TXT_TERMINAL_SELO_FECHAR : TXT_TERMINAL_SELO_AJUDA);
    Vector2 m = ui_mede(e, selo, 16 * s);
    Rectangle b = {direita - m.x - 16 * s, topo, m.x + 16 * s, 26 * s};
    DrawRectangleRounded(b, 0.3f, 4, e->ajuda_aberta ? (Color){22, 34, 26, 255} : Fade(COR_AMBAR, 0.18f));
    DrawRectangleRoundedLinesEx(b, 0.3f, 4, 1.5f * s, COR_AMBAR);
    ui_texto(e, selo, (Vector2){b.x + 8 * s, b.y + 5 * s}, 16 * s, COR_AMBAR);
}
