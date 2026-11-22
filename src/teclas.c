// teclas.c — a barra de teclas do rodapé e o minimapa do teclado. Só desenha, a partir do estado: escolhe as
// teclas pelo contexto (terminal, painel, andando, armário, bilhete) e não muda nada.
#include "teclas.h"
#include "rodadas.h"
#include "susto.h"
#include "ui.h"
#include "dialogo.h"
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
        t[n++] = (TeclaAcao){"SETAS_V", TXT_TECLA_ESCOLHER};
        t[n++] = (TeclaAcao){"ENTER", TXT_TECLA_LIGAR};
        t[n++] = (TeclaAcao){"1 2 3", TXT_TECLA_CHAVES}; // atalho: liga e desliga direto
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
        t[0] = (TeclaAcao){"E", TXT_TECLA_SAIR_ARMARIO};
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

// ================================================================== minimapa do teclado
// A parte do teclado que o jogo usa, em "unidades de tecla": coluna, linha, largura e o nome que acende.
typedef struct {
    float x, y, w;
    const char *nome;
    const char *acende; // o rótulo da barra que acende esta tecla
} TeclaMapa;

static const TeclaMapa MAPA[] = {
    {0, 0, 1, "Esc", "ESC"},
    {1.2f, 0, 1, "1", "1"},
    {2.2f, 0, 1, "2", "2"},
    {3.2f, 0, 1, "3", "3"},
    {4.2f, 0, 1, "4", ""},
    {0, 1, 1.5f, "", ""},
    {1.5f, 1, 1, "Q", ""},
    {2.5f, 1, 1, "W", "WASD"},
    {3.5f, 1, 1, "E", "E"},
    {4.5f, 1, 1, "R", ""},
    {1.8f, 2, 1, "A", "WASD"},
    {2.8f, 2, 1, "S", "WASD"},
    {3.8f, 2, 1, "D", "WASD"},
    {4.8f, 2, 1, "F", "F"},
    {5.8f, 2, 1, "G", ""},
    {6.8f, 2, 1, "H", "H"},
    {0, 3, 2.3f, "Shift", "SHIFT"},
    {2.3f, 3, 1, "Z", ""},
    {8.2f, 1, 1.6f, "Enter", "ENTER"},
    {9.2f, 2, 1, "", "SETAS_V"},
    {8.2f, 3, 1, "", "SETAS_H"},
    {9.2f, 3, 1, "", "SETAS_V"},
    {10.2f, 3, 1, "", "SETAS_H"},
};

static bool acesa(const TeclaMapa *k, const TeclaAcao *itens, int n) {
    for (int i = 0; i < n; i++) { // padrão FOLD: alguma tecla da barra acende esta?
        if (k->acende[0] == '\0') continue;
        bool setas = strcmp(itens[i].tecla, "SETAS") == 0 && strncmp(k->acende, "SETAS", 5) == 0;
        bool numeros =
            strcmp(itens[i].tecla, "1 2 3") == 0 && strlen(k->acende) == 1 && strchr("123", k->acende[0]);
        if (setas || numeros || strcmp(itens[i].tecla, k->acende) == 0) return true;
    }
    return false;
}

static void desenha_minimapa(const Estado *e, const TeclaAcao *itens, int n, Vector2 pos) {
    float s = ui_escala(), u = 22 * s; // tamanho de uma tecla no minimapa
    int total = (int)(sizeof(MAPA) / sizeof(MAPA[0]));
    for (int i = 0; i < total; i++) { // padrão MAP: cada tecla do mapa, acesa ou apagada
        const TeclaMapa *k = &MAPA[i];
        Rectangle r = {pos.x + k->x * u, pos.y + k->y * u, k->w * u - 2 * s, u - 2 * s};
        bool liga = acesa(k, itens, n);
        DrawRectangleRounded(r, 0.25f, 3, liga ? COR_FOSFORO : (Color){20, 28, 22, 255});
        if (k->nome[0] != '\0') {
            float t = 11 * s;
            Vector2 m = ui_mede(e, k->nome, t);
            ui_texto(e, k->nome, (Vector2){r.x + (r.width - m.x) * 0.5f, r.y + (r.height - m.y) * 0.5f}, t,
                     liga ? (Color){8, 14, 10, 255} : Fade(COR_FOSFORO, 0.35f));
        }
    }
}

// ================================================================== a barra
static void desenha_barra(const Estado *e, bool minimapa) {
    TeclaAcao itens[MAX_ITENS];
    int n = itens_do_contexto(e, itens);
    float s = ui_escala(), altura = 30 * s, y = GetScreenHeight() - altura - 12 * s;
    DrawRectangle(0, (int)(y - 10 * s), GetScreenWidth(), (int)(altura + 22 * s), (Color){4, 7, 5, 235});
    if (minimapa) desenha_minimapa(e, itens, n, (Vector2){14 * s, y - 4 * 22 * s - 18 * s});
    float x = minimapa ? 290 * s : 90 * s; // sem o minimapa (menu), a barra começa na margem do texto
    for (int i = 0; i < n; i++) {          // padrão MAP: cada tecla com o que ela faz agora
        x += tecla(e, itens[i].tecla, (Vector2){x, y}, altura) + 8 * s;
        const char *acao = dialogo_texto(itens[i].acao);
        ui_texto(e, acao, (Vector2){x, y + altura * 0.22f}, 17 * s, RAYWHITE);
        x += ui_mede(e, acao, 17 * s).x + 28 * s;
    }
}

void teclas_desenha(const Estado *e) {
    bool jogando =
        e->fase != FASE_MENU && e->fase != FASE_INTRO && e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM;
    if (jogando && !e->pausado && !susto_ativo(e)) desenha_barra(e, true);
}

// O menu e as telas depois do final cobrem a tela inteira: elas chamam a barra por cima de si. No menu, sem
// o minimapa (a tela de início fica limpa).
void teclas_desenha_tela_cheia(const Estado *e) {
    if (e->fase == FASE_MENU) desenha_barra(e, false);
    if ((e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM) && e->fim.tela_final) desenha_barra(e, true);
}
