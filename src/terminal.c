// terminal.c — interface do ANALISADOR SEMÂNTICO SNI-85: frases em glifos, luzes, tabela-verdade ao vivo,
// escolha do conectivo e tradução. Só desenha a partir do estado; quem decide é rodadas.c.
#include "terminal.h"
#include "rodadas.h"
#include "glifos.h"
#include "criatura.h"
#include "susto.h"
#include "ui.h"
#include "manual.h"
#include "dialogo.h"
#include "crt.h"
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
    if (e->in.ajuda) e->ajuda_aberta = !e->ajuda_aberta; // H: a cola dos conectivos, quando a pessoa quiser
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

// O monitor do analisador: fundo com scanlines e varredura, quinas, título com franja de cor e, embaixo dele,
// o `cabecalho` (NULL: quem chama desenha a linha de status). Devolve a área útil.
Rectangle terminal_moldura(const Estado *e, const char *cabecalho) {
    float s = escala();
    Rectangle r = {GetScreenWidth() - 620 * s, 56 * s, 596 * s, 600 * s};
    crt_painel(e, r, COR_FOSFORO, true);
    crt_titulo(e, dialogo_texto(TXT_TERMINAL_TITULO), (Vector2){r.x + 26 * s, r.y + 22 * s}, 24 * s,
               COR_FOSFORO);
    if (cabecalho) ui_texto(e, cabecalho, (Vector2){r.x + 26 * s, r.y + 54 * s}, 15 * s, COR_CRT_APAGADO);
    char operador[128]; // [REQ:NOME-USO] quem opera o analisador: "Operadora: Professora Inês"
    dialogo_personaliza(&e->ident, dialogo_texto(TXT_TERMINAL_OPERADOR), operador, sizeof(operador));
    // acima da moldura, à direita: no terminal o HUD de cima não aparece, então ali não disputa espaço
    ui_texto(e, operador, (Vector2){r.x + r.width - ui_mede(e, operador, 16 * s).x, r.y - 24 * s}, 16 * s,
             COR_AMBAR);
    if (e->diario_lido)
        ui_texto_quebrado(e, dialogo_texto(TXT_TERMINAL_ANOTACAO),
                          (Vector2){r.x + 20 * s, r.y + r.height + 10 * s}, r.width - 40 * s, 15 * s,
                          COR_AMBAR);
    return (Rectangle){r.x + 26 * s, r.y + 90 * s, r.width - 52 * s, r.height - 104 * s};
}

// Um rótulo apagado seguido do valor em verde ("RODADA 1/3"); devolve a largura usada.
static float status_item(const Estado *e, const char *rotulo, const char *valor, Vector2 p, float t) {
    float s = escala(), w = ui_mede(e, rotulo, t).x + 8 * s;
    ui_texto(e, rotulo, p, t, COR_CRT_APAGADO);
    if (valor) ui_texto(e, valor, (Vector2){p.x + w, p.y}, t, COR_FOSFORO);
    return w + (valor ? ui_mede(e, valor, t).x : 0.0f) + 20 * s;
}

// Linha de status: RODADA 1/3 · VIDRO ■■■ (os quadrados acesos brilham) · PLACAR 000.
static Rectangle moldura_da_rodada(const Estado *e) {
    float s = escala(), t = 15 * s;
    Rectangle area = terminal_moldura(e, NULL);
    char rodada[16], placar[16];
    snprintf(rodada, sizeof(rodada), "%d/3", e->rod.numero);
    snprintf(placar, sizeof(placar), "%03d", e->placar);
    Vector2 p = {area.x, area.y - 36 * s};
    p.x += status_item(e, dialogo_texto(TXT_TERMINAL_CABECALHO), rodada, p, t);
    p.x += status_item(e, dialogo_texto(TXT_HUD_VIDRO), NULL, p, t) - 12 * s;
    Color cor = e->vidas <= 1 ? COR_ALARME : COR_FOSFORO;
    for (int i = 0; i < VIDAS_VIDRO; i++) { // padrão MAP: um quadrado por vida do vidro
        Rectangle q = {p.x + i * 18 * s, p.y + 1 * s, 14 * s, 14 * s};
        if (i < e->vidas) // o brilho em volta do quadrado aceso
            DrawRectangleRec((Rectangle){q.x - 2 * s, q.y - 2 * s, 18 * s, 18 * s}, Fade(cor, 0.2f));
        DrawRectangleRec(q, i < e->vidas ? cor : (Color){18, 48, 31, 255});
    }
    p.x += VIDAS_VIDRO * 18 * s + 16 * s;
    status_item(e, dialogo_texto(TXT_TERMINAL_PLACAR), placar, p, t);
    return area;
}

