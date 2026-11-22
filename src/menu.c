// menu.c — a primeira coisa que a pessoa vê no estande: pouca informação e a cena falando por si. O texto
// fica numa coluna à esquerda (sobre um escurecido), e o centro da tela fica livre para o Precursor.
#include "menu.h"
#include "tela_fim.h"
#include "teclas.h"
#include "ui.h"
#include "dialogo.h"
#include "falha.h"
#include <math.h>
#include <stdio.h>

// ================================================================== atualiza
// INICIAR, RANKING e SAIR (no modo feira, sem SAIR). O ranking abre por cima até ENTER ou ESC.
void menu_atualiza(Estado *e) {
    if (e->fase != FASE_MENU) return;
    if (e->menu.ranking_aberto) {
        if (e->in.confirmar || e->in.voltar) e->menu.ranking_aberto = false;
        return;
    }
    int opcoes = e->modo_feira ? 2 : 3;
    if (e->in.cima || e->in.baixo) e->menu.troca_t = e->tempo_total;
    if (e->in.cima) e->menu.cursor = (e->menu.cursor + opcoes - 1) % opcoes;
    if (e->in.baixo) e->menu.cursor = (e->menu.cursor + 1) % opcoes;
    if (!e->in.confirmar) return;
    if (e->menu.cursor == 0)
        e->evento = EV_INICIAR;
    else if (e->menu.cursor == 1)
        e->menu.ranking_aberto = true;
    else
        e->sair = true;
}

// ================================================================== desenho
// ------------------------------------------------------------------ a fita (efeitos portados do protótipo
// em HTML e do menu_inicial.c da equipe). Tudo sai do relógio: desenhar não muda o estado.
static float ciclo(const Estado *e, float periodo) { // 0..1 dentro de um ciclo de `periodo` segundos
    return fmodf(e->tempo_total / periodo, 1.0f);
}

// Texto de OSD da fita: branco com franjas vermelha e ciana, como o vídeo de um VHS velho.
static void texto_osd(const Estado *e, const char *texto, Vector2 pos, float tamanho) {
    float s = ui_escala();
    ui_texto(e, texto, (Vector2){pos.x - 2 * s, pos.y}, tamanho, Fade((Color){40, 220, 255, 255}, 0.5f));
    ui_texto(e, texto, (Vector2){pos.x + 2 * s, pos.y}, tamanho, Fade((Color){255, 40, 80, 255}, 0.5f));
    ui_texto(e, texto, pos, tamanho, (Color){232, 255, 240, 255});
}

// A tela de início é uma gravação do SNI: ▶ PLAY e PE ●●● no alto, ● REC com o contador e a data da fita.
static void marcas_da_fita(const Estado *e) {
    float s = ui_escala(), w = GetScreenWidth(), t = 20 * s, m = 34 * s;
    DrawTriangle((Vector2){m, m + 2 * s}, (Vector2){m, m + t - 2 * s}, (Vector2){m + t * 0.7f, m + t * 0.5f},
                 (Color){232, 255, 240, 255});
    texto_osd(e, "PLAY", (Vector2){m + t, m}, t);
    texto_osd(e, "PE", (Vector2){m + t + 90 * s, m}, t);
    for (int i = 0; i < 3; i++) // padrão MAP: as três bolinhas da velocidade da fita
        DrawCircleV((Vector2){m + t + 126 * s + i * 14 * s, m + t * 0.5f}, 4 * s, Fade(RAYWHITE, 0.85f));
    char contador[16];
    int seg = (int)e->tempo_fase;
    snprintf(contador, sizeof(contador), "%02d:%02d:%02d", seg / 3600, seg / 60 % 60, seg % 60);
    float x_rec = w - m - ui_mede(e, "REC", t).x;
    if (ciclo(e, 2.0f) < 0.5f)
        DrawRectangle((int)(x_rec - 22 * s), (int)(m + 4 * s), (int)(12 * s), (int)(12 * s), RED);
    texto_osd(e, "REC", (Vector2){x_rec, m}, t);
    texto_osd(e, contador, (Vector2){w - m - ui_mede(e, contador, t * 0.9f).x, m + t + 4 * s}, t * 0.9f);
    const char *data = dialogo_texto(TXT_MENU_DATA_FITA);
    texto_osd(e, data, (Vector2){w - m - ui_mede(e, data, t * 0.9f).x, m + 2 * t + 10 * s}, t * 0.9f);
}

