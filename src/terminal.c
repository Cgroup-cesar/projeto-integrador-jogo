// terminal.c — interface do ANALISADOR SEMÂNTICO SNI-85: frases em glifos, luzes, tabela-verdade ao vivo,
// escolha do conectivo e tradução. Só desenha a partir do estado; quem decide é rodadas.c.
#include "terminal.h"
#include "rodadas.h"
#include "glifos.h"
#include "criatura.h"
#include "ui.h"
#include "dialogo.h"
#include <stdio.h>

// O terminal cuida da navegação (Esc sai, setas movem o cursor); quem decide o que o ENTER faz é rodadas.c.
void terminal_atualiza(Estado *e, float dt) {
    (void)dt; // a navegação é por tecla, sem tempo
    Rodada *r = &e->rod;
    if (e->jog.modo != MODO_TERMINAL || !regras_fase_eh_rodada(e->fase) || criatura_em_susto(e)) return;
    if (e->in.voltar) {
        e->jog.modo = MODO_ANDANDO;
        return;
    }
    int opcoes = r->etapa == ETAPA_ESCOLHER_CONECTIVO ? CONECTIVOS_ESCOLHA
                 : r->etapa == ETAPA_TRADUZIR         ? 3
                                                      : 0;
    if (opcoes == 0) return;
    if (e->in.cima) r->cursor = (r->cursor + opcoes - 1) % opcoes;
    if (e->in.baixo) r->cursor = (r->cursor + 1) % opcoes;
}

// ------------------------------------------------------------------ moldura
static float escala(void) {
    return ui_escala();
}

Rectangle terminal_moldura(const Estado *e, const char *cabecalho) {
    float s = escala();
    Rectangle r = {GetScreenWidth() - 620 * s, 56 * s, 596 * s, 600 * s};
    ui_painel(r, COR_FOSFORO);
    ui_texto(e, dialogo_texto(TXT_TERMINAL_TITULO), (Vector2){r.x + 20 * s, r.y + 14 * s}, 24 * s,
             COR_FOSFORO);
    ui_texto(e, cabecalho, (Vector2){r.x + 20 * s, r.y + 46 * s}, 18 * s, Fade(COR_FOSFORO, 0.75f));
    DrawLineEx((Vector2){r.x + 14 * s, r.y + 74 * s}, (Vector2){r.x + r.width - 14 * s, r.y + 74 * s}, 1,
               Fade(COR_FOSFORO, 0.35f));
    if (e->diario_lido)
        ui_texto_quebrado(e, dialogo_texto(TXT_TERMINAL_ANOTACAO),
                          (Vector2){r.x + 20 * s, r.y + r.height + 10 * s}, r.width - 40 * s, 15 * s,
                          COR_AMBAR);
    return (Rectangle){r.x + 20 * s, r.y + 86 * s, r.width - 40 * s, r.height - 100 * s};
}

// Cabeçalho: RODADA n/3 · VIDRO: ■ ■ □ · PLACAR: 000 (os quadrados são desenhados).
static Rectangle moldura_da_rodada(const Estado *e) {
    float s = escala();
    char inicio[48], fim[48];
    snprintf(inicio, sizeof(inicio), dialogo_texto(TXT_TERMINAL_CABECALHO), e->rod.numero);
    snprintf(fim, sizeof(fim), dialogo_texto(TXT_TERMINAL_PLACAR), e->placar);
    Rectangle area = terminal_moldura(e, inicio);
    float x = area.x + ui_mede(e, inicio, 18 * s).x + 10 * s, y = area.y - 40 * s + 2 * s;
    Color cor = e->vidas <= 1 ? COR_ALARME : COR_FOSFORO;
    x += ui_quadrados_vidro(e->vidas, (Vector2){x, y + 2 * s}, 12 * s, cor) + 4 * s;
    ui_texto(e, fim, (Vector2){x, y}, 18 * s, Fade(COR_FOSFORO, 0.75f));
    return area;
}

// ------------------------------------------------------------------ partes
static float desenha_frases(const Estado *e, const DadosRodada *d, Vector2 pos) {
    float s = escala();
    ui_texto(e, dialogo_texto(TXT_TERMINAL_FRASE_OBSERVADA), pos, 15 * s, Fade(COR_FOSFORO, 0.6f));
    for (int c = 0; c < d->n_colunas; c++) {
        Vector2 p = {pos.x + c * 260 * s, pos.y + 26 * s};
        if (d->n_colunas > 1)
            ui_texto(e, c == 0 ? "1" : "2", (Vector2){p.x, p.y + 8 * s}, 18 * s, Fade(COR_FOSFORO, 0.5f));
        desenha_frase_glifos(e, &d->teste[c], (Vector2){p.x + (d->n_colunas > 1 ? 24 * s : 0), p.y}, 34 * s,
                             3.0f * s);
    }
    return 80 * s;
}

