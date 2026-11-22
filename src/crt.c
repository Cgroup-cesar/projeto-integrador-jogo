// crt.c — moldura de monitor para as telas de tarefa. Só desenha; o movimento (varredura, falha) sai do
// relógio.
#include "crt.h"
#include "falha.h"
#include "ui.h"
#include <math.h>
#include <string.h>

// Quatro quinas em L, por dentro da borda.
static void cantos(Rectangle r, Color cor, float s) {
    float l = 14 * s, t = 2 * s, m = 8 * s, x1 = r.x + m, y1 = r.y + m, x2 = r.x + r.width - m,
          y2 = r.y + r.height - m;
    DrawRectangleRec((Rectangle){x1, y1, l, t}, cor);
    DrawRectangleRec((Rectangle){x1, y1, t, l}, cor);
    DrawRectangleRec((Rectangle){x2 - l, y1, l, t}, cor);
    DrawRectangleRec((Rectangle){x2 - t, y1, t, l}, cor);
    DrawRectangleRec((Rectangle){x1, y2 - t, l, t}, cor);
    DrawRectangleRec((Rectangle){x1, y2 - l, t, l}, cor);
    DrawRectangleRec((Rectangle){x2 - l, y2 - t, l, t}, cor);
    DrawRectangleRec((Rectangle){x2 - t, y2 - l, t, l}, cor);
}

void crt_painel(const Estado *e, Rectangle r, Color borda, bool com_cantos) {
    float s = ui_escala();
    DrawRectangleRec(r, COR_CRT_FUNDO);
    DrawRectangleLinesEx(r, 1.0f * s, Fade(borda, 0.45f));
    for (float y = r.y; y < r.y + r.height; y += 3 * s) // padrão MAP: uma scanline a cada 3 px
        DrawLineEx((Vector2){r.x, y}, (Vector2){r.x + r.width, y}, 1.0f, Fade(BLACK, 0.22f));
    float faixa = 90 * s, curso = r.height + faixa; // a varredura desce em CRT_VARREDURA_S e recomeça
    float topo = r.y - faixa + fmodf(e->tempo_total / CRT_VARREDURA_S, 1.0f) * curso;
    BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    DrawRectangleGradientV((int)r.x, (int)topo, (int)r.width, (int)(faixa * 0.5f), Fade(COR_FOSFORO, 0.0f),
                           Fade(COR_FOSFORO, 0.05f));
    DrawRectangleGradientV((int)r.x, (int)(topo + faixa * 0.5f), (int)r.width, (int)(faixa * 0.5f),
                           Fade(COR_FOSFORO, 0.05f), Fade(COR_FOSFORO, 0.0f));
    EndScissorMode();
    if (com_cantos) cantos(r, borda, s);
}

Rectangle crt_caixa(const Estado *e, Rectangle r, const char *legenda) {
    float s = ui_escala();
    DrawRectangleRec(r, Fade(BLACK, 0.28f));
    DrawRectangleLinesEx(r, 1.0f * s, COR_CRT_LINHA);
    ui_texto(e, legenda, (Vector2){r.x + 16 * s, r.y + 12 * s}, 13 * s, COR_CRT_APAGADO);
    return (Rectangle){r.x + 16 * s, r.y + 36 * s, r.width - 32 * s, r.height - 46 * s};
}

void crt_titulo(const Estado *e, const char *texto, Vector2 pos, float tamanho, Color cor) {
    float s = ui_escala();
    for (int i = 0; i < 4; i++) // padrão MAP: o brilho do fósforo (4 cópias em volta)
        ui_texto(e, texto, (Vector2){pos.x + (i % 2 ? 1.5f : -1.5f) * s, pos.y + (i / 2 ? 1.5f : -1.5f) * s},
                 tamanho, Fade(cor, 0.12f));
    ui_texto(e, texto, (Vector2){pos.x + 2 * s, pos.y}, tamanho, Fade((Color){255, 60, 90, 255}, 0.35f));
    ui_texto(e, texto, (Vector2){pos.x - 2 * s, pos.y}, tamanho, Fade((Color){60, 160, 255, 255}, 0.3f));
    ui_texto(e, texto, pos, tamanho, cor);
}

void crt_pergunta(const Estado *e, const char *texto, Vector2 pos, float tamanho) {
    const char *dica = strstr(texto, "  (");
    if (!dica) {
        ui_texto(e, texto, pos, tamanho, COR_CRT_TEXTO);
        return;
    }
    char frase[160];
    size_t n = (size_t)(dica - texto) < sizeof(frase) - 1 ? (size_t)(dica - texto) : sizeof(frase) - 1;
    memcpy(frase, texto, n);
    frase[n] = '\0';
    ui_texto(e, frase, pos, tamanho, COR_CRT_TEXTO);
    float x = pos.x + ui_mede(e, frase, tamanho).x + 10 * ui_escala();
    ui_texto(e, dica + 2, (Vector2){x, pos.y + tamanho * 0.12f}, tamanho * 0.82f, COR_CRT_APAGADO);
}

// Fundo da faixa: apagada, só um véu; escolhida, degradê forte até 75% e sumindo no fim (como no menu).
static void fundo_da_faixa(Rectangle r, Color cor, bool ativa, float s) {
    if (!ativa) {
        DrawRectangleRec(r, Fade(cor, 0.03f));
        return;
    }
    float w1 = r.width * 0.75f;
    DrawRectangleGradientH((int)r.x, (int)r.y, (int)w1, (int)r.height, Fade(cor, 0.5f), Fade(cor, 0.08f));
    DrawRectangleGradientH((int)(r.x + w1), (int)r.y, (int)(r.width - w1), (int)r.height, Fade(cor, 0.08f),
                           Fade(cor, 0.0f));
    DrawRectangle((int)r.x, (int)r.y, (int)(3 * s), (int)r.height, cor);
}

void crt_opcao(const Estado *e, const char *texto, Rectangle r, float tamanho, bool ativa, bool errou,
               bool seta, Color cor) {
    float s = ui_escala(), forca = 0.0f;
    if (ativa) r.x += falha_deslocamento(e, MENU_FALHA_LEVE, &forca);
    if (ativa && errou) { // a escolha errada treme e fica vermelha
        r.x += sinf(e->tempo_total * 70.0f) * 5 * s;
        cor = COR_ALARME;
    }
    fundo_da_faixa(r, cor, ativa, s);
    float calha = seta ? 26 * s : 0.0f, ty = r.y + (r.height - tamanho) * 0.5f;
    if (seta && ativa) {
        float cy = r.y + r.height * 0.5f, x = r.x + 12 * s;
        DrawTriangle((Vector2){x, cy - 6 * s}, (Vector2){x, cy + 6 * s}, (Vector2){x + 9 * s, cy},
                     (Color){234, 255, 241, 255});
    }
    Color letra = ativa ? (Color){234, 255, 241, 255} : COR_CRT_APAGADO;
    falha_texto(e, texto, (Vector2){r.x + 14 * s + calha, ty}, tamanho, letra, forca, r);
}
