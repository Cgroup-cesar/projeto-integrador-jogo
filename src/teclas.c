// teclas.c — a barra de teclas do rodapé. Só desenha, a partir do estado: escolhe as teclas pelo contexto
// (terminal, painel, andando, armário, bilhete) e não muda nada. Andando, ela aparece só no começo de cada
// fase (TECLAS_ANDAR_S): depois disso, quem joga já sabe, e a dica do que está na mira mostra o E.
#include "teclas.h"
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
    if (e->tempo_fase > TECLAS_ANDAR_S) return 0; // já andou o bastante para saber
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

// Desenha a tecla como uma tecla de verdade (caixinha com borda) e devolve a largura usada.
static float tecla(const Estado *e, const char *rotulo, Vector2 pos, float altura) {
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
    DrawRectangleRoundedLinesEx(r, 0.25f, 4, 1.5f * s, COR_FOSFORO);
    Vector2 c = {r.x + r.width * 0.5f, r.y + r.height * 0.5f};
    float sr = altura * 0.17f;
    if (strcmp(rotulo, "SETAS_V") == 0) {
        seta((Vector2){c.x - sr * 1.6f, c.y}, sr, 0, COR_FOSFORO);
        seta((Vector2){c.x + sr * 1.6f, c.y}, sr, 1, COR_FOSFORO);
    } else if (strcmp(rotulo, "SETAS_H") == 0) {
        seta((Vector2){c.x - sr * 1.6f, c.y}, sr, 2, COR_FOSFORO);
        seta((Vector2){c.x + sr * 1.6f, c.y}, sr, 3, COR_FOSFORO);
    } else if (setas == 4) {
        for (int d = 0; d < 4; d++) // padrão MAP: uma seta por direção
            seta((Vector2){c.x + (d - 1.5f) * sr * 2.6f, c.y}, sr * 0.9f, d, COR_FOSFORO);
    } else {
        Vector2 m = ui_mede(e, rotulo, tamanho);
        ui_texto(e, rotulo, (Vector2){c.x - m.x * 0.5f, c.y - m.y * 0.5f}, tamanho, COR_FOSFORO);
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
    } TABELA[] = {{"ESC", TECLA_ESC}, {"ENTER", TECLA_ENTER}, {"H", TECLA_H},
                  {"1", TECLA_1},     {"2", TECLA_2},         {"E", TECLA_E},
                  {"F", TECLA_F},     {"ESPAÇO", TECLA_ESPACO}};
    for (size_t i = 0; i < sizeof(TABELA) / sizeof(TABELA[0]); i++) // padrão FOLD: procura o rótulo
        if (strcmp(TABELA[i].rotulo, rotulo) == 0) return TABELA[i].tecla;
    return TECLA_NENHUMA;
}

// Na partida, a barra fica numa faixa escura e um pouco menor (TECLAS_ESCALA_JOGO). No menu, as teclas
// flutuam direto sobre a cena, sem a faixa, e o texto ganha uma sombra para ler por cima do chuvisco.
static void desenha_barra(const Estado *e, bool faixa, float k) {
    TeclaAcao itens[MAX_ITENS];
    int n = itens_do_contexto(e, itens);
    if (n == 0) return; // nada a ensinar agora: nem a faixa escura
    float s = ui_escala() * k, altura = 30 * s, y = GetScreenHeight() - altura - 12 * s;
    if (faixa)
        DrawRectangle(0, (int)(y - 10 * s), GetScreenWidth(), (int)(altura + 22 * s), (Color){4, 7, 5, 235});
    float x = 90 * s; // na margem do texto do menu e do HUD
    for (int i = 0; i < n; i++) { // padrão MAP: cada tecla com o que ela faz agora
        float x0 = x;
        x += tecla(e, itens[i].tecla, (Vector2){x, y}, altura) + 8 * s;
        const char *acao = dialogo_texto(itens[i].acao);
        if (!faixa) ui_texto(e, acao, (Vector2){x + 1.5f * s, y + altura * 0.22f + 1.5f * s}, 17 * s, BLACK);
        ui_texto(e, acao, (Vector2){x, y + altura * 0.22f}, 17 * s, RAYWHITE);
        x += ui_mede(e, acao, 17 * s).x;
        TeclaClique t = tecla_clicavel(itens[i].tecla);
        if (t != TECLA_NENHUMA) clique_area(CLIQUE_TECLA, t, (Rectangle){x0, y, x - x0, altura});
        x += 28 * s;
    }
}

void teclas_desenha(const Estado *e) {
    bool jogando =
        e->fase != FASE_MENU && e->fase != FASE_INTRO && e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM;
    if (jogando && !e->pausado && !susto_ativo(e)) desenha_barra(e, true, TECLAS_ESCALA_JOGO);
}

// O menu e as telas depois do final cobrem a tela inteira: elas chamam a barra por cima de si. No menu, sem
// a faixa escura (as teclas flutuam sobre a cena).
void teclas_desenha_tela_cheia(const Estado *e) {
    if (e->fase == FASE_MENU) desenha_barra(e, false, 1.0f);
    bool fim = e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM;
    if (fim && e->fim.tela_final) desenha_barra(e, true, 1.0f);
}
