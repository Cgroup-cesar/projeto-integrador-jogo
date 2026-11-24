// teclas.c — as teclas que valem agora, escolhidas pelo contexto (terminal, painel, andando, armário,
// bilhete). Na partida, a tela fica limpa: só um "B Comandos" pequeno no canto inferior esquerdo, e o B abre
// e fecha a lista das teclas logo acima dele. No menu e nas telas depois do final, a barra do rodapé.
#include "teclas.h"
#include "botoes.h"
#include "rodadas.h"
#include "susto.h"
#include "ui.h"
#include "dialogo.h"
#include "respiracao.h"
#include "clique.h"
#include <string.h>

// Uma tecla e o que ela faz agora. Rótulos especiais: "SETAS_V" (↑↓), "SETAS_H" (←→), "SETAS" (as quatro).
typedef struct {
    const char *tecla;
    TextoId acao;
} TeclaAcao;

#define MAX_ITENS 7

// ================================================================== que teclas valem agora
static int itens_do_terminal(const Estado *e, TeclaAcao *t) {
    int n = 0;
    if (e->fase == FASE_RESPOSTA_FINAL) {
        t[n++] = (TeclaAcao){"SETAS", TXT_TECLA_MOVER};
        t[n++] = (TeclaAcao){"ENTER", TXT_TECLA_USAR_FICHA};
    } else if (e->rod.etapa == ETAPA_DECIFRAR) {
        t[n++] = (TeclaAcao){"1", TXT_TECLA_LUZ_A};
        if (rodadas_dados(e->rod.numero)->n_luzes == 2) t[n++] = (TeclaAcao){"2", TXT_TECLA_LUZ_B};
    } else if (e->rod.etapa != ETAPA_CONCLUIDA) {
        t[n++] = (TeclaAcao){"SETAS_V", TXT_TECLA_ESCOLHER};
        t[n++] = (TeclaAcao){"ENTER", TXT_TECLA_CONFIRMAR};
    }
    if (e->fase != FASE_RESPOSTA_FINAL && e->rod.etapa != ETAPA_CONCLUIDA)
        t[n++] = (TeclaAcao){"H", e->ajuda_aberta ? TXT_TECLA_FECHAR_AJUDA : TXT_TECLA_AJUDA};
    t[n++] = (TeclaAcao){"ESC", TXT_TECLA_SAIR};
    return n;
}

static int itens_do_painel(const Estado *e, TeclaAcao *t) {
    int n = 0;
    if (e->painel.numero == 1) {
        t[n++] = (TeclaAcao){"1 2 3", TXT_TECLA_CHAVES}; // a tecla de cada chave também está na linha dela
    } else {
        t[n++] = (TeclaAcao){"SETAS_H", TXT_TECLA_ENCAIXE};
        t[n++] = (TeclaAcao){"SETAS_V", TXT_TECLA_PORTA};
    }
    t[n++] = (TeclaAcao){"H", e->ajuda_aberta ? TXT_TECLA_FECHAR_AJUDA : TXT_TECLA_AJUDA};
    t[n++] = (TeclaAcao){"ESC", TXT_TECLA_FECHAR};
    return n;
}

static int itens_andando(const Estado *e, TeclaAcao *t) {
    int n = 0;
    t[n++] = (TeclaAcao){"WASD", TXT_TECLA_ANDAR};
    t[n++] = (TeclaAcao){"MOUSE", TXT_TECLA_OLHAR};
    if (regras_fase_eh_saida(e->fase)) t[n++] = (TeclaAcao){"SHIFT", TXT_TECLA_CORRER};
    t[n++] = (TeclaAcao){"E", TXT_TECLA_INTERAGIR};
    t[n++] = (TeclaAcao){"F", TXT_TECLA_LANTERNA};
    t[n++] = (TeclaAcao){"ESC", TXT_TECLA_PAUSA};
    return n;
}

