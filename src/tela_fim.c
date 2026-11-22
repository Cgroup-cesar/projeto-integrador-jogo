// tela_fim.c — a tela de desempenho (resultado, placar e lugar no ranking: só o principal), a avaliação do
// jogo por quem jogou (até 3 perguntas, de 0 a 5 estrelas, sem nome) e a tabela do ranking.
#include "tela_fim.h"
#include "ranking.h"
#include "teclas.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>

// ================================================================== atualiza
// A partida entra no ranking uma vez, quando o placar aparece. No --smoke fica só na memória.
static void registra(Estado *e) {
    Fim *f = &e->fim;
    f->registrado = true;
    EntradaRanking p = {e->placar, (int)e->tempo_partida, e->fase == FASE_FIM_BOM, e->ident.feminino, {0}};
    const char *nome = e->ident.nome[0] != '\0' ? e->ident.nome : dialogo_texto(TXT_NOME_PADRAO);
    snprintf(p.nome, sizeof(p.nome), "%s", nome);
    f->posicao = regras_ranking_insere(&e->ranking, &p);
    if (!e->modo_smoke) ranking_salva(&e->ranking);
}

static void envia_avaliacao(const Estado *e) {
    bool respondeu = false;
    for (int i = 0; i < AVALIACAO_PERGUNTAS; i++) // padrão FOLD: alguma pergunta respondida?
        respondeu = respondeu || e->fim.notas[i] > 0;
    if (respondeu && !e->modo_smoke)
        ranking_registra_avaliacao(e->fim.notas, e->placar, e->fase == FASE_FIM_BOM);
}

// ↑↓ escolhe a pergunta, ←→ dá as estrelas, ENTER envia e mostra o ranking. Parado, segue sozinho.
static void atualiza_desempenho(Estado *e, float dt) {
    Fim *f = &e->fim;
    bool mexeu = e->in.cima || e->in.baixo || e->in.esquerda || e->in.direita;
    f->parado_t = mexeu ? 0.0f : f->parado_t + dt;
    if (e->in.cima) f->nota_cursor = (f->nota_cursor + AVALIACAO_PERGUNTAS - 1) % AVALIACAO_PERGUNTAS;
    if (e->in.baixo) f->nota_cursor = (f->nota_cursor + 1) % AVALIACAO_PERGUNTAS;
    int *nota = &f->notas[f->nota_cursor];
    if (e->in.direita && *nota < AVALIACAO_MAX) (*nota)++;
    if (e->in.esquerda && *nota > 0) (*nota)--;
    // O ENTER que fechou o texto do final chega aqui no mesmo quadro: a folga impede que ele pule esta tela.
    bool enter = e->in.confirmar && f->tempo_tela > FIM_PLACAR_FOLGA_S;
    if (!enter && f->parado_t < FIM_VOLTA_MENU_S) return;
    envia_avaliacao(e);
    f->tela = 1;
    f->tempo_tela = 0.0f;
}

void tela_fim_atualiza(Estado *e, float dt) {
    Fim *f = &e->fim;
    if ((e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM) || !f->tela_final) return;
    if (!f->registrado) registra(e);
    f->tempo_tela += dt;
    if (f->tela == 0) {
        atualiza_desempenho(e, dt);
        return;
    }
    bool enter = e->in.confirmar && f->tempo_tela > FIM_PLACAR_FOLGA_S;
    if (enter || f->tempo_tela >= FIM_RANKING_S) e->evento = EV_VOLTAR_MENU;
}

// ================================================================== desenho
// Estrela de 5 pontas desenhada (a fonte não tem ★): cheia em âmbar, vazia só com o contorno.
static void estrela(Vector2 c, float r, bool cheia, float s) {
    Vector2 p[11];
    for (int i = 0; i <= 10; i++) { // padrão MAP: as 10 pontas, alternando raio grande e pequeno
        float a = -PI * 0.5f + i * PI / 5.0f, raio = (i % 2 == 0) ? r : r * 0.45f;
        p[i] = (Vector2){c.x + cosf(a) * raio, c.y + sinf(a) * raio};
    }
    for (int i = 0; i < 10; i++) { // padrão MAP: preenche (leque a partir do centro) e contorna
        if (cheia) DrawTriangle(c, p[i + 1], p[i], COR_AMBAR);
        DrawLineEx(p[i], p[i + 1], 1.5f * s, cheia ? COR_AMBAR : Fade(COR_AMBAR, 0.45f));
    }
}

