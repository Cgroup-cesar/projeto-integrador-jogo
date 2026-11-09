// regras.h — regras do jogo como funções puras (sem raylib.h, sem globais, sem ler teclado).
// Dada a entrada, devolvem a saída. Por isso dá para testar tudo em tests/ sem abrir janela.
#ifndef REGRAS_H
#define REGRAS_H

#include <stdbool.h>
#include "config.h"
#include "raymath.h"
#include "logica.h"
#include "colecoes.h"

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

// Eventos que podem mudar a fase. Quem gera o evento é o módulo de cada fase; quem decide é
// regras_proxima_fase.
typedef enum {
    EV_NENHUM,
    EV_INICIAR,                 // Enter no menu
    EV_INTRO_TERMINOU,          // fim do texto datilografado e das falas do interfone
    EV_RODADA_CONCLUIDA,        // decifrou e traduziu
    EV_VIDRO_ROMPEU,            // 3º erro: a rodada recomeça
    EV_SAIDA_CONCLUIDA,         // painel resolvido e de volta à sala
    EV_PEGO,                    // a criatura pegou o jogador: a saída recomeça
    EV_TRANSMITIU_CONTRADICAO,  // frase que nunca pode ser verdadeira → final bom
    EV_TRANSMITIU_SATISFAZIVEL, // qualquer outra frase válida → final ruim
    EV_VOLTAR_MENU,             // Enter na tela de fim ou 20 s
    EV_PULAR_FASE,              // debug F2
    EV_IR_RESPOSTA_FINAL,       // debug F9
    EV_TOTAL
} EventoFase;

const char *regras_nome_fase(Fase fase);
const char *regras_nome_evento(EventoFase evento);
bool regras_fase_eh_rodada(Fase fase);
bool regras_fase_eh_saida(Fase fase);
int regras_numero_rodada(Fase fase); // R1 → 1 ... (0 se não for rodada)

// A máquina de fases inteira numa função pura.
Fase regras_proxima_fase(Fase fase, EventoFase evento);
// true quando o evento recomeça a fase atual (vidro rompeu numa rodada, pego numa saída).
bool regras_reinicia_fase(Fase fase, EventoFase evento);
// A sequência de eventos de uma partida inteira; o test_fluxo e o --smoke usam a mesma.
int regras_roteiro_partida(bool final_bom, EventoFase *saida, int maximo);

// ------------------------------------------------------------------ resposta final
#define ATOMO_P 0 // [VOCÊ FALAR]
#define ATOMO_Q 1 // [PORTA ABRIR]
// Frase inválida → EV_NENHUM (e `erro` explica); contradição → final bom; satisfazível → final ruim.
EventoFase regras_avalia_transmissao(const Ficha *fichas, int n, ErroLogica *erro);

// ------------------------------------------------------------------ movimento e o Precursor
typedef enum { CRIATURA_PATRULHA, CRIATURA_INVESTIGA, CRIATURA_PERSEGUE, CRIATURA_PROCURA } EstadoCriatura;

typedef struct {
    bool ve_jogador;
    bool ouviu_ruido;
    bool chegou_ao_destino;
    float tempo_no_estado;
} PercepcaoCriatura;

Vector3 regras_frente(float yaw);
Vector3 regras_nova_posicao(Vector3 pos, float yaw, Vector2 mover, float velocidade, float dt);
bool regras_criatura_ve_jogador(Vector3 olho, Vector3 frente, Vector3 alvo, bool caminho_livre,
                                bool escondido);
float regras_ruido_percebido(Ruido ruido, Vector3 ouvinte);
EstadoCriatura regras_proximo_estado_criatura(EstadoCriatura atual, PercepcaoCriatura p);
const char *regras_nome_estado_criatura(EstadoCriatura estado);

// ------------------------------------------------------------------ placar, vidas e interativos
int regras_placar(const EventoPlacar *eventos, int n);
int regras_placar_da_fase(const EventoPlacar *eventos, int n, int fase);
int regras_bonus_tempo(float tempo_total);
int regras_integridade_vidro(const Rachadura *rachaduras, int n, int vidas_max);
// Depois de um erro: sem vidas, o vidro rompe (EV_VIDRO_ROMPEU); com vidas, nada muda de fase.
EventoFase regras_evento_do_erro(int vidas_restantes);
bool regras_interativo_ativo(TipoInterativo tipo, int fase);

// ------------------------------------------------------------------ circuitos dos painéis
typedef enum { PORTA_E, PORTA_OU, PORTA_XOU, PORTA_NE, PORTA_NAO, PORTA_TOTAL } TipoPorta;

#define MAX_PORTAS   4
#define REF_PORTA(i) (16 + (i)) // entradas são 0, 1, 2...; saídas de portas são REF_PORTA(i)

typedef struct {
    TipoPorta tipo;
    int a;
    int b; // ignorado na porta NÃO
} Porta;

typedef struct {
    Porta portas[MAX_PORTAS];
    int n_portas;
    int n_entradas; // a saída do circuito é a última porta
} Circuito;

void regras_circuito_painel1(Circuito *c);                             // (A ∧ ¬B) ∧ C
void regras_circuito_painel2(Circuito *c, TipoPorta p1, TipoPorta p2); // (A [p1] B) [p2] (¬A)
bool regras_avaliar_circuito(const Circuito *c, const bool *entradas, bool *leds);
bool regras_circuito_confere_tabela(const Circuito *c, const bool *alvo);

#endif