// Depois do final: no desempenho, as estrelas; no ranking, só voltar ao menu.
static int itens_do_placar(const Estado *e, TeclaAcao *t) {
    if (e->fim.tela == 1) {
        t[0] = (TeclaAcao){"ENTER", TXT_TECLA_MENU};
        return 1;
    }
    t[0] = (TeclaAcao){"SETAS_V", TXT_TECLA_PERGUNTA};
    t[1] = (TeclaAcao){"SETAS_H", TXT_TECLA_ESTRELAS};
    t[2] = (TeclaAcao){"ENTER", TXT_TECLA_ENVIAR};
    return 3;
}

static int itens_do_menu(const Estado *e, TeclaAcao *t) {
    if (e->menu.ranking_aberto) {
        t[0] = (TeclaAcao){"ENTER", TXT_TECLA_FECHAR}; // o ESC também fecha
        return 1;
    }
    t[0] = (TeclaAcao){"SETAS_V", TXT_TECLA_ESCOLHER};
    t[1] = (TeclaAcao){"ENTER", TXT_TECLA_CONFIRMAR};
    t[2] = (TeclaAcao){"WASD", TXT_TECLA_ANDAR}; // já avisa como se anda antes da partida começar
    return 3;
}

static int itens_do_contexto(const Estado *e, TeclaAcao *t) {
    bool fim = e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM;
    if (fim) return itens_do_placar(e, t);
    if (e->fase == FASE_MENU) return itens_do_menu(e, t);
    switch (e->jog.modo) {
    case MODO_TERMINAL:
        return itens_do_terminal(e, t);
    case MODO_PAINEL:
        return itens_do_painel(e, t);
    case MODO_ARMARIO:
        if (e->resp.etapa == RESP_ALIVIO) return 0; // ele está indo embora: espere
        t[0] = respiracao_ativa(e) ? (TeclaAcao){"ESPAÇO", TXT_TECLA_RESPIRAR}
                                   : (TeclaAcao){"E", TXT_TECLA_SAIR_ARMARIO};
        return 1;
    case MODO_DIARIO:
        t[0] = (TeclaAcao){"E", TXT_TECLA_FECHAR};
        return 1;
    default:
        return itens_andando(e, t);
    }
}

// ================================================================== controle
// Com o controle em uso, o rótulo da tecla vira o botão (as setas ficam: são o direcional). Devolve NULL
// para o que não tem botão próprio (o atalho "1 2 3" do painel: lá o direcional + A já faz tudo).
typedef struct {
    const char *tecla, *botao;
} TeclaBotao;

static const TeclaBotao BOTOES[] = {
    {"WASD", "L"}, {"MOUSE", "R"}, {"SHIFT", "LB"}, {"E", "A"}, {"ENTER", "A"}, {"ESPAÇO", "A"},
    {"ESC", "B"},  {"F", "Y"},     {"H", "VIEW"},   {"1", "X"}, {"2", "Y"},
};

static const char *rotulo(const Estado *e, const TeclaAcao *t) {
    if (!e->in.controle) return t->tecla;
    if (strcmp(t->tecla, "1 2 3") == 0) return NULL;
    if (strcmp(t->tecla, "ESC") == 0 && t->acao == TXT_TECLA_PAUSA) return "START"; // andando, o B não pausa
    int total = (int)(sizeof(BOTOES) / sizeof(BOTOES[0]));
    for (int i = 0; i < total; i++) // padrão FOLD: o botão desta tecla (ou ela mesma, se for seta)
        if (strcmp(BOTOES[i].tecla, t->tecla) == 0) return BOTOES[i].botao;
    return t->tecla;
}

// ================================================================== desenho de uma tecla
static void seta(Vector2 c, float r, int direcao, Color cor) { // 0 cima, 1 baixo, 2 esquerda, 3 direita
    Vector2 a, b, d;
    if (direcao == 0)
        a = (Vector2){c.x, c.y - r}, b = (Vector2){c.x - r, c.y + r * 0.6f},
        d = (Vector2){c.x + r, c.y + r * 0.6f};
    else if (direcao == 1)
        a = (Vector2){c.x, c.y + r}, b = (Vector2){c.x + r, c.y - r * 0.6f},
        d = (Vector2){c.x - r, c.y - r * 0.6f};
    else if (direcao == 2)
        a = (Vector2){c.x - r, c.y}, b = (Vector2){c.x + r * 0.6f, c.y + r},
        d = (Vector2){c.x + r * 0.6f, c.y - r};
    else
        a = (Vector2){c.x + r, c.y}, b = (Vector2){c.x - r * 0.6f, c.y - r},
        d = (Vector2){c.x - r * 0.6f, c.y + r};
    DrawTriangle(a, b, d, cor);
}

