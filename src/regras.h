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
    EV_INTRO_TERMINOU,          // fim das perguntas, do texto datilografado e da carta pelo rádio
    EV_RODADA_CONCLUIDA,        // decifrou e traduziu
    EV_VIDRO_ROMPEU,            // 3º erro: a rodada recomeça
    EV_SAIDA_CONCLUIDA,         // painel resolvido e de volta à sala
    EV_PEGO,                    // a criatura pegou o jogador: a saída recomeça
    EV_TRANSMITIU_CONTRADICAO,  // frase que nunca pode ser verdadeira → final bom
    EV_TRANSMITIU_SATISFAZIVEL, // qualquer outra frase válida → final ruim
    EV_VOLTAR_MENU,             // Enter na tela de fim ou 20 s; pausa; inatividade (feira)
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

// Modo feira: depois de FEIRA_INATIVIDADE_S sem nenhuma tecla numa fase de jogo (inclusive a intro),
// volta ao menu (EV_VOLTAR_MENU). Fora do modo feira, no menu e no fim (que já volta sozinho): nada.
EventoFase regras_evento_inatividade(bool modo_feira, Fase fase, float tempo_inativo);

// O plot twist: a partir da S2 o Varela de verdade desceu até a cela, e quem fala no rádio é o Precursor
// imitando a voz dele (S2, R3 e resposta final). ui_fala usa isto para trocar as falas pela versão falsa.
bool regras_radio_falso(Fase fase);

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

// ------------------------------------------------------------------ mixagem das trilhas (sons longos)
typedef enum {
    TRILHA_MENU,
    TRILHA_VOZ_PRECURSOR,
    TRILHA_VOZ_RADIO,
    TRILHA_CORACAO_NORMAL,
    TRILHA_CORACAO_RAPIDO,
    TRILHA_PASSO_ANDAR,
    TRILHA_PASSO_CORRER,
    TRILHA_TOTAL
} IdTrilha;

// O que está acontecendo no quadro, do ponto de vista do som (trilhas.c monta isto a partir do Estado).
typedef struct {
    bool tela_inicial;        // menu e texto datilografado da intro (antes de abrir no laboratório)
    bool precursor_falando;   // ele "diz" uma frase (símbolos no vidro) ou recebe a transmissão final
    bool radio_falando;       // a legenda da frente é do Varela
    bool criatura_solta;      // nos corredores, fora da cela
    float distancia_criatura; // metros até o jogador
    bool andando, correndo;
    bool silencio;    // os 0,2 s antes do susto, o próprio susto e o silêncio do final bom
    bool radio_falso; // a fala do rádio é do Varela falso: o canto do Precursor entra baixo, por baixo dela
} SituacaoSom;

// Volume alvo (0..1) de cada trilha. Coração: nada além de CORACAO_LONGE_M, normal (mais alto quanto mais
// perto) até CORACAO_PERTO_M, rápido abaixo disso. trilhas.c só faz o fade até estes valores.
void regras_volumes_trilhas(SituacaoSom s, float volumes[TRILHA_TOTAL]);

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
// Onde a corrente trava: andando da SAÍDA para trás, a porta e a entrada (0 = a, 1 = b) que a pessoa precisa
// olhar. porta = −1: a SAÍDA já acende.
typedef struct {
    int porta;
    int entrada;
} Bloqueio;
Bloqueio regras_circuito_bloqueio(const Circuito *c, const bool *entradas);
int regras_bonus_movimentos(int movimentos); // resolver o painel pensando, com poucos movimentos, vale bônus

// ------------------------------------------------------------------ textos do roteiro (nome e gênero)
// Um marcador do roteiro e o que entra no lugar dele: {"NOME", "Ana"} troca "{NOME}" por "Ana".
// As palavras ficam em dialogo.c; aqui só a regra de troca (por isso a função não sabe português).
typedef struct {
    const char *chave;
    const char *valor;
} Marcador;

// Copia `modelo` para `saida` trocando cada "{CHAVE}" pelo valor do marcador e cada "{x|y}" por x (masculino)
// ou y (feminino). Marcador desconhecido ou "{" sem "}" ficam como estão. Nunca passa de `tamanho` bytes e
// nunca corta uma letra acentuada pela metade (UTF-8).
void regras_monta_texto(const char *modelo, const Marcador *marcadores, int n, bool feminino, char *saida,
                        size_t tamanho);

// Edita o nome digitado: `codigo` é a letra (Unicode) ou NOME_APAGAR (Backspace, tira a última letra).
// Aceita letras (com acento), espaço entre palavras, hífen e apóstrofo, até `max_letras` letras.
// Devolve true se o nome mudou.
#define NOME_APAGAR 8
bool regras_nome_edita(char *nome, size_t tamanho, int max_letras, int codigo);

// Legenda com defeito (o nome do falante no rádio falso): em janelas curtas e irregulares, algumas letras
// viram símbolos. Copia `texto` para `saida` e devolve true enquanto está falhando. Determinística (o mesmo
// `t` dá sempre o mesmo resultado) e só troca letras ASCII: acentos (UTF-8) passam intactos.
bool regras_falha_texto(const char *texto, float t, char *saida, size_t tamanho);

#endif