static float desenha_luzes(const Estado *e, const DadosRodada *d, Vector2 pos) {
    float s = escala();
    char texto[96];
    const char *a = dialogo_texto(e->rod.luz_a ? TXT_TERMINAL_LUZ_ACESA : TXT_TERMINAL_LUZ_APAGADA);
    const char *b = dialogo_texto(e->rod.luz_b ? TXT_TERMINAL_LUZ_ACESA : TXT_TERMINAL_LUZ_APAGADA);
    if (d->n_luzes == 1)
        snprintf(texto, sizeof(texto), dialogo_texto(TXT_TERMINAL_LUZ_UMA), a);
    else
        snprintf(texto, sizeof(texto), dialogo_texto(TXT_TERMINAL_LUZ_DUAS), a, b);
    ui_texto(e, texto, pos, 17 * s, COR_VIDRO);
    return 30 * s;
}

// A tabela-verdade se preenche sozinha conforme ele responde. A linha das luzes atuais fica marcada.
static float desenha_tabela(const Estado *e, const DadosRodada *d, Vector2 pos) {
    float s = escala(), linha_h = 26 * s;
    const Rodada *r = &e->rod;
    ui_texto(e, dialogo_texto(TXT_TERMINAL_TABELA), pos, 15 * s, Fade(COR_FOSFORO, 0.6f));
    float y = pos.y + 24 * s;
    ui_texto(e, d->n_luzes == 1 ? "A" : "A   B", (Vector2){pos.x + 24 * s, y}, 18 * s,
             Fade(COR_FOSFORO, 0.7f));
    for (int c = 0; c < d->n_colunas; c++) {
        char coluna[24];
        snprintf(coluna, sizeof(coluna),
                 dialogo_texto(d->n_colunas > 1 ? TXT_TERMINAL_COLUNA : TXT_TERMINAL_COLUNA_UNICA), c + 1);
        ui_texto(e, coluna, (Vector2){pos.x + (140 + c * 120) * s, y}, 18 * s, Fade(COR_FOSFORO, 0.7f));
    }
    int atual = rodadas_linha_atual(e);
    for (int l = 0; l < rodadas_linhas(d); l++) {
        y += linha_h;
        if (l == atual && r->etapa == ETAPA_DECIFRAR)
            DrawRectangleRounded((Rectangle){pos.x + 10 * s, y - 3 * s, 360 * s, linha_h - 2 * s}, 0.3f, 4,
                                 Fade(COR_FOSFORO, 0.12f));
        char entradas[16];
        if (d->n_luzes == 1)
            snprintf(entradas, sizeof(entradas), "%d", l & 1);
        else
            snprintf(entradas, sizeof(entradas), "%d   %d", (l >> 1) & 1, l & 1);
        ui_texto(e, entradas, (Vector2){pos.x + 24 * s, y}, 18 * s, COR_FOSFORO);
        for (int c = 0; c < d->n_colunas; c++) {
            bool vista = r->linha_vista[c][l], valor = r->linha_valor[c][l];
            const char *t = !vista ? "-" : valor ? "V" : "F";
            Color cor = !vista ? Fade(COR_FOSFORO, 0.3f) : valor ? COR_FOSFORO : COR_ALARME;
            ui_texto(e, t, (Vector2){pos.x + (160 + c * 120) * s, y}, 20 * s, cor);
        }
    }
    return y + linha_h + 8 * s - pos.y;
}

static void desenha_opcoes_conectivo(const Estado *e, const DadosRodada *d, Vector2 pos) {
    float s = escala();
    char texto[64];
    const char *base = dialogo_texto(TXT_TERMINAL_ESCOLHA_CONECTIVO);
    if (d->n_colunas > 1) {
        snprintf(texto, sizeof(texto), dialogo_texto(TXT_TERMINAL_QUAL_FRASE), e->rod.coluna + 1,
                 d->n_colunas);
        ui_texto(e, texto, pos, 16 * s, COR_AMBAR);
        pos.y += 22 * s;
    }
    ui_texto(e, base, pos, 17 * s, COR_FOSFORO);
    for (int i = 0; i < CONECTIVOS_ESCOLHA; i++)
        ui_opcao(e, dialogo_conectivo(i), (Vector2){pos.x + 10 * s, pos.y + (30 + i * 32) * s}, 220 * s,
                 22 * s, e->rod.cursor == i, COR_FOSFORO);
}

