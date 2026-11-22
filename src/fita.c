// fita.c — a fita do avistamento por trás da intro datilografada. Só desenha: o relógio é o da intro
// (e->intro.fita_t e linha_t) e a escolha da foto vem de regras_fita_tomada.
#include "fita.h"
#include "dialogo.h"
#include "falha.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>

void fita_carrega(Recursos *rec) {
    for (int i = 0; i < FITA_FOTOS; i++) { // carregamento: uma vez por foto (não é map/filter/fold)
        char relativo[64], caminho[512];
        snprintf(relativo, sizeof(relativo), "fita/avistamento_%d.png", i + 1);
        rec->tem_fita[i] = ui_caminho_asset(relativo, caminho, sizeof(caminho));
        if (!rec->tem_fita[i]) continue;
        rec->fita[i] = LoadTexture(caminho);
        rec->tem_fita[i] = rec->fita[i].id != 0;
        SetTextureFilter(rec->fita[i], TEXTURE_FILTER_BILINEAR);
    }
}

void fita_descarrega(Recursos *rec) {
    for (int i = 0; i < FITA_FOTOS; i++) // padrão MAP: libera cada foto carregada
        if (rec->tem_fita[i]) UnloadTexture(rec->fita[i]);
}

// A foto cobrindo a tela, com o zoom lento de quem tenta enquadrar e a mão tremendo. `dx` empurra para o lado
// (as fatias da falha). `tinta` permite as cópias vermelha e ciana.
static void foto(const Estado *e, Texture2D t, float zoom, float dx, Color tinta) {
    float s = ui_escala(), w = GetScreenWidth(), h = GetScreenHeight(), r = e->intro.fita_t;
    float escala = fmaxf(w / t.width, h / t.height) * zoom;
    float tremx = (sinf(r * 1.7f) * 6.0f + sinf(r * 4.3f) * 2.5f) * s;
    float tremy = (cosf(r * 1.3f) * 4.0f + sinf(r * 5.1f) * 2.0f) * s;
    Rectangle destino = {w * 0.5f + tremx + dx, h * 0.5f + tremy, t.width * escala, t.height * escala};
    DrawTexturePro(t, (Rectangle){0, 0, (float)t.width, (float)t.height}, destino,
                   (Vector2){destino.width * 0.5f, destino.height * 0.5f}, sinf(r * 0.9f) * 0.4f, tinta);
}

static void imagem(const Estado *e, Texture2D t, float falha) {
    float s = ui_escala(), w = GetScreenWidth(), h = GetScreenHeight();
    float zoom = 1.06f + 0.06f * fminf(1.0f, e->intro.linha_t / 5.0f); // a câmera vai fechando
    foto(e, t, zoom, 0.0f, (Color){232, 228, 236, 255});               // cores desbotadas
    BeginBlendMode(BLEND_ADDITIVE); // franjas de cor do VHS (aumentam com a falha)
    float franja = (3.0f + 10.0f * falha) * s;
    foto(e, t, zoom, franja, Fade((Color){255, 30, 50, 255}, 0.10f + 0.15f * falha));
    foto(e, t, zoom, -franja, Fade((Color){40, 200, 255, 255}, 0.08f + 0.12f * falha));
    EndBlendMode();
    if (falha <= 0.0f) return;
    int q = (int)(e->intro.fita_t * 24.0f);
    for (int i = 0; i < 7; i++) { // padrão MAP: fatias da imagem escorregando para os lados
        if (falha_sorteio(q * 7 + i) > falha) continue;
        float y = falha_sorteio(q * 13 + i) * h, alto = (6 + 40 * falha_sorteio(q * 17 + i)) * s;
        BeginScissorMode(0, (int)y, (int)w, (int)alto);
        foto(e, t, zoom, (falha_sorteio(q * 19 + i) - 0.5f) * 120.0f * s * falha, WHITE);
        EndScissorMode();
    }
    if (falha_sorteio(q + 5) < falha * 0.5f) // o sinal "pisca"
        DrawRectangle(0, 0, (int)w, (int)h, Fade(WHITE, 0.10f * falha));
}

// O que a câmera grava por cima da imagem: ▶ PLAY no alto e, embaixo à direita, a hora e a data.
static void osd(const Estado *e) {
    float s = ui_escala(), w = GetScreenWidth(), h = GetScreenHeight(), t = 22 * s, m = 40 * s;
    DrawTriangle((Vector2){m, m + 2 * s}, (Vector2){m, m + t - 2 * s}, (Vector2){m + t * 0.7f, m + t * 0.5f},
                 (Color){232, 255, 240, 255});
    falha_osd(e, "PLAY", (Vector2){m + t, m}, t);
    int seg = FITA_HORA_INICIO + (int)e->intro.fita_t;
    char hora[16];
    snprintf(hora, sizeof(hora), "%02d:%02d", seg / 3600 % 24, seg / 60 % 60);
    const char *data = dialogo_texto(TXT_FITA_DATA);
    float largura = fmaxf(ui_mede(e, hora, t).x, ui_mede(e, data, t).x);
    falha_osd(e, hora, (Vector2){w - m - largura, h - m - 3 * t - 20 * s}, t);
    falha_osd(e, data, (Vector2){w - m - largura, h - m - 2 * t - 12 * s}, t);
}

// Linhas de varredura e cantos escuros: a tela de um televisor velho.
static void televisor(void) {
    float s = ui_escala(), w = GetScreenWidth(), h = GetScreenHeight();
    for (float y = 0; y < h; y += 3 * s) // padrão MAP: uma linha escura a cada 3 px
        DrawRectangle(0, (int)y, (int)w, (int)fmaxf(1.0f, s), Fade(BLACK, 0.14f));
    DrawRectangleGradientV(0, 0, (int)w, (int)(h * 0.2f), Fade(BLACK, 0.6f), Fade(BLACK, 0.0f));
    DrawRectangleGradientH(0, 0, (int)(w * 0.12f), (int)h, Fade(BLACK, 0.5f), Fade(BLACK, 0.0f));
    DrawRectangleGradientH((int)(w * 0.88f), 0, (int)(w * 0.12f) + 1, (int)h, Fade(BLACK, 0.0f),
                           Fade(BLACK, 0.5f));
}

void fita_desenha(const Estado *e) {
    float w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangle(0, 0, (int)w, (int)h, BLACK);
    const Intro *it = &e->intro;
    TomadaFita tomada = regras_fita_tomada(it->linha, it->letras, it->linha_t);
    if (tomada.foto >= 0 && e->rec.tem_fita[tomada.foto]) {
        imagem(e, e->rec.fita[tomada.foto], tomada.falha);
        televisor();
        falha_chuvisco(e, 0.035f + 0.3f * tomada.falha, 0.04f, true); // o grão da fita
    }
    if (tomada.estatica > 0.0f) falha_chuvisco(e, 0.75f * tomada.estatica, 0.25f, true); // fim do sinal
    // embaixo, o texto datilografado (ui.c) precisa de fundo escuro para ser lido sobre a imagem
    DrawRectangleGradientV(0, (int)(h * 0.5f), (int)w, (int)(h * 0.5f) + 1, Fade(BLACK, 0.0f),
                           Fade(BLACK, 0.85f));
    if (tomada.foto >= 0 && e->rec.tem_fita[tomada.foto]) osd(e); // por cima da sombra: sempre legível
}
