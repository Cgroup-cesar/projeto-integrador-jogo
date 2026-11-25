// esqueleto.h — o rig do Precursor: 22 ossos em hierarquia, medidos no precursor.glb (unidades do arquivo:
// y de −1 a 1, rosto para +z; o lado +x é a esquerda DELE). Funções puras (sem raylib.h): cinemática direta,
// matrizes de skinning e IK de dois ossos. Os testes rodam sem janela (tests/test_regras.c).
//
//   quadril ─ lombar ─ peito ─ pescoço ─ cabeça ─ mandíbula
//      │                 ├─ ombro_e ─ braço_e ─ antebraço_e ─ mão_e ─ dedos_e
//      │                 └─ ombro_d ─ braço_d ─ antebraço_d ─ mão_d ─ dedos_d
//      ├─ coxa_e ─ canela_e ─ pé_e
//      └─ coxa_d ─ canela_d ─ pé_d
#ifndef ESQUELETO_H
#define ESQUELETO_H

#include <stdbool.h>
#include "raymath.h"

typedef enum {
    OSSO_QUADRIL,
    OSSO_LOMBAR,
    OSSO_PEITO,
    OSSO_PESCOCO,
    OSSO_CABECA,
    OSSO_MANDIBULA,
    OSSO_OMBRO_E,
    OSSO_BRACO_E,
    OSSO_ANTEBRACO_E,
    OSSO_MAO_E,
    OSSO_DEDOS_E,
    OSSO_OMBRO_D,
    OSSO_BRACO_D,
    OSSO_ANTEBRACO_D,
    OSSO_MAO_D,
    OSSO_DEDOS_D,
    OSSO_COXA_E,
    OSSO_CANELA_E,
    OSSO_PE_E,
    OSSO_COXA_D,
    OSSO_CANELA_D,
    OSSO_PE_D,
    OSSOS_TOTAL
} Osso;

// O shader de luz declara uniform mat4 ossos[N]: o número precisa ser literal (confere em esqueleto.c).
#define OSSOS_TOTAL_GLSL 22

// Lado 0 = esquerdo (+x), 1 = direito (−x). Os ossos de cada lado vêm em sequência na tabela.
#define OSSO_OMBRO(lado) ((Osso)(OSSO_OMBRO_E + (lado) * 5))
#define OSSO_COXA(lado)  ((Osso)(OSSO_COXA_E + (lado) * 3))
#define LADO_SINAL(lado) ((lado) == 0 ? 1.0f : -1.0f) // +1 no lado +x

typedef struct {
    const char *nome;
    int pai;        // −1 na raiz
    Vector3 cabeca; // o pivô (a articulação), em repouso
    Vector3 cauda;  // a ponta: a próxima articulação, ou o fim do membro
    float raio;     // espessura da carne em volta (pesos do skinning e o corpo procedural)
} OssoRepouso;

const OssoRepouso *esqueleto_osso(int i);
float esqueleto_comprimento(int osso);

// A pose: a rotação local de cada osso (relativa ao pai, a partir do repouso) e o deslocamento da raiz.
typedef struct {
    Quaternion local[OSSOS_TOTAL];
    Vector3 raiz; // unidades do modelo
} Pose;

// A cinemática direta: a rotação de cada osso no espaço do modelo e onde fica cada articulação.
typedef struct {
    Quaternion global[OSSOS_TOTAL];
    Vector3 junta[OSSOS_TOTAL]; // a cabeça de cada osso, já posada
} PoseGlobal;

Pose esqueleto_pose_repouso(void);
void esqueleto_fk(const Pose *p, PoseGlobal *g);
// Um ponto do modelo em repouso, preso a `osso`, levado para a pose (ex.: os olhos presos à cabeça).
Vector3 esqueleto_ponto(const PoseGlobal *g, int osso, Vector3 repouso);
// Matrizes de skinning (repouso → pose), uma por osso: vão para o shader de luz (uniform ossos[]).
void esqueleto_matrizes(const PoseGlobal *g, Matrix *saida);

// IK de dois ossos (coxa + canela, braço + antebraço) no espaço do modelo: grava a rotação local dos dois
// em `p`. O pai de `osso1` precisa estar posado em `g`. `polo` diz para onde a articulação do meio dobra
// (joelho para a frente, cotovelo para trás): nunca dobra ao contrário, e a perna nunca estica por inteiro.
// Devolve quanto faltou para alcançar (0 = alcançou).
float esqueleto_ik(Pose *p, const PoseGlobal *g, int osso1, Vector3 alvo, Vector3 polo);

// 0..1: quanto um vértice do repouso acompanha a mandíbula (o queixo, abaixo da boca e à frente do pescoço).
float esqueleto_peso_mandibula(Vector3 v);

#endif