// No controle, o botão desenhado (assets/botoes/), com sombra, centrado na altura da tecla. Devolve a largura
// usada, ou 0 se não houver ícone (aí a tecla vira a caixinha com o nome).
static float icone(const Estado *e, const char *rotulo, Vector2 pos, float altura, Color cor) {
    if (!e->in.controle) return 0;
    float s = ui_escala(), h = altura * 1.15f, y = pos.y - (h - altura) * 0.5f;
    float largura = botoes_desenha(e, rotulo, (Vector2){pos.x + 2 * s, y + 3 * s}, h, Fade(BLACK, 0.8f));
    if (largura > 0) botoes_desenha(e, rotulo, (Vector2){pos.x, y}, h, cor);
    return largura;
}

// Desenha a tecla como uma tecla de verdade (caixinha com borda) e devolve a largura usada.
static float tecla(const Estado *e, const char *rotulo, Vector2 pos, float altura, Color cor) {
    float usada = icone(e, rotulo, pos, altura, cor);
    if (usada > 0) return usada;
    float s = ui_escala(), tamanho = altura * 0.55f;
    int setas = strcmp(rotulo, "SETAS_V") == 0   ? 2
                : strcmp(rotulo, "SETAS_H") == 0 ? 2
                : strcmp(rotulo, "SETAS") == 0   ? 4
                                                 : 0;
    float largura =
        setas ? altura * (setas == 4 ? 2.2f : 1.5f) : fmaxf(altura, ui_mede(e, rotulo, tamanho).x + 16 * s);
    Rectangle r = {pos.x, pos.y, largura, altura};
    DrawRectangleRounded((Rectangle){r.x, r.y + 3 * s, r.width, r.height}, 0.25f, 4,
                         (Color){0, 0, 0, 200}); // sombra
    DrawRectangleRounded(r, 0.25f, 4, (Color){22, 34, 26, 255});
    DrawRectangleRoundedLinesEx(r, 0.25f, 4, 1.5f * s, cor);
    Vector2 c = {r.x + r.width * 0.5f, r.y + r.height * 0.5f};
    float sr = altura * 0.17f;
    if (strcmp(rotulo, "SETAS_V") == 0) {
        seta((Vector2){c.x - sr * 1.6f, c.y}, sr, 0, cor);
        seta((Vector2){c.x + sr * 1.6f, c.y}, sr, 1, cor);
    } else if (strcmp(rotulo, "SETAS_H") == 0) {
        seta((Vector2){c.x - sr * 1.6f, c.y}, sr, 2, cor);
        seta((Vector2){c.x + sr * 1.6f, c.y}, sr, 3, cor);
    } else if (setas == 4) {
        for (int d = 0; d < 4; d++) // padrão MAP: uma seta por direção
            seta((Vector2){c.x + (d - 1.5f) * sr * 2.6f, c.y}, sr * 0.9f, d, cor);
    } else {
        Vector2 m = ui_mede(e, rotulo, tamanho);
        ui_texto(e, rotulo, (Vector2){c.x - m.x * 0.5f, c.y - m.y * 0.5f}, tamanho, cor);
    }
    return largura;
}

