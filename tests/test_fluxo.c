// test_fluxo.c — simula partidas inteiras só com regras_proxima_fase e eventos.
// O --smoke do jogo usa o mesmo roteiro (regras_roteiro_partida).
#include "checagem.h"
#include "regras.h"
#include <string.h>

static Fase joga_roteiro(bool final_bom, Fase *fase_antes_do_menu) {
    EventoFase roteiro[16];
    int n = regras_roteiro_partida(final_bom, roteiro, 16);
    Fase fase = FASE_MENU;
    for (int i = 0; i < n; i++) {
        if (roteiro[i] == EV_VOLTAR_MENU) *fase_antes_do_menu = fase;
        fase = regras_proxima_fase(fase, roteiro[i]);
    }
    return fase;
}

static void testa_partidas_completas(void) {
    Fase final = FASE_MENU;
    CHECK(joga_roteiro(true, &final) == FASE_MENU);
    CHECK(final == FASE_FIM_BOM);
    CHECK(joga_roteiro(false, &final) == FASE_MENU);
    CHECK(final == FASE_FIM_RUIM);
}

// [REQ:PURA-regras_proxima_fase] testa a ordem das fases da partida
static void testa_ordem_das_fases(void) {
    Fase f = regras_proxima_fase(FASE_MENU, EV_INICIAR);
    CHECK(f == FASE_NOME);
    f = regras_proxima_fase(f, EV_NOME_CONFIRMADO);
    CHECK(f == FASE_INTRO);
    f = regras_proxima_fase(f, EV_INTRO_TERMINOU);
    CHECK(f == FASE_R1);
    f = regras_proxima_fase(f, EV_RODADA_CONCLUIDA);
    CHECK(f == FASE_S1);
    f = regras_proxima_fase(f, EV_SAIDA_CONCLUIDA);
    CHECK(f == FASE_R2);
    f = regras_proxima_fase(f, EV_RODADA_CONCLUIDA);
    CHECK(f == FASE_S2);
    f = regras_proxima_fase(f, EV_SAIDA_CONCLUIDA);
    CHECK(f == FASE_R3);
    f = regras_proxima_fase(f, EV_RODADA_CONCLUIDA);
    CHECK(f == FASE_RESPOSTA_FINAL);
}

// Transmitir uma contradição chega ao FIM_BOM; uma frase satisfazível, ao FIM_RUIM; inválida, a nada.
// [REQ:PURA-regras_avalia_transmissao] testa contradição, satisfazível e frase inválida
static void testa_transmissoes(void) {
    const Ficha P = {FICHA_ATOMO, ATOMO_P}, Q = {FICHA_ATOMO, ATOMO_Q};
    const Ficha NAO = {FICHA_NAO, 0}, E = {FICHA_E, 0}, OU = {FICHA_OU, 0}, SE = {FICHA_SE_ENTAO, 0};
    ErroLogica erro;

    Ficha bom[] = {P, E, NAO, P};
    EventoFase ev = regras_avalia_transmissao(bom, 4, &erro);
    CHECK(ev == EV_TRANSMITIU_CONTRADICAO);
    CHECK(regras_proxima_fase(FASE_RESPOSTA_FINAL, ev) == FASE_FIM_BOM);

    Ficha ruim[] = {P, SE, Q};
    ev = regras_avalia_transmissao(ruim, 3, &erro);
    CHECK(ev == EV_TRANSMITIU_SATISFAZIVEL);
    CHECK(regras_proxima_fase(FASE_RESPOSTA_FINAL, ev) == FASE_FIM_RUIM);

    Ficha tambem_ruim[] = {P, OU, NAO, P}; // tautologia também é satisfazível
    CHECK(regras_avalia_transmissao(tambem_ruim, 4, &erro) == EV_TRANSMITIU_SATISFAZIVEL);

    Ficha outra_contradicao[] = {NAO, Q, E, Q};
    CHECK(regras_avalia_transmissao(outra_contradicao, 4, &erro) == EV_TRANSMITIU_CONTRADICAO);

    Ficha invalida[] = {P, E};
    ev = regras_avalia_transmissao(invalida, 2, &erro);
    CHECK(ev == EV_NENHUM && !erro.ok); // "SINTAXE INVÁLIDA", sem penalidade
    CHECK(regras_proxima_fase(FASE_RESPOSTA_FINAL, ev) == FASE_RESPOSTA_FINAL);

    Ficha longa[] = {P, E, P, E, P, E, P, E, P};
    CHECK(regras_avalia_transmissao(longa, 9, &erro) == EV_NENHUM); // mais de 7 fichas
}

