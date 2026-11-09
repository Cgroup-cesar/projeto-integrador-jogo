// regras.c — regras do jogo como funções puras. Não inclui raylib.h.
#include "regras.h"

const char *regras_nome_fase(Fase fase) {
    static const char *const nomes[FASE_TOTAL] = {
        "MENU", "INTRO", "R1", "S1", "R2", "S2", "R3", "RESPOSTA_FINAL", "FIM_RUIM", "FIM_BOM",
    };
    if (fase < 0 || fase >= FASE_TOTAL) return "?";
    return nomes[fase];
}

const char *regras_nome_evento(EventoFase evento) {
    static const char *const nomes[EV_TOTAL] = {
        "NENHUM",          "INICIAR", "INTRO_TERMINOU",           "RODADA_CONCLUIDA",
        "VIDRO_ROMPEU",    "SAIDA_CONCLUIDA", "PEGO",             "TRANSMITIU_CONTRADICAO",
        "TRANSMITIU_SATISFAZIVEL", "VOLTAR_MENU", "PULAR_FASE",   "IR_RESPOSTA_FINAL",
    };
    if (evento < 0 || evento >= EV_TOTAL) return "?";
    return nomes[evento];
}
