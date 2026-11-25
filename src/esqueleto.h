// esqueleto.h — o rig do Precursor: 22 ossos em hierarquia, medidos no precursor_papelao.glb (unidades do
// arquivo: y de −1 a 1, rosto para +z; o lado +x é a esquerda DELE). Funções puras (sem raylib.h): cinemática
// direta, matrizes de skinning e IK de dois ossos. Os testes rodam sem janela (tests/test_regras.c).
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

// A postura que a animação soma ao modelo (graus em x = para a frente/baixo): o arquivo é reto, e a
// curvatura ameaçadora vem daqui.
typedef struct {
    float lombar, peito, pescoco, cabeca;
    float braco, braco_abre, antebraco, mao, dedos;
    float desce_m; // a raiz desce (m): com os pés no chão, o IK dobra os joelhos
    float passo;   // escala da passada (perna mais curta, passo mais curto)
} PosturaModelo;

// Uma marca pintada na cara (os olhos do rosto liso): um meio-disco escuro, com a reta em cima e a curva
// embaixo, encostado na pele e preso ao osso da cabeça (unidades do arquivo).
typedef struct {
    Vector3 centro; // o meio da reta de cima, rente à pele
    Vector3 eixo;   // a direção da reta (de dentro para fora do rosto)
    Vector3 normal; // para onde a pele olha ali
    float raio;
} MarcaRosto;

#define MARCAS_ROSTO_MAX 2
#define MARCA_PONTOS     13 // o centro da reta e 12 pontos na borda (a reta e o arco)

typedef struct {
    const char *arquivo; // em assets/
    OssoRepouso ossos[OSSOS_TOTAL];
    float altura_nominal, base_nominal;  // a caixa do arquivo em y (sem o .glb e nos testes)
    Vector3 olhos[2];                    // os pontinhos de luz do menu, presos à cabeça
    Vector3 rosto;                       // o centro do rosto no susto, em fração da caixa do modelo
    MarcaRosto marcas[MARCAS_ROSTO_MAX]; // modelos de rosto liso: as marcas pintadas (os olhos)
    int n_marcas;
    float peito_m;       // altura do peito no mundo (a lanterna e os holofotes miram ali)
    float cela_avanco_m; // na cela, a mais na direção do vidro
    // abaixo deste y, cada braço é uma peça solta do corpo: rig.c não deixa ponto do braço pegar osso do
    // corpo, nem o contrário
    float bracos_soltos_y;
    PosturaModelo postura;
} ModeloCriatura;

// O modelo do Precursor (modelos_criatura.c): o rig, a animação e o desenho leem daqui.
extern const ModeloCriatura MODELO_CRIATURA;
const ModeloCriatura *esqueleto_modelo(void);

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

// Os pontos de uma marca do rosto (unidades do arquivo): [0] é o centro da reta; de 1 a MARCA_PONTOS − 1, a
// borda, de uma ponta da reta à outra passando pelo arco de baixo (um leque a partir de [0]).
void esqueleto_marca_pontos(const MarcaRosto *m, Vector3 *saida);

#endif