// 3 erros reiniciam a rodada; ser pego reinicia a saída; nada disso muda de fase.
static void testa_reinicios(void) {
    Rachadura rachaduras[3] = {{.dano = 1}, {.dano = 1}, {.dano = 1}};
    int vidas = regras_integridade_vidro(rachaduras, 3, VIDAS_VIDRO);
    CHECK(vidas == 0);
    EventoFase ev = vidas == 0 ? EV_VIDRO_ROMPEU : EV_NENHUM;
    CHECK(regras_proxima_fase(FASE_R2, ev) == FASE_R2);
    CHECK(regras_reinicia_fase(FASE_R2, ev));

    CHECK(regras_proxima_fase(FASE_S1, EV_PEGO) == FASE_S1);
    CHECK(regras_reinicia_fase(FASE_S1, EV_PEGO));
    CHECK(!regras_reinicia_fase(FASE_S1, EV_VIDRO_ROMPEU));
    CHECK(!regras_reinicia_fase(FASE_R1, EV_PEGO));
}

// Uma partida com tropeços no meio: o vidro rompe na R2 (3 erros) e o jogador é pego na S1, e ainda assim
// a partida chega ao FIM_BOM. Os eventos de erro saem das regras puras, como no jogo.
static Fase aplica(Fase f, EventoFase ev, int *recomecos) {
    if (regras_reinicia_fase(f, ev)) (*recomecos)++;
    return regras_proxima_fase(f, ev);
}

static void testa_partida_com_recomecos(void) {
    int recomecos = 0;
    Fase f = FASE_MENU;
    f = aplica(f, EV_INICIAR, &recomecos);
    f = aplica(f, EV_NOME_CONFIRMADO, &recomecos);
    f = aplica(f, EV_INTRO_TERMINOU, &recomecos);
    f = aplica(f, EV_RODADA_CONCLUIDA, &recomecos);
    CHECK(f == FASE_S1);
    f = aplica(f, EV_PEGO, &recomecos); // pego: a saída recomeça
    CHECK(f == FASE_S1 && recomecos == 1);
    f = aplica(f, EV_SAIDA_CONCLUIDA, &recomecos);
    CHECK(f == FASE_R2);
    Rachadura r[3] = {{.dano = 1}, {.dano = 1}, {.dano = 1}};
    for (int erros = 1; erros <= 3; erros++) {
        EventoFase ev = regras_evento_do_erro(regras_integridade_vidro(r, erros, VIDAS_VIDRO));
        CHECK(ev == (erros < 3 ? EV_NENHUM : EV_VIDRO_ROMPEU)); // só o 3º erro rompe o vidro
        f = aplica(f, ev, &recomecos);
    }
    CHECK(f == FASE_R2 && recomecos == 2);
    f = aplica(f, EV_RODADA_CONCLUIDA, &recomecos);
    f = aplica(f, EV_SAIDA_CONCLUIDA, &recomecos);
    f = aplica(f, EV_RODADA_CONCLUIDA, &recomecos);
    CHECK(f == FASE_RESPOSTA_FINAL);
    const Ficha frase[] = {{FICHA_ATOMO, ATOMO_P}, {FICHA_E, 0}, {FICHA_NAO, 0}, {FICHA_ATOMO, ATOMO_P}};
    ErroLogica erro;
    f = aplica(f, regras_avalia_transmissao(frase, 4, &erro), &recomecos);
    CHECK(f == FASE_FIM_BOM);
    CHECK(aplica(f, EV_VOLTAR_MENU, &recomecos) == FASE_MENU);
}

// Eventos fora de hora são ignorados (nenhum softlock nem pulo indevido).
static void testa_eventos_ignorados(void) {
    CHECK(regras_proxima_fase(FASE_MENU, EV_PEGO) == FASE_MENU);
    CHECK(regras_proxima_fase(FASE_NOME, EV_INTRO_TERMINOU) == FASE_NOME);
    CHECK(regras_proxima_fase(FASE_MENU, EV_NOME_CONFIRMADO) == FASE_MENU);
    CHECK(regras_proxima_fase(FASE_R1, EV_SAIDA_CONCLUIDA) == FASE_R1);
    CHECK(regras_proxima_fase(FASE_S2, EV_RODADA_CONCLUIDA) == FASE_S2);
    CHECK(regras_proxima_fase(FASE_FIM_BOM, EV_INICIAR) == FASE_FIM_BOM);
    CHECK(regras_proxima_fase((Fase)99, EV_INICIAR) == FASE_MENU);
}

