// test_fluxo.c — simula partidas inteiras só com regras_proxima_fase e eventos.
// O --smoke do jogo usa o mesmo roteiro (regras_roteiro_partida).
#include "checagem.h"
#include "regras.h"

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

static void testa_ordem_das_fases(void) {
    Fase f = regras_proxima_fase(FASE_MENU, EV_INICIAR);
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
}

int main(void) {
    testa_partidas_completas();
    testa_ordem_das_fases();
    testa_transmissoes();
    testa_reinicios();
    testa_partida_com_recomecos();
    testa_eventos_ignorados();
    testa_debug();
    return resultado_final("test_fluxo");
}