// Uma frase pisca por ~0,1 s, uma vez a cada janela de 10,5 s, num instante e num lugar sorteados.
static void subliminar(const Estado *e) {
    if (!MENU_SUBLIMINARES) return;
    static const TextoId frases[] = {TXT_SUBLIMINAR_1, TXT_SUBLIMINAR_2, TXT_SUBLIMINAR_3, TXT_SUBLIMINAR_4,
                                     TXT_SUBLIMINAR_5};
    float janela = 10.5f;
    int n = (int)(e->tempo_total / janela);
    float dentro = e->tempo_total - n * janela, quando = 1.5f + falha_sorteio(n) * 7.5f;
    float dura = 0.07f + falha_sorteio(n + 7) * 0.09f;
    if (dentro < quando || dentro > quando + dura) return;
    float s = ui_escala(), tamanho = 96 * s;
    const char *frase = dialogo_texto(frases[(int)(falha_sorteio(n + 13) * 4.999f)]);
    Vector2 m = ui_mede(e, frase, tamanho);
    Vector2 pos = {GetScreenWidth() * (0.42f + falha_sorteio(n + 21) * 0.3f) - m.x * 0.5f,
                   GetScreenHeight() * (0.3f + falha_sorteio(n + 29) * 0.35f)};
    BeginBlendMode(BLEND_ADDITIVE);
    ui_texto(e, frase, (Vector2){pos.x + 4 * s, pos.y}, tamanho, (Color){40, 220, 255, 115});
    ui_texto(e, frase, pos, tamanho, (Color){255, 30, 30, 255});
    EndBlendMode();
}

// O título pifa como TV velha: tremida (ciclo de 3,4 s), queda de energia (9 s), fatias vermelhas e cianas
// deslocadas (4,6 s) e, mais raramente, letras que viram símbolos (a regra da legenda do rádio falso).
static void titulo(const Estado *e, Vector2 pos, float tamanho) {
    float s = ui_escala(), w = GetScreenWidth();
    char texto[64];
    regras_falha_texto(TITULO_JOGO, e->tempo_total * 0.25f, texto, sizeof(texto));
    float ct = ciclo(e, 3.4f), ce = ciclo(e, 9.0f), cg = ciclo(e, 4.6f);
    pos.x += (ct >= 0.41f && ct < 0.42f) ? -4 * s : (ct >= 0.42f && ct < 0.43f) ? 5 * s : 0.0f;
    float luz = ((ce >= 0.61f && ce < 0.62f) || (ce >= 0.63f && ce < 0.64f)) ? 0.0f
                : (ce >= 0.62f && ce < 0.63f)                                ? 0.7f
                : (ce >= 0.89f && ce < 0.90f)                                ? 0.2f
                                                                             : 1.0f;
    if (luz <= 0.0f) return;    // a energia caiu: o título some por um instante
    for (int i = 0; i < 4; i++) // padrão MAP: o brilho do fósforo (4 cópias em volta)
        ui_texto(e, texto, (Vector2){pos.x + (i % 2 ? 2 : -2) * s, pos.y + (i / 2 ? 2 : -2) * s}, tamanho,
                 Fade(COR_FOSFORO, 0.15f * luz));
    ui_texto(e, texto, (Vector2){pos.x + 4 * s, pos.y}, tamanho,
             Fade((Color){255, 30, 50, 255}, 0.55f * luz));
    ui_texto(e, texto, (Vector2){pos.x - 3 * s, pos.y}, tamanho,
             Fade((Color){40, 220, 255, 255}, 0.3f * luz));
    if (cg >= 0.41f && cg < 0.44f) { // fatias: só uma faixa de cada cópia aparece, deslocada
        const float faixas[4][3] = {
            {0.10f, 0.30f, -8}, {0.55f, 0.33f, 6}, {0.60f, 0.32f, 8}, {0.18f, 0.30f, -6}};
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 0; i < 4; i++) { // padrão MAP: cada fatia recortada (as 2 primeiras vermelhas)
            BeginScissorMode(0, (int)(pos.y + tamanho * faixas[i][0]), (int)w, (int)(tamanho * faixas[i][1]));
            Color cor = i < 2 ? (Color){255, 30, 50, 200} : (Color){40, 220, 255, 180};
            ui_texto(e, texto, (Vector2){pos.x + faixas[i][2] * s, pos.y}, tamanho, cor);
            EndScissorMode();
        }
        EndBlendMode();
    }
    ui_texto(e, texto, pos, tamanho, Fade(COR_FOSFORO, luz));
}