// ================================================================== a barra
// Que tecla um clique nesta parte da barra aperta (as setas não: elas só andam pelas opções, que já são
// clicáveis direto na tela).
static TeclaClique tecla_clicavel(const char *rotulo) {
    static const struct {
        const char *rotulo;
        TeclaClique tecla;
    } TABELA[] = {{"ESC", TECLA_ESC}, {"ENTER", TECLA_ENTER},   {"H", TECLA_H},
                  {"1", TECLA_1},     {"2", TECLA_2},           {"E", TECLA_E},
                  {"F", TECLA_F},     {"ESPAÇO", TECLA_ESPACO}, {"B", TECLA_B}};
    for (size_t i = 0; i < sizeof(TABELA) / sizeof(TABELA[0]); i++) // padrão FOLD: procura o rótulo
        if (strcmp(TABELA[i].rotulo, rotulo) == 0) return TABELA[i].tecla;
    return TECLA_NENHUMA;
}

// Nas telas depois do final, a barra fica numa faixa escura. No menu, as teclas flutuam direto sobre a
// cena, sem a faixa, e o texto ganha uma sombra para ler por cima do chuvisco.
static void desenha_barra(const Estado *e, bool faixa, float k) {
    TeclaAcao itens[MAX_ITENS];
    int n = itens_do_contexto(e, itens);
    if (n == 0) return; // nada a ensinar agora: nem a faixa escura
    float s = ui_escala() * k, altura = 30 * s, y = GetScreenHeight() - altura - 12 * s;
    if (faixa)
        DrawRectangle(0, (int)(y - 10 * s), GetScreenWidth(), (int)(altura + 22 * s), (Color){4, 7, 5, 235});
    float x = 90 * s; // na margem do texto do menu e do HUD
    for (int i = 0; i < n; i++) { // padrão MAP: cada tecla com o que ela faz agora
        const char *nome = rotulo(e, &itens[i]);
        if (nome == NULL) continue;
        float x0 = x;
        x += tecla(e, nome, (Vector2){x, y}, altura, COR_FOSFORO) + 8 * s;
        const char *acao = dialogo_texto(itens[i].acao);
        if (!faixa) ui_texto(e, acao, (Vector2){x + 1.5f * s, y + altura * 0.22f + 1.5f * s}, 17 * s, BLACK);
        ui_texto(e, acao, (Vector2){x, y + altura * 0.22f}, 17 * s, RAYWHITE);
        x += ui_mede(e, acao, 17 * s).x;
        TeclaClique t = tecla_clicavel(itens[i].tecla);
        if (t != TECLA_NENHUMA) clique_area(CLIQUE_TECLA, t, (Rectangle){x0, y, x - x0, altura});
        x += 28 * s;
    }
}

// ================================================================== na partida: B Comandos
static bool jogando(const Estado *e) {
    return e->fase != FASE_MENU && e->fase != FASE_INTRO && e->fase != FASE_FIM_BOM &&
           e->fase != FASE_FIM_RUIM;
}

void teclas_atualiza(Estado *e, float dt) {
    (void)dt;
    if (!jogando(e))
        e->comandos_abertos = false;
    else if (e->in.comandos)
        e->comandos_abertos = !e->comandos_abertos;
}

// O brilho em volta de uma tecla (a dica da lanterna): camadas arredondadas cada vez maiores e mais fracas.
static void brilho(const Estado *e, const char *rotulo, Vector2 pos, float altura, float forca) {
    if (forca <= 0.0f) return;
    float s = ui_escala(), largura = fmaxf(altura, ui_mede(e, rotulo, altura * 0.55f).x + 16 * s);
    for (int i = 4; i >= 1; i--) { // padrão MAP: uma camada por anel, de fora para dentro
        float d = i * 2.2f * s;
        Rectangle r = {pos.x - d, pos.y - d, largura + 2 * d, altura + 2 * d};
        DrawRectangleRounded(r, 0.45f, 6,
                             Fade(DICA_LANTERNA_COR, forca * DICA_LANTERNA_GLOW * (5 - i) / 4.0f));
    }
}

