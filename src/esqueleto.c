// esqueleto.c — o modelo escolhido (as tabelas ficam em modelos_criatura.c), a cinemática direta, as
// matrizes de skinning e o IK de dois ossos. Puro: só raymath.h.
#include "esqueleto.h"
#include <math.h>
#include <string.h>

// O modelo escolhido (modelos_criatura.c). Um só por partida: o rig e a animação leem daqui.
static const ModeloCriatura *atual = &MODELOS_CRIATURA[CRIATURA_PRECURSOR];
#define OSSOS (atual->ossos)

// [REQ:PURA-modelos_criatura] o modelo escolhido: as tabelas de cada um ficam em modelos_criatura.c
void esqueleto_usa(int id) {
    if (id >= 0 && id < CRIATURAS_TOTAL) atual = &MODELOS_CRIATURA[id];
}

int esqueleto_modelo_por_nome(const char *nome) {
    for (int i = 0; i < CRIATURAS_TOTAL; i++) // padrão FOLD: o primeiro com esse nome
        if (strcmp(MODELOS_CRIATURA[i].nome, nome) == 0) return i;
    return -1;
}

const ModeloCriatura *esqueleto_modelo(void) {
    return atual;
}

typedef char confere_ossos_glsl[(OSSOS_TOTAL == OSSOS_TOTAL_GLSL) ? 1 : -1]; // não compila se divergir

const OssoRepouso *esqueleto_osso(int i) {
    return &OSSOS[i];
}

float esqueleto_comprimento(int osso) {
    return Vector3Distance(OSSOS[osso].cabeca, OSSOS[osso].cauda);
}

Pose esqueleto_pose_repouso(void) {
    Pose p;
    for (int i = 0; i < OSSOS_TOTAL; i++) // padrão MAP: todos os ossos sem rotação
        p.local[i] = QuaternionIdentity();
    p.raiz = (Vector3){0};
    return p;
}

// Os pais vêm antes dos filhos na tabela: uma passada só, do quadril para as pontas.
void esqueleto_fk(const Pose *p, PoseGlobal *g) {
    for (int i = 0; i < OSSOS_TOTAL; i++) { // padrão MAP: rotação e articulação de cada osso
        int pai = OSSOS[i].pai;
        if (pai < 0) {
            g->global[i] = p->local[i];
            g->junta[i] = Vector3Add(OSSOS[i].cabeca, p->raiz);
            continue;
        }
        g->global[i] = QuaternionNormalize(QuaternionMultiply(g->global[pai], p->local[i]));
        Vector3 osso = Vector3Subtract(OSSOS[i].cabeca, OSSOS[pai].cabeca);
        g->junta[i] = Vector3Add(g->junta[pai], Vector3RotateByQuaternion(osso, g->global[pai]));
    }
}

Vector3 esqueleto_ponto(const PoseGlobal *g, int osso, Vector3 repouso) {
    Vector3 local = Vector3Subtract(repouso, OSSOS[osso].cabeca);
    return Vector3Add(g->junta[osso], Vector3RotateByQuaternion(local, g->global[osso]));
}

// v → junta + R·(v − cabeça): a rotação do osso com a translação que leva a cabeça do repouso à junta.
void esqueleto_matrizes(const PoseGlobal *g, Matrix *saida) {
    for (int i = 0; i < OSSOS_TOTAL; i++) { // padrão MAP: uma matriz por osso
        Matrix m = QuaternionToMatrix(g->global[i]);
        Vector3 t = Vector3Subtract(g->junta[i], Vector3Transform(OSSOS[i].cabeca, m));
        m.m12 = t.x;
        m.m13 = t.y;
        m.m14 = t.z;
        saida[i] = m;
    }
}

// ------------------------------------------------------------------ IK de dois ossos
// A rotação que leva a base (a, n) do repouso para a base (a', n'): a é a direção do osso e n, a normal do
// plano em que a articulação dobra. Fixar n evita que o membro gire sobre o próprio eixo.
static Matrix base(Vector3 a, Vector3 n) {
    a = Vector3Normalize(a);
    n = Vector3Normalize(Vector3Subtract(n, Vector3Scale(a, Vector3DotProduct(n, a))));
    Vector3 b = Vector3CrossProduct(a, n);
    Matrix m = MatrixIdentity(); // colunas a, n, b
    m.m0 = a.x, m.m1 = a.y, m.m2 = a.z;
    m.m4 = n.x, m.m5 = n.y, m.m6 = n.z;
    m.m8 = b.x, m.m9 = b.y, m.m10 = b.z;
    return m;
}

static Quaternion alinha(Vector3 a0, Vector3 n0, Vector3 a1, Vector3 n1) {
    Matrix r = MatrixMultiply(MatrixTranspose(base(a0, n0)), base(a1, n1)); // B1 · B0ᵀ (ordem da raymath)
    return QuaternionNormalize(QuaternionFromMatrix(r));
}