// ------------------------------------------------------------------ ícones (em manual.c)

// ✓ VERDADE ou ✗ FALSO: palavra, cor e símbolo juntos (não depende só da cor, nem do som).
static float resposta(const Estado *e, Vector2 pos, float tamanho, bool verdade, float alfa) {
    manual_marca((Vector2){pos.x + tamanho * 0.45f, pos.y + tamanho * 0.5f}, tamanho * 0.38f, verdade, alfa);
    const char *t = dialogo_texto(verdade ? TXT_TABELA_VERDADE : TXT_TABELA_FALSO);
    ui_texto(e, t, (Vector2){pos.x + tamanho * 1.1f, pos.y}, tamanho,
             Fade(verdade ? COR_FOSFORO : COR_ALARME, alfa));
    return tamanho * 1.1f + ui_mede(e, t, tamanho).x;
}

// O que cada glifo significa, embaixo dele; o conectivo ainda não decifrado aparece como "?".
static void rotulos_glifos(const Estado *e, const FraseGlifos *f, Vector2 pos, float tamanho) {
    float s = escala();
    for (int i = 0; i < f->n; i++) { // padrão MAP: um rótulo por glifo
        IdGlifo g = f->glifos[i];
        int c = glifo_conectivo(g);
        const char *sig = (c >= 0 && !e->conectivo_decifrado[c]) ? "?" : dialogo_significado_glifo(g);
        Vector2 m = ui_mede(e, sig, 13 * s);
        float cx = pos.x + i * tamanho * 1.45f + tamanho * 0.5f;
        ui_texto(e, sig, (Vector2){cx - m.x * 0.5f, pos.y + tamanho + 8 * s}, 13 * s,
                 Fade(glifo_cor(e, g), 0.9f));
    }
}

// O brilho atrás de cada glifo: âmbar pulsando no conectivo ainda desconhecido, verde fraco nos outros.
static void brilho_glifos(const Estado *e, const FraseGlifos *f, Vector2 pos, float tamanho) {
    float pulso = 0.5f + 0.5f * sinf(e->tempo_total * 2.0f * PI / 2.2f);
    for (int i = 0; i < f->n; i++) { // padrão MAP: um halo por glifo
        int c = glifo_conectivo(f->glifos[i]);
        bool novo = c >= 0 && !e->conectivo_decifrado[c];
        Vector2 centro = {pos.x + i * tamanho * 1.45f + tamanho * 0.5f, pos.y + tamanho * 0.5f};
        DrawCircleV(centro, tamanho * 0.85f,
                    novo ? Fade(COR_AMBAR, 0.08f + 0.10f * pulso) : Fade(COR_FOSFORO, 0.06f));
    }
}

// ------------------------------------------------------------------ partes
// FRASE OBSERVADA: os glifos com o significado embaixo. Na R2, as duas frases, e a perguntada em âmbar.
static void desenha_frases(const Estado *e, const DadosRodada *d, Rectangle caixa) {
    float s = escala();
    const Rodada *r = &e->rod;
    bool escolhendo = d->n_colunas > 1 && r->etapa == ETAPA_ESCOLHER_CONECTIVO;
    if (escolhendo) { // qual das duas frases está em análise, à direita da legenda
        char analise[48];
        snprintf(analise, sizeof(analise), dialogo_texto(TXT_TERMINAL_EM_ANALISE), r->coluna + 1);
        float x = caixa.x + caixa.width - ui_mede(e, analise, 13 * s).x;
        ui_texto(e, analise, (Vector2){x, caixa.y - 24 * s}, 13 * s, COR_AMBAR);
    }
    for (int c = 0; c < d->n_colunas; c++) {
        Vector2 p = {caixa.x + c * 270 * s, caixa.y + 4 * s};
        if (escolhendo && r->coluna == c) // na R2, a frase que está sendo perguntada fica em destaque
            DrawRectangleLinesEx((Rectangle){p.x - 8 * s, p.y - 8 * s, 250 * s, 70 * s}, 1.5f * s, COR_AMBAR);
        if (d->n_colunas > 1)
            ui_texto(e, c == 0 ? "1" : "2", (Vector2){p.x, p.y + 8 * s}, 18 * s, COR_CRT_APAGADO);
        Vector2 g = {p.x + (d->n_colunas > 1 ? 24 * s : 0), p.y};
        brilho_glifos(e, &d->teste[c], g, 34 * s);
        desenha_frase_glifos(e, &d->teste[c], g, 34 * s, 3.0f * s);
        rotulos_glifos(e, &d->teste[c], g, 34 * s);
    }
}

