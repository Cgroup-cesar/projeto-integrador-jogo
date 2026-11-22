// terminal.c — interface do ANALISADOR SEMÂNTICO SNI-85: frases em glifos, luzes, tabela-verdade ao vivo,
// escolha do conectivo e tradução. Só desenha a partir do estado; quem decide é rodadas.c.
#include "terminal.h"
#include "rodadas.h"
#include "glifos.h"
#include "criatura.h"
#include "susto.h"
#include "ui.h"
#include "dialogo.h"
#include <math.h>
#include <stdio.h>

// O terminal cuida da navegação (Esc sai, setas movem o cursor); quem decide o que o ENTER faz é rodadas.c.
void terminal_atualiza(Estado *e, float dt) {
    (void)dt; // a navegação é por tecla, sem tempo
    Rodada *r = &e->rod;
    if (e->jog.modo != MODO_TERMINAL || !regras_fase_eh_rodada(e->fase) || susto_ativo(e)) return;
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
    char operador[128]; // [REQ:NOME-USO] quem opera o analisador: "Operadora: Professora Inês"
    dialogo_personaliza(&e->ident, dialogo_texto(TXT_TERMINAL_OPERADOR), operador, sizeof(operador));
    // acima da moldura, à direita: no terminal o HUD de cima não aparece, então ali não disputa espaço
    ui_texto(e, operador, (Vector2){r.x + r.width - ui_mede(e, operador, 16 * s).x, r.y - 24 * s}, 16 * s,
             COR_AMBAR);
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

// ------------------------------------------------------------------ ícones (desenhados: não dependem da
// fonte)
static void icone_lampada(Vector2 c, float r, bool acesa) {
    if (acesa) DrawCircleV(c, r * 1.8f, Fade(COR_LAMPADA, 0.25f));
    DrawCircleV(c, r, acesa ? COR_LAMPADA : (Color){24, 30, 26, 255});
    DrawCircleLinesV(c, r, acesa ? COR_LAMPADA : Fade(COR_FOSFORO, 0.5f));
}

static void icone_marca(Vector2 c, float r, bool verdade) {
    float t = fmaxf(2.0f, r * 0.35f);
    if (verdade) {
        DrawLineEx((Vector2){c.x - r, c.y}, (Vector2){c.x - r * 0.3f, c.y + r * 0.7f}, t, COR_FOSFORO);
        DrawLineEx((Vector2){c.x - r * 0.3f, c.y + r * 0.7f}, (Vector2){c.x + r, c.y - r * 0.8f}, t,
                   COR_FOSFORO);
    } else {
        DrawLineEx((Vector2){c.x - r * 0.8f, c.y - r * 0.8f}, (Vector2){c.x + r * 0.8f, c.y + r * 0.8f}, t,
                   COR_ALARME);
        DrawLineEx((Vector2){c.x + r * 0.8f, c.y - r * 0.8f}, (Vector2){c.x - r * 0.8f, c.y + r * 0.8f}, t,
                   COR_ALARME);
    }
}

// ✓ VERDADE ou ✗ FALSO: palavra, cor e símbolo juntos (não depende só da cor, nem do som).
static float resposta(const Estado *e, Vector2 pos, float tamanho, bool verdade) {
    icone_marca((Vector2){pos.x + tamanho * 0.45f, pos.y + tamanho * 0.5f}, tamanho * 0.38f, verdade);
    const char *t = dialogo_texto(verdade ? TXT_TABELA_VERDADE : TXT_TABELA_FALSO);
    ui_texto(e, t, (Vector2){pos.x + tamanho * 1.1f, pos.y}, tamanho, verdade ? COR_FOSFORO : COR_ALARME);
    return tamanho * 1.1f + ui_mede(e, t, tamanho).x;
}

// O que cada glifo significa, embaixo dele; o conectivo ainda não decifrado aparece como "?".
static void rotulos_glifos(const Estado *e, const FraseGlifos *f, Vector2 pos, float tamanho) {
    float s = escala();
    for (int i = 0; i < f->n; i++) { // padrão MAP: um rótulo por glifo
        IdGlifo g = f->glifos[i];
        int c = glifo_conectivo(g);
        const char *sig = (c >= 0 && !e->conectivo_decifrado[c]) ? "?" : dialogo_significado_glifo(g);
        ui_texto(e, sig, (Vector2){pos.x + i * tamanho * 1.45f, pos.y + tamanho + 8 * s}, 13 * s,
                 Fade(glifo_cor(e, g), 0.9f));
    }
}

// ------------------------------------------------------------------ partes
static float desenha_frases(const Estado *e, const DadosRodada *d, Vector2 pos) {
    float s = escala();
    const Rodada *r = &e->rod;
    ui_texto(e, dialogo_texto(TXT_TERMINAL_FRASE_OBSERVADA), pos, 15 * s, Fade(COR_FOSFORO, 0.6f));
    for (int c = 0; c < d->n_colunas; c++) {
        Vector2 p = {pos.x + c * 270 * s, pos.y + 26 * s};
        bool perguntada = d->n_colunas > 1 && r->etapa == ETAPA_ESCOLHER_CONECTIVO && r->coluna == c;
        if (perguntada) // na R2, a frase que está sendo perguntada fica em destaque
            DrawRectangleRoundedLinesEx((Rectangle){p.x - 8 * s, p.y - 8 * s, 250 * s, 74 * s}, 0.15f, 4,
                                        2.0f * s, COR_AMBAR);
        if (d->n_colunas > 1)
            ui_texto(e, c == 0 ? "1" : "2", (Vector2){p.x, p.y + 8 * s}, 18 * s, Fade(COR_FOSFORO, 0.5f));
        Vector2 g = {p.x + (d->n_colunas > 1 ? 24 * s : 0), p.y};
        desenha_frase_glifos(e, &d->teste[c], g, 34 * s, 3.0f * s);
        rotulos_glifos(e, &d->teste[c], g, 34 * s);
    }
    return 100 * s;
}

static float desenha_luzes(const Estado *e, const DadosRodada *d, Vector2 pos) {
    float s = escala(), x = pos.x;
    for (int i = 0; i < d->n_luzes; i++) { // padrão MAP: cada luz com a lâmpada e o estado
        bool acesa = i == 0 ? e->rod.luz_a : e->rod.luz_b;
        icone_lampada((Vector2){x + 9 * s, pos.y + 9 * s}, 8 * s, acesa);
        char t[48];
        snprintf(t, sizeof(t), "%s: %s", dialogo_texto(i == 0 ? TXT_TABELA_LUZ_A : TXT_TABELA_LUZ_B),
                 dialogo_texto(acesa ? TXT_TERMINAL_LUZ_ACESA : TXT_TERMINAL_LUZ_APAGADA));
        ui_texto(e, t, (Vector2){x + 26 * s, pos.y}, 17 * s, COR_VIDRO);
        x += 26 * s + ui_mede(e, t, 17 * s).x + 34 * s;
    }
    return 34 * s;
}

// A tabela-verdade se preenche sozinha conforme ele responde. As luzes são lâmpadas (acesa/apagada) e as
// respostas, ✓ VERDADE ou ✗ FALSO. A linha das luzes atuais fica marcada.
static float desenha_tabela(const Estado *e, const DadosRodada *d, Vector2 pos) {
    float s = escala(), linha_h = 28 * s, col_luz = 72 * s, col_res = 190 * s;
    const Rodada *r = &e->rod;
    ui_texto(e, dialogo_texto(TXT_TERMINAL_TABELA), pos, 15 * s, Fade(COR_FOSFORO, 0.6f));
    float y = pos.y + 24 * s, x_res = pos.x + d->n_luzes * col_luz + 24 * s;
    for (int i = 0; i < d->n_luzes; i++)
        ui_texto(e, dialogo_texto(i == 0 ? TXT_TABELA_LUZ_A : TXT_TABELA_LUZ_B),
                 (Vector2){pos.x + i * col_luz, y}, 15 * s, Fade(COR_FOSFORO, 0.7f));
    for (int c = 0; c < d->n_colunas; c++) {
        char coluna[24];
        if (d->n_colunas > 1)
            snprintf(coluna, sizeof(coluna), dialogo_texto(TXT_TERMINAL_COLUNA), c + 1);
        else
            snprintf(coluna, sizeof(coluna), "%s", dialogo_texto(TXT_TABELA_RESPOSTA));
        bool perguntada = d->n_colunas > 1 && r->etapa == ETAPA_ESCOLHER_CONECTIVO && r->coluna == c;
        ui_texto(e, coluna, (Vector2){x_res + c * col_res, y}, 15 * s,
                 perguntada ? COR_AMBAR : Fade(COR_FOSFORO, 0.7f));
    }
    int atual = rodadas_linha_atual(e);
    for (int l = 0; l < rodadas_linhas(d); l++) {
        y += linha_h;
        if (l == atual && r->etapa == ETAPA_DECIFRAR)
            DrawRectangleRounded(
                (Rectangle){pos.x - 8 * s, y - 4 * s, x_res + d->n_colunas * col_res - pos.x, linha_h}, 0.3f,
                4, Fade(COR_FOSFORO, 0.12f));
        for (int i = 0; i < d->n_luzes; i++) {
            bool acesa = d->n_luzes == 1 ? (l & 1) : i == 0 ? ((l >> 1) & 1) : (l & 1);
            icone_lampada((Vector2){pos.x + i * col_luz + 18 * s, y + 9 * s}, 7 * s, acesa);
        }
        for (int c = 0; c < d->n_colunas; c++) {
            Vector2 p = {x_res + c * col_res, y};
            if (r->linha_vista[c][l])
                resposta(e, p, 18 * s, r->linha_valor[c][l]);
            else
                ui_texto(e, "?", (Vector2){p.x + 6 * s, p.y}, 18 * s, Fade(COR_FOSFORO, 0.3f));
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
    rotulos_glifos(e, &d->traducao, pos, 40 * s);
    float y = pos.y + 84 * s;
    ui_texto_quebrado(e, dialogo_texto(TXT_TERMINAL_TRADUZA), (Vector2){pos.x, y}, largura, 17 * s,
                      COR_FOSFORO);
    for (int i = 0; i < 3; i++)
        ui_opcao(e, dialogo_opcao_traducao(e->rod.numero, i),
                 (Vector2){pos.x + 10 * s, y + (34 + i * 34) * s}, largura - 10 * s, 20 * s,
                 e->rod.cursor == i, COR_FOSFORO);
}

// Decifrando: "mude as luzes", "...ele responde..." ou, logo depois da resposta, ✓ VERDADE / ✗ FALSO grande.
static void desenha_estado_decifrar(const Estado *e, Vector2 p, float largura) {
    float s = escala();
    const Rodada *r = &e->rod;
    if (r->clarao_t > 0.0f && r->resposta_t <= 0.0f) {
        resposta(e, (Vector2){p.x, p.y + 4 * s}, 38 * s, r->resposta_valor);
        return;
    }
    TextoId t = r->resposta_t > 0.0f ? TXT_TERMINAL_RESPONDENDO : TXT_TERMINAL_OBSERVE;
    ui_texto_quebrado(e, dialogo_texto(t), p, largura, 17 * s,
                      r->resposta_t > 0.0f ? COR_AMBAR : COR_FOSFORO);
}

static void desenha_rodape(const Estado *e, Rectangle area) {
    float s = escala();
    const Rodada *r = &e->rod;
    if (r->aviso_t > 0.0f) {
        bool erro = r->aviso == dialogo_texto(TXT_TERMINAL_ERRO);
        ui_texto(e, r->aviso, (Vector2){area.x, area.y + area.height - 34 * s}, 22 * s,
                 erro ? COR_ALARME : COR_FOSFORO);
    }
}

// ------------------------------------------------------------------ a cola (R1 e R2)
// Um conectivo: o nome, o que ele faz em palavras e a mini-tabela (lâmpadas → ✓/✗), para comparar.
static void conectivo_da_cola(const Estado *e, int indice, Rectangle b) {
    static const OpLogico ops[CONECTIVOS_ESCOLHA] = {OP_NAO, OP_E, OP_OU, OP_IMPLICA};
    static const TextoId descricao[CONECTIVOS_ESCOLHA] = {TXT_COLA_NAO, TXT_COLA_E, TXT_COLA_OU, TXT_COLA_SE};
    float s = escala();
    DrawRectangleRoundedLinesEx(b, 0.1f, 4, 1.0f * s, Fade(COR_FOSFORO, 0.3f));
    ui_texto(e, dialogo_conectivo(indice), (Vector2){b.x + 10 * s, b.y + 8 * s}, 18 * s, COR_FOSFORO);
    ui_texto_quebrado(e, dialogo_texto(descricao[indice]), (Vector2){b.x + 10 * s, b.y + 32 * s},
                      b.width * 0.52f, 13 * s, RAYWHITE);
    bool unario = ops[indice] == OP_NAO;
    int linhas = unario ? 2 : 4;
    float x = b.x + b.width * 0.6f, y = b.y + 14 * s;
    for (int l = 0; l < linhas; l++) { // padrão MAP: cada linha da mini-tabela
        bool a = unario ? (l & 1) : ((l >> 1) & 1), bb = l & 1;
        float yl = y + l * 25 * s;
        icone_lampada((Vector2){x + 8 * s, yl + 8 * s}, 6 * s, a);
        if (!unario) icone_lampada((Vector2){x + 28 * s, yl + 8 * s}, 6 * s, bb);
        icone_marca((Vector2){x + 62 * s, yl + 8 * s}, 7 * s, logica_aplica(ops[indice], a, bb));
    }
}

static void desenha_cola(const Estado *e) {
    const Rodada *r = &e->rod;
    if (r->numero > 2 || (r->etapa != ETAPA_DECIFRAR && r->etapa != ETAPA_ESCOLHER_CONECTIVO)) return;
    float s = escala();
    Rectangle c = {24 * s, 56 * s, GetScreenWidth() - 680 * s, 360 * s};
    ui_painel(c, COR_AMBAR);
    ui_texto(e, dialogo_texto(TXT_COLA_TITULO), (Vector2){c.x + 16 * s, c.y + 12 * s}, 17 * s, COR_AMBAR);
    float bw = (c.width - 44 * s) * 0.5f, bh = 136 * s;
    for (int i = 0; i < CONECTIVOS_ESCOLHA; i++) // padrão MAP: os quatro conectivos, em 2 × 2
        conectivo_da_cola(e, i,
                          (Rectangle){c.x + 16 * s + (i % 2) * (bw + 12 * s),
                                      c.y + 42 * s + (i / 2) * (bh + 10 * s), bw, bh});
    if (r->etapa == ETAPA_ESCOLHER_CONECTIVO)
        ui_texto(e, dialogo_texto(TXT_COLA_COMPARE), (Vector2){c.x + 16 * s, c.y + c.height - 26 * s}, 15 * s,
                 COR_FOSFORO);
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
            desenha_estado_decifrar(e, p, area.width);
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
    desenha_cola(e);
}