// IK "macio": perto do alcance máximo a distância é comprimida suavemente (nunca passa de 99,5% de L1 + L2).
// Sem isso, o joelho estala nos últimos centímetros, onde o ângulo do triângulo muda depressa demais.
static float distancia_macia(float d, float l1, float l2) {
    float maximo = (l1 + l2) * 0.995f, macio = 0.06f * (l1 + l2), inicio = maximo - macio;
    if (d > inicio) d = inicio + macio * (1.0f - expf(-(d - inicio) / macio));
    return fmaxf(d, fabsf(l1 - l2) + 1e-3f);
}

// Onde fica o joelho (ou o cotovelo): triângulo de lados L1, L2 e a distância até o alvo, dobrando para o
// polo. A articulação nunca trava esticada nem dobra ao contrário.
static Vector3 articulacao_meio(Vector3 a, Vector3 alvo, float l1, float l2, Vector3 polo, Vector3 *fim) {
    Vector3 para = Vector3Subtract(alvo, a);
    float d = Vector3Length(para);
    Vector3 u = d > 1e-5f ? Vector3Scale(para, 1.0f / d) : (Vector3){0, -1, 0};
    d = distancia_macia(d, l1, l2);
    Vector3 v = Vector3Subtract(polo, Vector3Scale(u, Vector3DotProduct(polo, u)));
    v = Vector3LengthSqr(v) > 1e-8f ? Vector3Normalize(v) : (Vector3){0, 0, 1};
    float c = Clamp((l1 * l1 + d * d - l2 * l2) / (2.0f * l1 * d), -1.0f, 1.0f);
    *fim = Vector3Add(a, Vector3Scale(u, d));
    return Vector3Add(a, Vector3Add(Vector3Scale(u, l1 * c), Vector3Scale(v, l1 * sqrtf(1.0f - c * c))));
}

// [REQ:PURA-esqueleto] IK de dois ossos: o membro alcança o alvo dobrando para o polo, sem torcer
float esqueleto_ik(Pose *p, const PoseGlobal *g, int osso1, Vector3 alvo, Vector3 polo) {
    int osso2 = osso1 + 1, pai = OSSOS[osso1].pai;
    float l1 = esqueleto_comprimento(osso1), l2 = esqueleto_comprimento(osso2);
    Vector3 a = g->junta[osso1], fim;
    Vector3 meio = articulacao_meio(a, alvo, l1, l2, polo, &fim);
    Vector3 d1_rep = Vector3Subtract(OSSOS[osso2].cabeca, OSSOS[osso1].cabeca);
    Vector3 d2_rep = Vector3Subtract(OSSOS[osso2].cauda, OSSOS[osso2].cabeca);
    Vector3 n_rep = Vector3CrossProduct(d1_rep, d2_rep);
    if (Vector3LengthSqr(n_rep) < 1e-10f) // membro reto no repouso: o plano de dobra é o de lado (x)
        n_rep = (Vector3){osso1 == OSSO_COXA_E || osso1 == OSSO_COXA_D ? 1.0f : -1.0f, 0.0f, 0.0f};
    Vector3 d1 = Vector3Subtract(meio, a), d2 = Vector3Subtract(fim, meio);
    Vector3 n = Vector3CrossProduct(d1, d2);
    Quaternion g1 = alinha(d1_rep, n_rep, d1, n), g2 = alinha(d2_rep, n_rep, d2, n);
    Quaternion g_pai = pai >= 0 ? g->global[pai] : QuaternionIdentity();
    p->local[osso1] = QuaternionNormalize(QuaternionMultiply(QuaternionInvert(g_pai), g1));
    p->local[osso2] = QuaternionNormalize(QuaternionMultiply(QuaternionInvert(g1), g2));
    return Vector3Distance(fim, alvo);
}

// ------------------------------------------------------------------ a mandíbula
// Medidas do arquivo (atual->queixo): o queixo fica abaixo da boca e à frente do pescoço.

static float suaviza(float de, float ate, float x) {
    float t = Clamp((x - de) / (ate - de), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// 1 no queixo; cai a zero na linha da boca, embaixo, nas laterais e para trás (sem rasgar a pele).
float esqueleto_peso_mandibula(Vector3 v) {
    const QueixoModelo *q = &atual->queixo;
    if (q->meia_largura <= 0.0f) return 0.0f; // modelo sem boca
    return suaviza(q->y_max, q->y_max - 0.025f, v.y) * suaviza(q->y_min, q->y_min + 0.03f, v.y) *
           suaviza(q->meia_largura, q->meia_largura - 0.03f, fabsf(v.x - q->x_centro)) *
           suaviza(q->z_min, q->z_min + 0.04f, v.z);
}