// Uma opção como faixa: a cor vai sumindo para a direita. A escolhida acende inteira, ganha a borda,
// balança e falha (cópias vermelha e ciana deslocadas e uma fatia do texto fora do lugar).
static void faixa(const Estado *e, const char *texto, Rectangle r, Color cor, bool ativa) {
    float s = ui_escala(), forca = 0.0f;
    if (ativa) r.x += falha_deslocamento(e, 1.0f, &forca);
    DrawRectangleGradientH((int)r.x, (int)r.y, (int)r.width, (int)r.height, Fade(cor, ativa ? 0.75f : 0.18f),
                           Fade(cor, 0.0f));
    if (ativa) DrawRectangle((int)r.x, (int)r.y, (int)(4 * s), (int)r.height, cor);
    if (ativa && fmodf(e->tempo_total, 1.6f) < 0.8f) { // ▶ piscando seco, como no protótipo
        float cy = r.y + r.height * 0.5f, x = r.x - 24 * s;
        DrawTriangle((Vector2){x, cy - 8 * s}, (Vector2){x, cy + 8 * s}, (Vector2){x + 12 * s, cy}, cor);
    }
    Color letra = ativa ? (Color){240, 250, 242, 255} : Fade(cor, 0.75f);
    Vector2 p = {r.x + 20 * s, r.y + (r.height - 26 * s) * 0.5f};
    falha_texto(e, texto, p, 26 * s, letra, forca, r);
}

// O convite no canto: quem está em 1º e "você pode ser o próximo". A borda respira devagar.
static void convite(const Estado *e, Rectangle r) {
    float s = ui_escala(), respira = 0.45f + 0.35f * sinf(e->tempo_total * 2.0f);
    DrawRectangleRec(r, Fade(BLACK, 0.6f));
    DrawRectangleLinesEx(r, 1.5f * s, Fade(COR_AMBAR, respira));
    if (e->ranking.n == 0) {
        ui_texto_quebrado(e, dialogo_texto(TXT_MENU_CTA_VAZIO), (Vector2){r.x + 16 * s, r.y + 16 * s},
                          r.width - 32 * s, 18 * s, COR_AMBAR);
        return;
    }
    const EntradaRanking *lider = &e->ranking.itens[0];
    Identidade id = {.feminino = lider->feminino};
    snprintf(id.nome, sizeof(id.nome), "%s", lider->nome);
    char nome[96], texto[160];
    dialogo_personaliza(&id, dialogo_texto(TXT_FIM_JOGADOR), nome, sizeof(nome));
    snprintf(texto, sizeof(texto), dialogo_texto(TXT_MENU_LIDER), nome, lider->placar);
    ui_texto(e, texto, (Vector2){r.x + 16 * s, r.y + 14 * s}, 17 * s, COR_AMBAR);
    ui_texto(e, dialogo_texto(TXT_MENU_CTA), (Vector2){r.x + 16 * s, r.y + 44 * s}, 22 * s, COR_FOSFORO);
}

// [REQ:RF5] tela de início: título, gancho, INICIAR, RANKING e SAIR, o convite e o aviso de nome provisório
void menu_desenha(const Estado *e) {
    if (e->fase != FASE_MENU) return;
    if (e->menu.ranking_aberto) { // só a tabela: o menu por trás aparecia fantasma
        tela_fim_desenha_ranking(e, -1, TXT_MENU_CTA);
        teclas_desenha_tela_cheia(e);
        return;
    }
    float s = ui_escala(), w = GetScreenWidth(), h = GetScreenHeight(), x = 90 * s;
    DrawRectangleGradientH(0, 0, (int)(w * 0.62f), (int)h, Fade(BLACK, 0.88f), Fade(BLACK, 0.0f));
    DrawRectangleGradientV(0, (int)(h * 0.6f), (int)w, (int)(h * 0.4f), Fade(BLACK, 0.0f), Fade(BLACK, 0.7f));
    falha_chuvisco(e, MENU_ESTATICA_ALFA, MENU_ESTATICA_PICO, true);
    titulo(e, (Vector2){x, 130 * s}, 76 * s);
    ui_texto_quebrado(e, dialogo_texto(TXT_MENU_GANCHO), (Vector2){x + 4 * s, 248 * s}, 720 * s, 20 * s,
                      Fade(RAYWHITE, 0.75f));
    const TextoId opcoes[3] = {TXT_MENU_INICIAR, TXT_MENU_RANKING, TXT_MENU_SAIR};
    const Color cores[3] = {COR_FOSFORO, COR_AMBAR, COR_ALARME};
    int n = e->modo_feira ? 2 : 3;
    for (int i = 0; i < n; i++) // padrão MAP: cada opção numa faixa
        faixa(e, dialogo_texto(opcoes[i]), (Rectangle){x, (330 + i * 62) * s, 400 * s, 46 * s}, cores[i],
              e->menu.cursor == i);
    convite(e, (Rectangle){w - 90 * s - 400 * s, h - 190 * s, 400 * s, 84 * s});
#if TITULO_PROVISORIO
    ui_texto(e, dialogo_texto(TXT_MENU_AVISO_PROVISORIO), (Vector2){x, h - 92 * s}, 14 * s,
             Fade(COR_AMBAR, 0.5f));
#endif
    marcas_da_fita(e);
    subliminar(e);
    teclas_desenha_tela_cheia(e);
}
