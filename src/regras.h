// regras.h — regras do jogo como funções puras (sem raylib.h, sem globais, sem ler teclado).
// Dada a entrada, devolvem a saída. Por isso dá para testar tudo em tests/ sem abrir janela.
#ifndef REGRAS_H
#define REGRAS_H

#include <stdbool.h>

// ------------------------------------------------------------------ fases da partida
// MENU → INTRO → R1 → S1 → R2 → S2 → R3 → RESPOSTA_FINAL → FIM_RUIM | FIM_BOM → MENU
typedef enum {
    FASE_MENU,
    FASE_INTRO,
    FASE_R1,
    FASE_S1,
    FASE_R2,
    FASE_S2,
    FASE_R3,
    FASE_RESPOSTA_FINAL,
    FASE_FIM_RUIM,
    FASE_FIM_BOM,
    FASE_TOTAL
} Fase;

// Eventos que podem mudar a fase. Quem gera o evento é o módulo de cada fase; quem decide é regras_proxima_fase.
typedef enum {
    EV_NENHUM,
    EV_INICIAR,                  // Enter no menu
    EV_INTRO_TERMINOU,           // fim do texto datilografado e das falas do interfone
    EV_RODADA_CONCLUIDA,         // decifrou e traduziu
    EV_VIDRO_ROMPEU,             // 3º erro: a rodada recomeça
    EV_SAIDA_CONCLUIDA,          // painel resolvido e de volta à sala
    EV_PEGO,                     // a criatura pegou o jogador: a saída recomeça
    EV_TRANSMITIU_CONTRADICAO,   // frase que nunca pode ser verdadeira → final bom
    EV_TRANSMITIU_SATISFAZIVEL,  // qualquer outra frase válida → final ruim
    EV_VOLTAR_MENU,              // Enter na tela de fim ou 20 s
    EV_PULAR_FASE,               // debug F2
    EV_IR_RESPOSTA_FINAL,        // debug F9
    EV_TOTAL
} EventoFase;

const char *regras_nome_fase(Fase fase);
const char *regras_nome_evento(EventoFase evento);

#endif