// ESTADO DA LUZ: cada luz com a lâmpada e "LUZ A: ACESA"; quebra a linha quando não cabe.
static void desenha_luzes(const Estado *e, const DadosRodada *d, Rectangle caixa) {
    float s = escala(), x = caixa.x, y = caixa.y + 2 * s;
    for (int i = 0; i < d->n_luzes; i++) { // padrão MAP: cada luz com a lâmpada e o estado
        bool acesa = i == 0 ? e->rod.luz_a : e->rod.luz_b;
        char t[48];
        snprintf(t, sizeof(t), "%s: %s", dialogo_texto(i == 0 ? TXT_TABELA_LUZ_A : TXT_TABELA_LUZ_B),
                 dialogo_texto(acesa ? TXT_TERMINAL_LUZ_ACESA : TXT_TERMINAL_LUZ_APAGADA));
        float w = 36 * s + ui_mede(e, t, 16 * s).x;
        if (x > caixa.x && x + w > caixa.x + caixa.width) x = caixa.x, y += 36 * s;
        manual_lampada((Vector2){x + 12 * s, y + 9 * s}, 9 * s, acesa);
        ui_texto(e, t, (Vector2){x + 36 * s, y}, 16 * s, COR_VIDRO);
        x += w + 30 * s;
    }
}

// Topo: FRASE OBSERVADA e ESTADO DA LUZ lado a lado; na R2 (duas frases) a frase ocupa a largura toda e a
// luz só aparece enquanto ele está decifrando. Devolve a altura usada.
static float desenha_topo(const Estado *e, const DadosRodada *d, Rectangle area) {
    float s = escala(), gap = 14 * s, h = 106 * s;
    bool lado = d->n_colunas == 1, luzes = lado || e->rod.etapa == ETAPA_DECIFRAR;
    float wf = lado ? (area.width - gap) * 0.5f : area.width;
    const char *legenda = dialogo_texto(TXT_TERMINAL_FRASE_OBSERVADA);
    desenha_frases(e, d, crt_caixa(e, (Rectangle){area.x, area.y, wf, h}, legenda));
    if (!luzes) return h + gap;
    Rectangle rl = lado ? (Rectangle){area.x + wf + gap, area.y, wf, h}
                        : (Rectangle){area.x, area.y + h + gap, area.width, 70 * s};
    desenha_luzes(e, d, crt_caixa(e, rl, dialogo_texto(TXT_TERMINAL_ESTADO_LUZ)));
    return lado ? h + gap : h + gap + rl.height + gap;
}

// O cabeçalho de cada coluna de respostas: RESPOSTA, "frase 1" ou, na R2, "FRASE 1 · EM ANÁLISE" em âmbar.
static void cabecalho_coluna(const Estado *e, const DadosRodada *d, int c, Vector2 p, float linhas_h) {
    float s = escala(), col_res = 190 * s;
    const Rodada *r = &e->rod;
    bool escolhendo = d->n_colunas > 1 && r->etapa == ETAPA_ESCOLHER_CONECTIVO;
    bool perguntada = escolhendo && r->coluna == c;
    char coluna[48];
    if (perguntada) // a coluna em análise: moldura âmbar em volta do cabeçalho e das respostas
        DrawRectangleLinesEx((Rectangle){p.x - 10 * s, p.y - 6 * s, col_res - 8 * s, linhas_h + 6 * s},
                             1.5f * s, COR_AMBAR);
    if (perguntada)
        snprintf(coluna, sizeof(coluna), dialogo_texto(TXT_TERMINAL_EM_ANALISE), c + 1);
    else if (d->n_colunas > 1)
        snprintf(coluna, sizeof(coluna), dialogo_texto(TXT_TERMINAL_COLUNA), c + 1);
    else
        snprintf(coluna, sizeof(coluna), "%s", dialogo_texto(TXT_TABELA_RESPOSTA));
    ui_texto(e, coluna, p, 13 * s, perguntada ? COR_AMBAR : Fade(COR_CRT_APAGADO, escolhendo ? 0.5f : 1.0f));
}