// Uma linha: a tecla e o que ela faz, com sombra (fica por cima da cena). Devolve a largura usada.
// `destaque` (0..1) acende o brilho em volta e puxa a tecla e o texto para o amarelo (a dica da lanterna).
static float linha_comando(const Estado *e, const char *rotulo, const char *acao, Vector2 pos, float k,
                           float destaque) {
    float s = ui_escala() * k, altura = 30 * s, x = pos.x;
    brilho(e, rotulo, pos, altura, destaque);
    x += tecla(e, rotulo, pos, altura, ColorLerp(COR_FOSFORO, DICA_LANTERNA_COR, destaque)) + 8 * s;
    Vector2 t = {x, pos.y + altura * 0.22f};
    ui_texto(e, acao, (Vector2){t.x + 1.5f * s, t.y + 1.5f * s}, 17 * s, BLACK);
    ui_texto(e, acao, t, 17 * s, ColorLerp(Fade(RAYWHITE, 0.85f), DICA_LANTERNA_COR, destaque));
    x += ui_mede(e, acao, 17 * s).x;
    TeclaClique c = tecla_clicavel(rotulo);
    if (c != TECLA_NENHUMA) clique_area(CLIQUE_TECLA, c, (Rectangle){pos.x, pos.y, x - pos.x, altura});
    return x - pos.x;
}

// Aberta, a lista sobe do B, numa caixa escura translúcida, uma tecla por linha.
static void desenha_lista(const Estado *e, float y_base, float k) {
    TeclaAcao itens[MAX_ITENS];
    int n = itens_do_contexto(e, itens);
    if (n == 0) return; // nada vale agora (ele está indo embora do armário): só o B
    float s = ui_escala() * k, passo = 38 * s, margem = 10 * s, x = 24 * ui_escala();
    float topo = y_base - n * passo - margem, largura = 0.0f;
    for (int i = 0; i < n; i++) { // padrão FOLD: a linha mais larga dá a largura da caixa
        const char *acao = dialogo_texto(itens[i].acao);
        float l =
            ui_mede(e, acao, 17 * s).x + fmaxf(30 * s, ui_mede(e, itens[i].tecla, 16.5f * s).x + 16 * s);
        largura = fmaxf(largura, l + 8 * s + 22 * s); // a seta desenhada é mais larga que o rótulo
    }
    DrawRectangleRounded((Rectangle){x - margem, topo - margem, largura + 2 * margem, n * passo + margem},
                         0.08f, 4, (Color){4, 7, 5, 190});
    for (int i = 0; i < n; i++) // padrão MAP: cada tecla com o que ela faz agora
        linha_comando(e, itens[i].tecla, dialogo_texto(itens[i].acao), (Vector2){x, topo + i * passo}, k,
                      0.0f);
}

void teclas_desenha(const Estado *e) {
    if (!jogando(e) || e->pausado || susto_ativo(e)) return;
    float k = TECLAS_ESCALA_JOGO, s = ui_escala() * k, y = GetScreenHeight() - 30 * s - 18 * ui_escala();
    TextoId rotulo = e->comandos_abertos ? TXT_TECLA_FECHAR : TXT_TECLA_COMANDOS;
    float x = 24 * ui_escala();
    x += linha_comando(e, "B", dialogo_texto(rotulo), (Vector2){x, y}, k, 0.0f) + 20 * s;
    // A lanterna sempre à vista: é a tecla que mais se esquece. Até a 1ª vez que acende, pulsa em amarelo.
    float pulso = 0.5f - 0.5f * cosf(e->tempo_total * 2.0f * PI * DICA_LANTERNA_HZ);
    float destaque = e->lanterna.ja_acesa ? 0.0f : pulso;
    if (e->jog.modo == MODO_ANDANDO)
        linha_comando(e, "F", dialogo_texto(TXT_TECLA_LANTERNA), (Vector2){x, y}, k, destaque);
    if (e->comandos_abertos) desenha_lista(e, y - 8 * s, k);
}

// O menu e as telas depois do final cobrem a tela inteira: elas chamam a barra por cima de si. No menu, sem
// a faixa escura (as teclas flutuam sobre a cena).
void teclas_desenha_tela_cheia(const Estado *e) {
    if (e->fase == FASE_MENU) desenha_barra(e, false, 1.0f);
    bool fim = e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM;
    if (fim && e->fim.tela_final) desenha_barra(e, true, 1.0f);
}
