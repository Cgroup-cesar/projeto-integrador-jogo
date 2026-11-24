// regras_animacao.h — a animação procedural do Precursor sobre o rig (esqueleto.h), como funções puras: o
// jogo diz onde ele está e o que está fazendo (EntradaAnim); a animação devolve a pose. Sem raylib.h, sem
// rand(): o "acaso" (tiques, variação dos passos) vem de um hash do relógio, então é repetível nos testes.
//
// Camadas somadas a cada quadro, cada uma com o seu peso (crossfade, sem saltos de pose):
//   postura curvada (sempre) · parado (respiração, peso, micro-movimentos) · andar (passada com os pés
//   plantados no mundo e IK) · perseguir · escutar · lanterna · fala · mão no vidro · tiques
//   · cenas: saída do bueiro, distorção (puxado pela contenção) e convulsão (final bom).
#ifndef REGRAS_ANIMACAO_H
#define REGRAS_ANIMACAO_H

#include "config.h"
#include "esqueleto.h"

// O que o jogo diz à animação a cada quadro (montado em animacao.c a partir do Estado).
typedef struct {
    Vector3 pos;       // onde ele está (mundo, y = chão)
    float yaw;         // para onde o corpo aponta (rad; 0 = +z)
    float escala;      // metros por unidade do modelo
    float base_y;      // o y dos pés no modelo (unidades do modelo)
    bool teleporte;    // pulou de lugar: replanta os pés e recomeça do zero
    float persegue;    // 0..1
    float escuta;      // 0..1: parado, escutando (na porta do armário)
    float lanterna;    // 0..1: a lanterna nele (na cela)
    bool fala;         // a fenda/mandíbula se mexe
    bool mao_no_vidro; // a mão esquerda espalmada em `vidro`
    Vector3 vidro;     // mundo
    bool tem_olhar;    // a cabeça procura `olhar` (mundo)
    Vector3 olhar;
    float bueiro_t;  // < 0 = fora; senão, segundos desde o começo da saída do bueiro
    Vector3 bueiro;  // o centro da boca do duto (mundo)
    float distorcao; // 0..1
    bool convulsao;  // final bom: convulsiona...
    bool congelado;  // ...e congela (a pose para onde está)
} EntradaAnim;

typedef struct {
    Vector3 ancora; // plantado: onde o tornozelo está no chão (mundo, y = 0)
    Vector3 de;     // no ar: de onde saiu
    Vector3 agora;  // a posição do tornozelo neste quadro (mundo; y = altura acima do chão)
    bool no_ar;
    float s;        // 0..1 no ar
    float parado_t; // há quanto tempo ele está parado com este pé fora do lugar
    float altura;   // altura deste passo (varia)
    float passada;  // a passada de quando levantou (o alvo não muda no meio do passo)
} PeAnim;

typedef struct {
    float x, v; // mola amortecida (braços: o atraso em relação ao tronco)
} Mola;

typedef struct {
    bool iniciado;
    float t;         // relógio próprio
    Vector3 pos_ant; // para medir a velocidade
    float yaw_ant;
    Vector3 vel;     // m/s, no mundo (suavizada)
    float giro;      // rad/s (suavizado)
    float fase;      // a passada: +1 a cada passada completa (dois passos), pela distância andada
    int passo_n;     // conta os passos (sorteio da variação de cada um)
    float passada_m; // comprimento da passada atual
    PeAnim pe[2];    // 0 = esquerdo (+x), 1 = direito
    Mola braco[2];
    float w_anda, w_persegue, w_escuta, w_luz, w_vidro, w_fala, w_bueiro, w_distorcao, w_convulsao;
    float respira;                // fase da respiração (ciclos)
    float olhar_yaw, olhar_pitch; // graus, suavizados
    float espiada_prox, espiada_t, espiada_graus;
    float tique_prox, tique_t;
    int tique_tipo, tique_n;
    float conv_prox; // convulsão: próximo tranco
    int conv_n;
    float conv_alvo[8], conv_atual[8];
    Pose pose;                 // o resultado: rotações locais e raiz
    PoseGlobal global;         // juntas e rotações no espaço do modelo
    Matrix ossos[OSSOS_TOTAL]; // as matrizes de skinning prontas (o desenho só lê)
} AnimCriatura;

// [REQ:PURA-regras_anim_passo] um quadro da animação: atualiza o estado e grava a pose e as matrizes.
void regras_anim_passo(AnimCriatura *a, const EntradaAnim *in, float dt);

// Partes (regras_animacao_cenas.c): o chão de cada passo e as cenas roteirizadas.
void regras_anim_pes(AnimCriatura *a, const EntradaAnim *in, float dt, float w_pes);
Vector3 regras_anim_neutro(const AnimCriatura *a, const EntradaAnim *in, int lado); // mundo
float regras_anim_ruido(float x);            // ruído suave −1..1 (repetível)
float regras_anim_sorteio(int n, int canal); // 0..1 (repetível)

// Uma cena roteirizada: ângulos por osso (graus: x = para a frente/baixo, y = gira, z = rola), a raiz
// (m, no corpo: x = esquerda dele, z = frente) e alvos para as mãos (mundo) com peso.
typedef struct {
    Vector3 ang[OSSOS_TOTAL];
    Vector3 raiz;
    Vector3 mao[2];
    float w_mao[2];
    float w_pes; // 0 = pernas soltas (fora do chão), 1 = pés plantados
} CenaAnim;

void regras_anim_bueiro(float t, CenaAnim *c);
void regras_anim_distorcao(float d, float t, CenaAnim *c);
void regras_anim_convulsao(AnimCriatura *a, float dt, CenaAnim *c);

#endif