// TABELA-VERDADE numa caixa: se preenche sozinha conforme ele responde. As luzes são lâmpadas e as
// respostas, ✓ VERDADE ou ✗ FALSO; a linha das luzes atuais ganha o degradê âmbar. Devolve a altura.
static float desenha_tabela(const Estado *e, const DadosRodada *d, Rectangle area) {
    float s = escala(), linha_h = 26 * s, col_luz = 72 * s, col_res = 190 * s;
    const Rodada *r = &e->rod;
    int linhas = rodadas_linhas(d), atual = rodadas_linha_atual(e);
    float altura = 36 * s + 24 * s + linhas * linha_h + 12 * s;
    Rectangle in =
        crt_caixa(e, (Rectangle){area.x, area.y, area.width, altura}, dialogo_texto(TXT_TERMINAL_TABELA));
    float y = in.y, x_res = in.x + d->n_luzes * col_luz + 24 * s;
    for (int i = 0; i < d->n_luzes; i++)
        ui_texto(e, dialogo_texto(i == 0 ? TXT_TABELA_LUZ_A : TXT_TABELA_LUZ_B),
                 (Vector2){in.x + i * col_luz, y}, 13 * s, COR_CRT_APAGADO);
    for (int c = 0; c < d->n_colunas; c++)
        cabecalho_coluna(e, d, c, (Vector2){x_res + c * col_res, y}, (linhas + 1) * linha_h);
    bool escolhendo = d->n_colunas > 1 && r->etapa == ETAPA_ESCOLHER_CONECTIVO;
    y += 22 * s;
    for (int l = 0; l < linhas; l++, y += linha_h) {
        DrawLineEx((Vector2){in.x, y}, (Vector2){in.x + in.width, y}, 1.0f * s, COR_CRT_GRADE);
        if (l == atual && r->etapa == ETAPA_DECIFRAR)
            DrawRectangleGradientH((int)in.x, (int)y, (int)in.width, (int)linha_h, Fade(COR_AMBAR, 0.16f),
                                   Fade(COR_AMBAR, 0.0f));
        for (int i = 0; i < d->n_luzes; i++) {
            bool acesa = d->n_luzes == 1 ? (l & 1) : i == 0 ? ((l >> 1) & 1) : (l & 1);
            manual_lampada((Vector2){in.x + i * col_luz + 14 * s, y + linha_h * 0.5f}, 7 * s, acesa);
        }
        for (int c = 0; c < d->n_colunas; c++) {
            Vector2 p = {x_res + c * col_res, y + 5 * s};
            float alfa = escolhendo && r->coluna != c ? 0.25f : 1.0f; // a outra frase fica apagada
            if (r->linha_vista[c][l])
                resposta(e, p, 17 * s, r->linha_valor[c][l], alfa);
            else
                ui_texto(e, "?", (Vector2){p.x + 6 * s, p.y}, 17 * s, Fade(COR_FOSFORO, 0.3f));
        }
    }
    return altura + 14 * s;
}

// Errou agora há pouco? A opção escolhida treme vermelha nos primeiros instantes do aviso.
static bool errou_agora(const Estado *e) {
    const Rodada *r = &e->rod;
    return r->aviso_t > TERMINAL_TREMIDA_ERRO_S && r->aviso == dialogo_texto(TXT_TERMINAL_ERRO);
}

// Na R2, qual frase está em análise já aparece na caixa da frase e na tabela: aqui só a pergunta.
static void desenha_opcoes_conectivo(const Estado *e, Vector2 pos, float largura) {
    float s = escala();
    crt_pergunta(e, dialogo_texto(TXT_TERMINAL_ESCOLHA_CONECTIVO), pos, 16 * s);
    for (int i = 0; i < CONECTIVOS_ESCOLHA; i++) // padrão MAP: cada conectivo numa faixa
        crt_opcao(e, dialogo_conectivo(i), (Rectangle){pos.x, pos.y + (28 + i * 34) * s, largura, 30 * s},
                  18 * s, e->rod.cursor == i, errou_agora(e), true, COR_FOSFORO);
}

