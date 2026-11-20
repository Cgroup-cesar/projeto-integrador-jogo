// tela_nome.c — entrada do nome de quem joga (FASE_NOME, entre o menu e a intro).
// O nome mora só em Estado.nome_jogador (buffer fixo): nada vai para o disco nem para o log.
// Quem decide o que entra, como apagar e quando zerar são as regras puras (regras_nome_*).
#include "tela_nome.h"
#include "audio_sintese.h"
#include "dialogo.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>

// [REQ:RF-NOME] tela de nome: digita (GetCharPressed), Backspace apaga um caractere inteiro, ENTER segue
// para a intro (vazio vira o padrão ao entrar na intro), Esc volta ao menu (e o nome é apagado)
void tela_nome_atualiza(Estado *e, float dt) {
    (void)dt; // só teclas; o cursor pisca pelo tempo da fase
    if (e->fase != FASE_NOME) return;
    if (e->modo_smoke) { // automático: não espera teclado, segue com o nome padrão
        e->evento = EV_NOME_CONFIRMADO;
        return;
    }
    if (e->in.voltar) {
        e->evento = EV_VOLTAR_MENU;
        return;
    }
    bool entrou = false;
    // padrão FOLD: cada caractere digitado no quadro é acumulado no nome (os inválidos ou excedentes ficam de
    // fora)
    for (int i = 0; i < e->in.n_digitados; i++)
        entrou |= regras_nome_acrescenta(e->nome_jogador, sizeof(e->nome_jogador), e->in.digitados[i]);
    if (entrou) audio_toca(e, SOM_MAQUINA_ESCREVER, 0.5f, 1.0f);
    if (e->in.apagar) regras_nome_apaga_ultimo(e->nome_jogador);
    if (e->in.confirmar) e->evento = EV_NOME_CONFIRMADO;
}

// Campo de texto: o nome, o cursor piscante logo depois dele e o contador de letras.
static void desenha_campo(const Estado *e, float y) {
    float s = ui_escala();
    float tamanho = 40 * s;
    Rectangle caixa = {GetScreenWidth() * 0.5f - 300 * s, y, 600 * s, 64 * s};
    ui_painel(caixa, COR_FOSFORO);
    Vector2 m = ui_mede(e, e->nome_jogador, tamanho);
    float x = caixa.x + 24 * s, ty = caixa.y + (caixa.height - tamanho) * 0.5f;
    ui_texto(e, e->nome_jogador, (Vector2){x, ty}, tamanho, COR_FOSFORO);
    if (fmodf(e->tempo_fase, 2.0f * NOME_CURSOR_PISCA_S) < NOME_CURSOR_PISCA_S)
        DrawRectangle((int)(x + m.x + 4 * s), (int)(ty + 2 * s), (int)(14 * s), (int)(tamanho - 4 * s),
                      COR_FOSFORO);
    char contador[16];
    int letras = regras_nome_conta_letras(e->nome_jogador);
    snprintf(contador, sizeof(contador), dialogo_texto(TXT_NOME_CONTADOR), letras, NOME_MAX_CHARS);
    Vector2 mc = ui_mede(e, contador, 18 * s);
    ui_texto(e, contador, (Vector2){caixa.x + caixa.width - mc.x, caixa.y + caixa.height + 8 * s}, 18 * s,
             letras >= NOME_MAX_CHARS ? COR_AMBAR : Fade(COR_FOSFORO, 0.6f));
}

// Mesmo visual do menu: fundo escurecido sobre a sala, título em fósforo, dicas embaixo.
void tela_nome_desenha(const Estado *e) {
    if (e->fase != FASE_NOME) return;
    float s = ui_escala();
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangleGradientV(0, 0, w, h, Fade(BLACK, 0.45f), Fade(BLACK, 0.9f));
    ui_texto_centro(e, TITULO_JOGO, 120 * s, 52 * s, Fade(COR_FOSFORO, 0.5f));
    ui_texto_centro(e, dialogo_texto(TXT_NOME_TITULO), 230 * s, 34 * s, COR_FOSFORO);
    ui_texto_centro(e, dialogo_texto(TXT_NOME_PERGUNTA), 300 * s, 24 * s, COR_AMBAR);
    desenha_campo(e, 345 * s);
    ui_texto_centro(e, dialogo_texto(TXT_NOME_DICA), 560 * s, 18 * s, Fade(COR_VIDRO, 0.8f));
    ui_texto_centro(e, dialogo_texto(TXT_NOME_AVISO), 650 * s, 16 * s, Fade(COR_FOSFORO, 0.55f));
}
