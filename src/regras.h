// regras.h — regras do jogo como funções puras (sem raylib.h, sem globais, sem ler teclado).
// Dada a entrada, devolvem a saída. Por isso dá para testar tudo em tests/ sem abrir janela.
#ifndef REGRAS_H
#define REGRAS_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"
#include "raymath.h"
#include "logica.h"
#include "colecoes.h"

// ------------------------------------------------------------------ fases da partida
// MENU → NOME → INTRO → R1 → S1 → R2 → S2 → R3 → RESPOSTA_FINAL → FIM_RUIM | FIM_BOM → MENU
typedef enum {
    FASE_MENU,
    FASE_NOME, // entrada do nome do(a) jogador(a): só em memória, só nesta partida
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
    EV_NOME_CONFIRMADO,         // Enter na tela de nome
    EV_INTRO_TERMINOU,         // fim do texto datilografado e das falas do interfone
    EV_RODADA_CONCLUIDA,        // decifrou e traduziu
    EV_VIDRO_ROMPEU,            // 3º erro: a rodada recomeça
    EV_SAIDA_CONCLUIDA,         // painel resolvido e de volta à sala
    EV_PEGO,                    // a criatura pegou o jogador: a saída recomeça
    EV_TRANSMITIU_CONTRADICAO,  // frase que nunca pode ser verdadeira → final bom
    EV_TRANSMITIU_SATISFAZIVEL, // qualquer outra frase válida → final ruim
    EV_VOLTAR_MENU,             // Enter na tela de fim ou 20 s; Esc no nome; pausa; inatividade (feira)
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

// Modo feira: depois de FEIRA_INATIVIDADE_S sem nenhuma tecla numa fase de jogo (inclusive a tela de
// nome), volta ao menu (EV_VOLTAR_MENU). Fora do modo feira, no menu e no fim (que já volta sozinho): nada.
EventoFase regras_evento_inatividade(bool modo_feira, Fase fase, float tempo_inativo);

// ------------------------------------------------------------------ nome do(a) jogador(a)
// O nome vive só em memória (buffer fixo em Estado) e só durante a partida. Nada vai para o disco.
// Os buffers são UTF-8 com `cap` bytes (NOME_CAPACIDADE); o limite visível é NOME_MAX_CHARS letras.

// Letras (inclusive as acentuadas do Latin-1), números, espaço, hífen e apóstrofo.
bool regras_nome_caractere_valido(int codepoint);
int regras_nome_conta_letras(const char *nome); // codepoints, não bytes
// Acrescenta um caractere digitado ao fim do nome, se for válido e couber; true se entrou.
bool regras_nome_acrescenta(char *nome, size_t cap, int codepoint);
// Apaga o último caractere inteiro (todos os bytes do codepoint).
void regras_nome_apaga_ultimo(char *nome);
// Descarta o que não é válido, tira os espaços das pontas, junta espaços repetidos e corta em
// NOME_MAX_CHARS letras (nunca no meio de um codepoint). Devolve o número de letras de `saida`.
int regras_nome_normaliza(const char *entrada, char *saida, size_t cap);
// Normaliza e, se o resultado ficar vazio, usa `padrao`. `entrada` e `saida` podem ser o mesmo buffer.
void regras_nome_define(const char *entrada, char *saida, size_t cap, const char *padrao);

// O que fazer com o nome ao entrar numa fase.
typedef enum {
    NOME_MANTEM,  // recomeço de fase (vidro rompido, pego, F5): a mesma pessoa continua
    NOME_APAGA,   // menu (de qualquer origem) e partida nova (tela de nome): zera o buffer
    NOME_GARANTE, // fases de jogo: se ainda estiver vazio (F2, --fase), vira o padrão
} AcaoNome;
AcaoNome regras_nome_ao_entrar(Fase fase, bool reinicio);
// Aplica a ação acima ao buffer (apagar zera os `cap` bytes inteiros). jogo.c chama ao entrar em cada fase.
void regras_nome_aplica(char *nome, size_t cap, Fase fase, bool reinicio, const char *padrao);

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

// Postura do corpo a cada quadro: substitui a animação que o modelo .glb não tem.
// `velocidade` em m/s (já suavizada), `passada` = metros andados (dá o ritmo dos passos), `t` em s.
typedef struct {
    float inclina_graus; // para a frente (corcunda); cresce com a velocidade
    float balanco_graus; // de um lado para o outro a cada passo; zero parado
    float sobe;          // metros: sobe e desce duas vezes por passada; zero parado
    float respira;       // escala horizontal do corpo (~1); só parado
} PosturaCriatura;

PosturaCriatura regras_postura_criatura(float velocidade, float passada, float t);

// Susto: `t` = segundos desde que o rosto apareceu. O rosto pula de JUMPSCARE_LONGE_M até JUMPSCARE_PERTO_M
// em JUMPSCARE_PULO_S e treme (posição nova JUMPSCARE_TREMOR_HZ vezes por segundo). Determinístico.
typedef struct {
    float distancia;          // metros da câmera ao rosto
    float tremor_x, tremor_y; // metros, de −JUMPSCARE_TREMOR_M a +JUMPSCARE_TREMOR_M
    float giro_graus;         // em torno do eixo da câmera
    float clarao;             // 1 quando ele chega, apagando em JUMPSCARE_CLARAO_S
    float boca;               // 0 fechando, 1 escancarada: no pulo, aberta; depois abre e fecha
} QuadroSusto;

QuadroSusto regras_quadro_susto(float t);

// Lâmpada com defeito: 0 apagada, até 1 no flash. Piscada dupla (acende, apaga, acende fraquejando) em
// intervalos irregulares de ~3 s. Determinística: o mesmo `t` dá sempre o mesmo valor.
float regras_piscada_cela(float t);

// ------------------------------------------------------------------ placar, vidas e interativos
int regras_placar(const EventoPlacar *eventos, int n);
int regras_placar_da_fase(const EventoPlacar *eventos, int n, int fase);
int regras_bonus_tempo(float tempo_total);
int regras_integridade_vidro(const Rachadura *rachaduras, int n, int vidas_max);
// Depois de um erro: sem vidas, o vidro rompe (EV_VIDRO_ROMPEU); com vidas, nada muda de fase.
EventoFase regras_evento_do_erro(int vidas_restantes);
bool regras_interativo_ativo(TipoInterativo tipo, int fase);

// ------------------------------------------------------------------ sanidade (regras_sanidade.c)
// Um quadro de sanidade (0..SANIDADE_MAX). Nos corredores (`fase_saida`), cai com a criatura a menos de
// SANIDADE_RAIO (de QUEDA_MIN × ACELERA_LONGE na borda a QUEDA_MAX × ACELERA_PERTO em SANIDADE_PERTO,
// × `surgindo`, metade se `escondido`) e
// só volta a subir depois de SANIDADE_ESPERA_REGEN s longe dela (`tempo_longe`). Nas outras fases de jogo,
// sobe sempre devagar.
float regras_sanidade_passo(float s, float dist, float surgindo, bool escondido, float tempo_longe,
                            bool fase_saida, float dt);
// Efeito de um evento de fase: partida nova → cheia; tarefa concluída → bônus; recomeço → piso.
float regras_sanidade_bonus(float s, EventoFase ev);
// Bônus dos conectivos decifrados neste quadro (`agora` − `antes`, se cresceu).
float regras_sanidade_conectivos(float s, int antes, int agora);
// Força do efeito na tela (0..1), linear no que falta de sanidade: 1 − s/SANIDADE_MAX.
float regras_sanidade_intensidade(float s);

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