// Tradução: a frase com o significado de cada glifo embaixo (conectivo só se já foi decifrado).
static void desenha_traducao(const Estado *e, const DadosRodada *d, Vector2 pos, float largura) {
    float s = escala();
    Rectangle in = crt_caixa(e, (Rectangle){pos.x, pos.y, largura, 120 * s},
                             dialogo_texto(TXT_TERMINAL_FRASE_OBSERVADA));
    brilho_glifos(e, &d->traducao, (Vector2){in.x, in.y + 2 * s}, 40 * s);
    desenha_frase_glifos(e, &d->traducao, (Vector2){in.x, in.y + 2 * s}, 40 * s, 3.2f * s);
    rotulos_glifos(e, &d->traducao, (Vector2){in.x, in.y + 2 * s}, 40 * s);
    float y = pos.y + 136 * s;
    crt_pergunta(e, dialogo_texto(TXT_TERMINAL_TRADUZA), (Vector2){pos.x, y}, 16 * s);
    for (int i = 0; i < 3; i++) // padrão MAP: cada tradução numa faixa
        crt_opcao(e, dialogo_opcao_traducao(e->rod.numero, i),
                  (Rectangle){pos.x, y + (30 + i * 42) * s, largura, 34 * s}, 17 * s, e->rod.cursor == i,
                  errou_agora(e), true, COR_FOSFORO);
}

// Decifrando: "mude as luzes", "...ele responde..." ou, logo depois da resposta, ✓ VERDADE / ✗ FALSO grande.
static void desenha_estado_decifrar(const Estado *e, Vector2 p, float largura) {
    float s = escala();
    const Rodada *r = &e->rod;
    if (r->clarao_t > 0.0f && r->resposta_t <= 0.0f) {
        resposta(e, (Vector2){p.x, p.y + 4 * s}, 38 * s, r->resposta_valor, 1.0f);
        return;
    }
    bool duas = rodadas_dados(r->numero)->n_luzes == 2;
    TextoId t = r->resposta_t > 0.0f ? TXT_TERMINAL_RESPONDENDO
                : duas               ? TXT_TERMINAL_OBSERVE_DUAS
                                     : TXT_TERMINAL_OBSERVE;
    ui_texto_quebrado(e, dialogo_texto(t), p, largura, 16 * s,
                      r->resposta_t > 0.0f ? COR_AMBAR : COR_CRT_TEXTO);
}

static void desenha_rodape(const Estado *e, Rectangle area) {
    float s = escala();
    const Rodada *r = &e->rod;
    if (r->etapa != ETAPA_CONCLUIDA) // o botão da ajuda, no alto à direita, na linha do título
        manual_selo_ajuda(e, area.x + area.width, area.y - 70 * s, s);
    if (r->aviso_t > 0.0f) {
        bool erro = r->aviso == dialogo_texto(TXT_TERMINAL_ERRO);
        ui_texto(e, r->aviso, (Vector2){area.x, area.y + area.height - 22 * s}, 18 * s,
                 erro ? COR_ALARME : COR_FOSFORO);
    }
}

// ------------------------------------------------------------------ a cola (R1 e R2)

// Um conectivo: o nome, o que ele faz em palavras e a mini-tabela (lâmpadas → ✓/✗), para comparar.
static void conectivo_da_cola(const Estado *e, int indice, Rectangle b, float s) {
    static const OpLogico ops[CONECTIVOS_ESCOLHA] = {OP_NAO, OP_E, OP_OU, OP_IMPLICA};
    static const TextoId descricao[CONECTIVOS_ESCOLHA] = {TXT_COLA_NAO, TXT_COLA_E, TXT_COLA_OU, TXT_COLA_SE};
    bool unario = ops[indice] == OP_NAO;
    int valores[4];
    for (int l = 0; l < 4; l++) // padrão MAP: o resultado do conectivo em cada linha
        valores[l] = logica_aplica(ops[indice], unario ? (l & 1) : ((l >> 1) & 1), l & 1);
    manual_cartao(e, b, dialogo_conectivo(indice), dialogo_texto(descricao[indice]), unario, valores, false,
                  s);
}