static void testa_debug(void) {
    CHECK(regras_proxima_fase(FASE_R1, EV_PULAR_FASE) == FASE_S1);
    CHECK(regras_proxima_fase(FASE_FIM_RUIM, EV_PULAR_FASE) == FASE_MENU);
    CHECK(regras_proxima_fase(FASE_S1, EV_IR_RESPOSTA_FINAL) == FASE_RESPOSTA_FINAL);
    CHECK(regras_proxima_fase(FASE_MENU, EV_IR_RESPOSTA_FINAL) == FASE_MENU);
    CHECK(regras_proxima_fase(FASE_MENU, EV_PULAR_FASE) == FASE_NOME);
    CHECK(regras_proxima_fase(FASE_NOME, EV_PULAR_FASE) == FASE_INTRO);
    CHECK(regras_proxima_fase(FASE_NOME, EV_IR_RESPOSTA_FINAL) == FASE_NOME);
}

// ================================================================== nome de quem joga (só nesta partida)
// Sessão simulada: fase + buffer do nome, aplicados como em jogo.c (entra_fase → regras_nome_aplica).
#define PADRAO "VISITANTE"

typedef struct {
    Fase fase;
    char nome[NOME_CAPACIDADE];
} Sessao;

static void sessao_evento(Sessao *s, EventoFase ev) {
    Fase proxima = regras_proxima_fase(s->fase, ev);
    bool reinicia = regras_reinicia_fase(s->fase, ev);
    if (proxima == s->fase && !reinicia) return;
    regras_nome_aplica(s->nome, sizeof(s->nome), proxima, reinicia, PADRAO);
    s->fase = proxima;
}

static void sessao_digita(Sessao *s, const int *codigos, int n) {
    // padrão FOLD: cada tecla acumulada no nome, como na tela de nome
    for (int i = 0; i < n; i++)
        regras_nome_acrescenta(s->nome, sizeof(s->nome), codigos[i]);
}

// O buffer inteiro, não só a parte visível: nenhum byte do nome anterior sobra na memória.
static bool zerado(const char *buffer, size_t n) {
    int sujos = 0;
    // padrão FOLD
    for (size_t i = 0; i < n; i++)
        sujos += buffer[i] != '\0';
    return sujos == 0;
}

// Partida completa (final bom e ruim): o nome digitado vale da intro ao fim e some ao voltar ao menu.
static void testa_nome_na_partida(bool final_bom) {
    const int ana[] = {' ', ' ', 'A', 'n', 0xE1, ' ', ' ', ' ', 'L', 'u', ' '}; // "  Aná   Lu "
    EventoFase roteiro[16];
    int n = regras_roteiro_partida(final_bom, roteiro, 16);
    Sessao s = {FASE_MENU, ""};
    for (int i = 0; i < n; i++) {
        if (s.fase == FASE_NOME) sessao_digita(&s, ana, 11);
        sessao_evento(&s, roteiro[i]);
        if (s.fase != FASE_MENU && s.fase != FASE_NOME) CHECK(strcmp(s.nome, "An\xC3\xA1 Lu") == 0);
        if (s.fase == FASE_NOME) CHECK(zerado(s.nome, sizeof(s.nome))); // partida nova começa vazia
    }
    CHECK(s.fase == FASE_MENU);
    CHECK(zerado(s.nome, sizeof(s.nome)));
}

static void testa_nome_vazio_vira_padrao(void) {
    Sessao s = {FASE_MENU, ""};
    sessao_evento(&s, EV_INICIAR);
    const int espacos[] = {' ', ' ', ' '};
    sessao_digita(&s, espacos, 3);
    sessao_evento(&s, EV_NOME_CONFIRMADO);
    CHECK(s.fase == FASE_INTRO && strcmp(s.nome, PADRAO) == 0);
    // F2 na tela de nome e --fase S2 (debug) também nunca deixam o nome indefinido
    Sessao f2 = {FASE_MENU, ""};
    sessao_evento(&f2, EV_PULAR_FASE);
    sessao_evento(&f2, EV_PULAR_FASE);
    CHECK(f2.fase == FASE_INTRO && strcmp(f2.nome, PADRAO) == 0);
    Sessao direto = {FASE_MENU, ""};
    regras_nome_aplica(direto.nome, sizeof(direto.nome), FASE_S2, false, PADRAO);
    CHECK(strcmp(direto.nome, PADRAO) == 0);
}