// Tradução: a frase com o significado de cada glifo embaixo (conectivo só se já foi decifrado).
static void desenha_traducao(const Estado *e, const DadosRodada *d, Vector2 pos, float largura) {
    float s = escala();
    desenha_frase_glifos(e, &d->traducao, pos, 40 * s, 3.2f * s);
    for (int i = 0; i < d->traducao.n; i++) {
        IdGlifo g = d->traducao.glifos[i];
        int c = glifo_conectivo(g);
        const char *sig = (c >= 0 && !e->conectivo_decifrado[c]) ? "?" : dialogo_significado_glifo(g);
        ui_texto(e, sig, (Vector2){pos.x + i * 40 * s * 1.45f, pos.y + 48 * s}, 13 * s,
                 Fade(glifo_cor(e, g), 0.85f));
    }
    float y = pos.y + 84 * s;
    ui_texto_quebrado(e, dialogo_texto(TXT_TERMINAL_TRADUZA), (Vector2){pos.x, y}, largura, 17 * s,
                      COR_FOSFORO);
    for (int i = 0; i < 3; i++)
        ui_opcao(e, dialogo_opcao_traducao(e->rod.numero, i),
                 (Vector2){pos.x + 10 * s, y + (34 + i * 34) * s}, largura - 10 * s, 20 * s,
                 e->rod.cursor == i, COR_FOSFORO);
}

static void desenha_rodape(const Estado *e, Rectangle area) {
    float s = escala();
    const Rodada *r = &e->rod;
    float y = area.y + area.height - 24 * s;
    ui_texto(e, dialogo_texto(TXT_TERMINAL_SAIR), (Vector2){area.x, y}, 15 * s, Fade(COR_FOSFORO, 0.45f));
    if (r->aviso_t > 0.0f) {
        bool erro = r->aviso == dialogo_texto(TXT_TERMINAL_ERRO);
        ui_texto(e, r->aviso, (Vector2){area.x, y - 34 * s}, 22 * s, erro ? COR_ALARME : COR_FOSFORO);
    }
    if (r->numero == 1 && r->etapa != ETAPA_TRADUZIR && r->etapa != ETAPA_CONCLUIDA) {
        float h = ui_texto_quebrado(e, dialogo_texto(TXT_TERMINAL_DICA_R1_1), (Vector2){area.x, y - 120 * s},
                                    area.width, 15 * s, COR_AMBAR);
        ui_texto_quebrado(e, dialogo_texto(TXT_TERMINAL_DICA_R1_2), (Vector2){area.x, y - 120 * s + h},
                          area.width, 15 * s, COR_AMBAR);
    }
}

void terminal_desenha(const Estado *e) {
    if (e->jog.modo != MODO_TERMINAL || !regras_fase_eh_rodada(e->fase)) return;
    const Rodada *r = &e->rod;
    const DadosRodada *d = rodadas_dados(r->numero);
    Rectangle area = moldura_da_rodada(e);
    float s = escala();
    Vector2 p = {area.x, area.y};
    if (r->etapa == ETAPA_DECIFRAR || r->etapa == ETAPA_ESCOLHER_CONECTIVO) {
        p.y += desenha_frases(e, d, p);
        p.y += desenha_luzes(e, d, p);
        p.y += desenha_tabela(e, d, p);
        if (r->etapa == ETAPA_DECIFRAR) {
            TextoId t = r->resposta_t > 0.0f ? TXT_TERMINAL_RESPONDENDO : TXT_TERMINAL_OBSERVE;
            ui_texto_quebrado(e, dialogo_texto(t), p, area.width, 17 * s,
                              r->resposta_t > 0.0f ? COR_AMBAR : COR_FOSFORO);
        } else {
            desenha_opcoes_conectivo(e, d, p);
        }
    } else if (r->etapa == ETAPA_TRADUZIR) {
        desenha_traducao(e, d, p, area.width);
    } else {
        ui_texto(e, dialogo_texto(r->numero == 3 ? TXT_TERMINAL_RESPONDA : TXT_TERMINAL_CERTO),
                 (Vector2){p.x, p.y + 40 * s}, 40 * s, r->numero == 3 ? COR_ALARME : COR_FOSFORO);
    }
    desenha_rodape(e, area);
}