// A tabela da pessoa (a frase em análise) no mesmo formato dos cartões: comparar vira achar o cartão igual.
static void sua_tabela_na_cola(const Estado *e, Rectangle b, float s) {
    const Rodada *r = &e->rod;
    const DadosRodada *d = rodadas_dados(r->numero);
    DrawRectangleGradientV((int)b.x, (int)b.y, (int)b.width, (int)b.height, Fade(COR_AMBAR, 0.16f),
                           Fade(COR_AMBAR, 0.04f));
    DrawRectangleLinesEx(b, 1.5f * s, COR_AMBAR);
    char titulo[48];
    if (d->n_colunas > 1)
        snprintf(titulo, sizeof(titulo), dialogo_texto(TXT_COLA_SUA_FRASE), r->coluna + 1);
    else
        snprintf(titulo, sizeof(titulo), "%s", dialogo_texto(TXT_COLA_SUA_TABELA));
    ui_texto_quebrado(e, titulo, (Vector2){b.x + 10 * s, b.y + 10 * s}, b.width - 20 * s, 16 * s, COR_AMBAR);
    int valores[4];
    for (int l = 0; l < 4; l++) // padrão MAP: o que ele respondeu em cada linha (−1: ainda não visto)
        valores[l] = r->linha_vista[r->coluna][l] ? r->linha_valor[r->coluna][l] : -1;
    manual_mini_tabela(e, (Vector2){b.x + (b.width - 76 * s) * 0.5f, b.y + 44 * s}, d->n_luzes == 1, valores,
                       s);
}

static void desenha_cola(const Estado *e) {
    const Rodada *r = &e->rod;
    // Só quando a pessoa pede (H). Enquanto ele fala, ela some: os símbolos dele aparecem desse lado do
    // vidro.
    if (!e->ajuda_aberta || e->cri.fala_t > 0.0f || r->etapa == ETAPA_CONCLUIDA) return;
    // A cola ocupa o espaço à esquerda do terminal. Em telas mais altas (16:10) esse espaço fica mais
    // estreito que o desenho dela (600 unidades): então tudo nela — cartões, lâmpadas e letras — encolhe
    // junto.
    float g = escala(), livre = GetScreenWidth() - 680 * g, s = g * fminf(1.0f, livre / (600 * g));
    Rectangle c = {24 * g, 66 * g, livre, 360 * s}; // abaixo da barra de sanidade (vhs/sanidade.c)
    crt_painel(e, c, COR_AMBAR, false); // o manual: mesma moldura de monitor, com a borda âmbar
    ui_texto(e, dialogo_texto(TXT_COLA_TITULO), (Vector2){c.x + 16 * s, c.y + 12 * s}, 17 * s, COR_AMBAR);
    float bh = 136 * s, faixa = 118 * s, x0 = c.x + 16 * s + faixa + 12 * s;
    float bw = (c.x + c.width - 16 * s - x0 - 12 * s) * 0.5f;
    sua_tabela_na_cola(e, (Rectangle){c.x + 16 * s, c.y + 42 * s, faixa, 2 * bh + 10 * s}, s);
    for (int i = 0; i < CONECTIVOS_ESCOLHA; i++) // padrão MAP: os quatro conectivos, em 2 × 2
        conectivo_da_cola(
            e, i, (Rectangle){x0 + (i % 2) * (bw + 12 * s), c.y + 42 * s + (i / 2) * (bh + 10 * s), bw, bh},
            s);
    if (r->etapa == ETAPA_ESCOLHER_CONECTIVO) { // encolhe a letra até caber na largura do painel
        const char *dica = dialogo_texto(TXT_COLA_COMPARE);
        float t = 15 * s, cabe = c.width - 32 * s, mede = ui_mede(e, dica, t).x;
        if (mede > cabe) t *= cabe / mede;
        ui_texto(e, dica, (Vector2){c.x + 16 * s, c.y + c.height - 26 * s}, t, COR_CRT_APAGADO);
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
        p.y += desenha_topo(e, d, area);
        p.y += desenha_tabela(e, d, (Rectangle){p.x, p.y, area.width, 0});
        if (r->etapa == ETAPA_DECIFRAR)
            desenha_estado_decifrar(e, p, area.width);
        else
            desenha_opcoes_conectivo(e, p, area.width);
    } else if (r->etapa == ETAPA_TRADUZIR) {
        desenha_traducao(e, d, p, area.width);
    } else {
        crt_titulo(e, dialogo_texto(r->numero == 3 ? TXT_TERMINAL_RESPONDA : TXT_TERMINAL_CERTO),
                   (Vector2){p.x, p.y + 40 * s}, 40 * s, r->numero == 3 ? COR_ALARME : COR_FOSFORO);
    }
    desenha_rodape(e, area);
    desenha_cola(e);
}