// Recomeços (vidro rompido, pego pela criatura, F5) mantêm o nome: a mesma pessoa continua jogando.
static void testa_nome_sobrevive_recomecos(void) {
    const int bia[] = {'B', 'i', 'a'};
    Sessao s = {FASE_MENU, ""};
    sessao_evento(&s, EV_INICIAR);
    sessao_digita(&s, bia, 3);
    sessao_evento(&s, EV_NOME_CONFIRMADO);
    sessao_evento(&s, EV_INTRO_TERMINOU);
    sessao_evento(&s, EV_RODADA_CONCLUIDA);
    CHECK(s.fase == FASE_S1);
    sessao_evento(&s, EV_PEGO); // capturado: a saída recomeça
    CHECK(s.fase == FASE_S1 && strcmp(s.nome, "Bia") == 0);
    sessao_evento(&s, EV_SAIDA_CONCLUIDA);
    sessao_evento(&s, EV_VIDRO_ROMPEU); // 3º erro na R2: a rodada recomeça
    CHECK(s.fase == FASE_R2 && strcmp(s.nome, "Bia") == 0);
    regras_nome_aplica(s.nome, sizeof(s.nome), s.fase, true, PADRAO); // F5
    CHECK(strcmp(s.nome, "Bia") == 0);
    Sessao tela = {FASE_NOME, "Bi"}; // F5 na própria tela de nome não apaga o que já foi digitado
    regras_nome_aplica(tela.nome, sizeof(tela.nome), FASE_NOME, true, PADRAO);
    CHECK(strcmp(tela.nome, "Bi") == 0);
}

// Voltar ao menu por qualquer caminho zera o buffer: pausa, Esc na tela de nome e inatividade (feira).
static void testa_nome_zerado_ao_voltar(void) {
    const int ze[] = {'Z', 0xE9};
    Sessao s = {FASE_MENU, ""};
    sessao_evento(&s, EV_INICIAR);
    sessao_digita(&s, ze, 2);
    sessao_evento(&s, EV_VOLTAR_MENU); // Esc na tela de nome
    CHECK(s.fase == FASE_MENU && zerado(s.nome, sizeof(s.nome)));

    Sessao pausa = {FASE_MENU, ""};
    sessao_evento(&pausa, EV_INICIAR);
    sessao_digita(&pausa, ze, 2);
    sessao_evento(&pausa, EV_NOME_CONFIRMADO);
    sessao_evento(&pausa, EV_INTRO_TERMINOU);
    sessao_evento(&pausa, EV_VOLTAR_MENU); // pausa → VOLTAR AO MENU, no meio da R1
    CHECK(pausa.fase == FASE_MENU && zerado(pausa.nome, sizeof(pausa.nome)));

    // Modo feira: a pessoa some no meio da S2 ou na própria tela de nome.
    CHECK(regras_evento_inatividade(true, FASE_S2, FEIRA_INATIVIDADE_S - 1.0f) == EV_NENHUM);
    CHECK(regras_evento_inatividade(false, FASE_S2, FEIRA_INATIVIDADE_S + 1.0f) == EV_NENHUM);
    CHECK(regras_evento_inatividade(true, FASE_MENU, FEIRA_INATIVIDADE_S + 1.0f) == EV_NENHUM);
    CHECK(regras_evento_inatividade(true, FASE_FIM_BOM, FEIRA_INATIVIDADE_S + 1.0f) == EV_NENHUM);
    Sessao feira = {FASE_MENU, ""};
    sessao_evento(&feira, EV_INICIAR);
    sessao_digita(&feira, ze, 2);
    sessao_evento(&feira, regras_evento_inatividade(true, feira.fase, FEIRA_INATIVIDADE_S + 1.0f));
    CHECK(feira.fase == FASE_MENU && zerado(feira.nome, sizeof(feira.nome)));
    sessao_evento(&feira, EV_INICIAR);
    sessao_digita(&feira, ze, 2);
    sessao_evento(&feira, EV_NOME_CONFIRMADO);
    sessao_evento(&feira, EV_PULAR_FASE); // INTRO → R1
    sessao_evento(&feira, EV_PULAR_FASE); // → S1
    sessao_evento(&feira, regras_evento_inatividade(true, feira.fase, FEIRA_INATIVIDADE_S + 1.0f));
    CHECK(feira.fase == FASE_MENU && zerado(feira.nome, sizeof(feira.nome)));
}

int main(void) {
    testa_partidas_completas();
    testa_ordem_das_fases();
    testa_transmissoes();
    testa_reinicios();
    testa_partida_com_recomecos();
    testa_eventos_ignorados();
    testa_debug();
    testa_nome_na_partida(true);
    testa_nome_na_partida(false);
    testa_nome_vazio_vira_padrao();
    testa_nome_sobrevive_recomecos();
    testa_nome_zerado_ao_voltar();
    return resultado_final("test_fluxo");
}
