// menu.c — a primeira coisa que a pessoa vê no estande: pouca informação e a cena falando por si. O texto
// fica numa coluna à esquerda (sobre um escurecido), e o centro da tela fica livre para o Precursor.
#include "menu.h"
#include "tela_fim.h"
#include "teclas.h"
#include "ui.h"
#include "dialogo.h"
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
// O título falha de vez em quando, como a legenda do rádio falso (a mesma regra pura): letras viram
// símbolos, ele treme e ganha uma sombra vermelha deslocada.
static void titulo(const Estado *e, Vector2 pos, float tamanho) {
    float s = ui_escala();
    char texto[64];
    bool falhando = regras_falha_texto(TITULO_JOGO, e->tempo_total * 0.6f, texto, sizeof(texto));
    if (falhando) {
        float tremor = sinf(e->tempo_total * 70.0f) * 3.0f * s;
        ui_texto(e, texto, (Vector2){pos.x + 4 * s + tremor, pos.y}, tamanho, Fade(COR_ALARME, 0.55f));
        pos.x -= tremor * 0.5f;
    }
    ui_texto(e, texto, pos, tamanho, COR_FOSFORO);
}

// Uma opção como faixa: a cor vai sumindo para a direita. A escolhida acende inteira e ganha a borda.
static void faixa(const Estado *e, const char *texto, Rectangle r, Color cor, bool ativa) {
    float s = ui_escala();
    DrawRectangleGradientH((int)r.x, (int)r.y, (int)r.width, (int)r.height, Fade(cor, ativa ? 0.75f : 0.18f),
                           Fade(cor, 0.0f));
    if (ativa) DrawRectangle((int)r.x, (int)r.y, (int)(4 * s), (int)r.height, cor);
    Color letra = ativa ? (Color){240, 250, 242, 255} : Fade(cor, 0.75f);
    ui_texto(e, texto, (Vector2){r.x + 20 * s, r.y + (r.height - 26 * s) * 0.5f}, 26 * s, letra);
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
    teclas_desenha_tela_cheia(e);
}