// [REQ:RF5] [REQ:RF6] tela de fim: VITÓRIA ou DERROTA, o placar e o lugar no ranking (só o principal)
static void desenha_desempenho(const Estado *e) {
    float s = ui_escala(), w = GetScreenWidth();
    const Fim *f = &e->fim;
    bool vitoria = e->fase == FASE_FIM_BOM;
    char texto[96];
    ui_texto_centro(e, dialogo_texto(vitoria ? TXT_FIM_VITORIA : TXT_FIM_DERROTA), 56 * s, 60 * s,
                    vitoria ? COR_FOSFORO : COR_ALARME);
    dialogo_personaliza(&e->ident, dialogo_texto(TXT_FIM_JOGADOR), texto, sizeof(texto)); // [REQ:NOME-USO]
    ui_texto_centro(e, texto, 128 * s, 22 * s, COR_AMBAR);
    snprintf(texto, sizeof(texto), "%s  %d", dialogo_texto(TXT_FIM_PLACAR), e->placar);
    ui_texto_centro(e, texto, 164 * s, 34 * s, COR_FOSFORO);
    if (f->posicao >= 0)
        snprintf(texto, sizeof(texto), dialogo_texto(TXT_FIM_POSICAO), f->posicao + 1, e->ranking.n);
    else
        snprintf(texto, sizeof(texto), dialogo_texto(TXT_FIM_FORA), RANKING_MAX);
    ui_texto_centro(e, texto, 212 * s, 22 * s, COR_VIDRO);
    Rectangle caixa = {w * 0.5f - 380 * s, 262 * s, 760 * s, 214 * s};
    ui_painel(caixa, COR_AMBAR);
    ui_texto(e, dialogo_texto(TXT_AVALIA_TITULO), (Vector2){caixa.x + 24 * s, caixa.y + 16 * s}, 18 * s,
             COR_AMBAR);
    static const TextoId perguntas[AVALIACAO_PERGUNTAS] = {TXT_AVALIA_P1, TXT_AVALIA_P2, TXT_AVALIA_P3};
    for (int i = 0; i < AVALIACAO_PERGUNTAS; i++) { // padrão MAP: cada pergunta com as suas estrelas
        float y = caixa.y + 56 * s + i * 50 * s;
        bool ativa = f->nota_cursor == i;
        ui_opcao(e, dialogo_texto(perguntas[i]), (Vector2){caixa.x + 34 * s, y + 6 * s}, caixa.width - 68 * s,
                 20 * s, ativa, ativa ? COR_FOSFORO : Fade(COR_FOSFORO, 0.7f));
        for (int j = 0; j < AVALIACAO_MAX; j++) // padrão MAP: as 5 estrelas da pergunta
            estrela((Vector2){caixa.x + caixa.width - 220 * s + j * 40 * s, y + 18 * s}, 14 * s,
                    j < f->notas[i], s);
    }
}

// Uma linha do ranking: lugar, "Professora Inês", resultado e placar.
static void linha_ranking(const Estado *e, int i, float y, bool destaque) {
    float s = ui_escala(), x = GetScreenWidth() * 0.5f - 380 * s, largura = 760 * s;
    const EntradaRanking *it = &e->ranking.itens[i];
    if (destaque) {
        DrawRectangleRounded((Rectangle){x, y - 6 * s, largura, 36 * s}, 0.3f, 4, Fade(COR_AMBAR, 0.18f));
        DrawRectangleRoundedLinesEx((Rectangle){x, y - 6 * s, largura, 36 * s}, 0.3f, 4, 2.0f * s, COR_AMBAR);
    }
    Color cor = destaque ? COR_AMBAR : COR_FOSFORO;
    char texto[96];
    snprintf(texto, sizeof(texto), "%dº", i + 1);
    ui_texto(e, texto, (Vector2){x + 20 * s, y}, 22 * s, cor);
    Identidade id = {.feminino = it->feminino};
    snprintf(id.nome, sizeof(id.nome), "%s", it->nome);
    dialogo_personaliza(&id, dialogo_texto(TXT_FIM_JOGADOR), texto, sizeof(texto));
    ui_texto(e, texto, (Vector2){x + 90 * s, y}, 22 * s, cor);
    if (destaque) {
        float fim_nome = x + 90 * s + ui_mede(e, texto, 22 * s).x + 14 * s;
        ui_texto(e, dialogo_texto(TXT_RANKING_VOCE), (Vector2){fim_nome, y + 4 * s}, 15 * s, COR_AMBAR);
    }
    const char *resultado = dialogo_texto(it->vitoria ? TXT_FIM_VITORIA : TXT_FIM_DERROTA);
    ui_texto(e, resultado, (Vector2){x + 480 * s, y + 3 * s}, 16 * s,
             Fade(it->vitoria ? COR_FOSFORO : COR_ALARME, 0.8f));
    snprintf(texto, sizeof(texto), "%d", it->placar);
    ui_texto(e, texto, (Vector2){x + largura - 20 * s - ui_mede(e, texto, 22 * s).x, y}, 22 * s, cor);
}

void tela_fim_desenha_ranking(const Estado *e, int destaque, TextoId chamada) {
    float s = ui_escala(), h = GetScreenHeight();
    DrawRectangle(0, 0, GetScreenWidth(), (int)h, (Color){4, 6, 5, 245});
    ui_texto_centro(e, dialogo_texto(TXT_RANKING_TITULO), 60 * s, 34 * s, COR_AMBAR);
    if (e->ranking.n == 0) ui_texto_centro(e, dialogo_texto(TXT_RANKING_VAZIO), 220 * s, 22 * s, COR_VIDRO);
    float y = 126 * s;
    int mostradas = e->ranking.n < RANKING_TELA ? e->ranking.n : RANKING_TELA;
    for (int i = 0; i < mostradas; i++, y += 42 * s) // padrão MAP: as primeiras do ranking
        linha_ranking(e, i, y, i == destaque);
    if (destaque >= RANKING_TELA) { // a partida ficou mais para baixo: aparece depois de "…"
        ui_texto_centro(e, "…", y, 22 * s, Fade(COR_FOSFORO, 0.6f));
        linha_ranking(e, destaque, y + 36 * s, true);
    }
    ui_texto_centro(e, dialogo_texto(chamada), h - 150 * s, 26 * s, COR_FOSFORO);
}

void tela_fim_desenha(const Estado *e) {
    if ((e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM) || !e->fim.tela_final) return;
    if (e->fim.tela == 0) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){4, 6, 5, 240});
        desenha_desempenho(e);
    } else {
        tela_fim_desenha_ranking(e, e->fim.posicao, TXT_RANKING_CTA_FIM);
    }
    teclas_desenha_placar(e);
}
