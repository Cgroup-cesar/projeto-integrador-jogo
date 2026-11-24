// regras_animacao.c — um quadro da animação do Precursor: mede o movimento, ajusta os pesos das camadas
// (crossfade), soma as camadas em ângulos por osso, resolve os pés e as mãos por IK e grava as matrizes.
// Os passos e as cenas roteirizadas ficam em regras_animacao_cenas.c. Puro: só raymath.h.
#include "regras_animacao.h"
#include <math.h>
#include <string.h>

// Até onde cada osso gira a partir do repouso (graus; x = para a frente/baixo, y = gira, z = rola): nada
// de ângulo absurdo. O joelho e o cotovelo só dobram para o lado certo (o IK também respeita isso).
static const Vector3 LIMITE_MIN[OSSOS_TOTAL] = {
    {-30, -30, -20}, {-25, -25, -20},  {-25, -25, -20},  {-30, -45, -25}, {-40, -65, -30},  {-2, 0, 0},
    {-20, -25, -25}, {-90, -40, -60},  {-120, -30, -10}, {-50, -20, -30}, {-30, -10, -20},  {-20, -25, -25},
    {-90, -40, -60}, {-120, -30, -10}, {-50, -20, -30},  {-30, -10, -20}, {-110, -30, -30}, {-35, -5, -5},
    {-40, -15, -15}, {-110, -30, -30}, {-35, -5, -5},    {-40, -15, -15},
};
static const Vector3 LIMITE_MAX[OSSOS_TOTAL] = {
    {30, 30, 20}, {40, 25, 20}, {40, 25, 20}, {40, 45, 25}, {40, 65, 30}, {25, 0, 0},
    {20, 25, 25}, {50, 40, 60}, {5, 30, 10},  {50, 20, 30}, {70, 10, 20}, {20, 25, 25},
    {50, 40, 60}, {5, 30, 10},  {50, 20, 30}, {70, 10, 20}, {40, 30, 30}, {120, 5, 5},
    {40, 15, 15}, {40, 30, 30}, {120, 5, 5},  {40, 15, 15},
};

// ================================================================== utilidades
static float aproxima(float x, float alvo, float taxa, float dt) {
    return x + (alvo - x) * fminf(1.0f, taxa * dt);
}

static float envelope(float t, float sobe, float fica, float desce) {
    if (t < 0.0f) return 0.0f;
    if (t < sobe) return t / sobe;
    if (t < sobe + fica) return 1.0f;
    return fmaxf(0.0f, 1.0f - (t - sobe - fica) / desce);
}

static void soma(Vector3 *ang, int osso, float x, float y, float z) {
    ang[osso] = Vector3Add(ang[osso], (Vector3){x, y, z});
}

static Quaternion euler(Vector3 graus) {
    return QuaternionFromEuler(graus.x * DEG2RAD, graus.y * DEG2RAD, graus.z * DEG2RAD);
}

// mundo → corpo (m): x = a esquerda dele, z = a frente, y = altura
static Vector3 para_corpo(const EntradaAnim *in, Vector3 p) {
    Vector3 d = Vector3Subtract(p, in->pos);
    float c = cosf(in->yaw), s = sinf(in->yaw);
    return (Vector3){d.x * c - d.z * s, d.y, d.x * s + d.z * c};
}

// corpo (m) → modelo (unidades do arquivo, com os pés em base_y)
static Vector3 corpo_para_modelo(const EntradaAnim *in, Vector3 b) {
    return (Vector3){b.x / in->escala, b.y / in->escala + in->base_y, b.z / in->escala};
}

// ================================================================== estado
static void reinicia(AnimCriatura *a, const EntradaAnim *in) {
    memset(a, 0, sizeof(*a));
    a->iniciado = true;
    a->pos_ant = in->pos;
    a->yaw_ant = in->yaw;
    a->passada_m = ANIM_PASSADA_M;
    a->tique_prox = ANIM_TIQUE_MIN_S * 0.5f;
    a->espiada_prox = ANIM_ESPIADA_MIN_S;
    a->w_bueiro = in->bueiro_t >= 0.0f ? 1.0f : 0.0f;
    a->w_vidro = in->mao_no_vidro ? 1.0f : 0.0f;
    a->pose = esqueleto_pose_repouso();
    regras_anim_pes(a, in, 0.0f, 0.0f); // pés plantados no lugar neutro
}

static void mede(AnimCriatura *a, const EntradaAnim *in, float dt) {
    if (dt <= 0.0f) return;
    Vector3 bruta = Vector3Scale(Vector3Subtract(in->pos, a->pos_ant), 1.0f / dt);
    bruta.y = 0.0f;
    a->vel = Vector3Lerp(a->vel, bruta, fminf(1.0f, 12.0f * dt));
    float giro = atan2f(sinf(in->yaw - a->yaw_ant), cosf(in->yaw - a->yaw_ant)) / dt;
    a->giro = aproxima(a->giro, giro, 8.0f, dt);
    a->pos_ant = in->pos;
    a->yaw_ant = in->yaw;
}

// O crossfade: cada camada persegue o seu alvo; nenhuma pose troca de uma vez.
static void atualiza_pesos(AnimCriatura *a, const EntradaAnim *in, float dt) {
    float v = sqrtf(a->vel.x * a->vel.x + a->vel.z * a->vel.z);
    a->w_anda = aproxima(a->w_anda, Clamp(v / 1.1f, 0.0f, 1.0f), 5.0f, dt);
    a->w_persegue = aproxima(a->w_persegue, in->persegue, ANIM_TRANSICAO, dt);
    a->w_escuta = aproxima(a->w_escuta, in->escuta, ANIM_TRANSICAO * 0.6f, dt);
    a->w_luz = aproxima(a->w_luz, in->lanterna, in->lanterna > a->w_luz ? 7.0f : 2.0f, dt);
    a->w_vidro = aproxima(a->w_vidro, in->mao_no_vidro ? 1.0f : 0.0f, ANIM_TRANSICAO * 0.6f, dt);
    a->w_fala = aproxima(a->w_fala, in->fala ? 1.0f : 0.0f, 6.0f, dt);
    // a saída do bueiro começa debaixo do chão (invisível): entra de uma vez e sai com crossfade
    a->w_bueiro = in->bueiro_t >= 0.0f ? 1.0f : aproxima(a->w_bueiro, 0.0f, 2.5f, dt);
    a->w_distorcao = in->distorcao;
    a->w_convulsao = aproxima(a->w_convulsao, in->convulsao ? 1.0f : 0.0f, 6.0f, dt);
    a->respira += dt / (ANIM_RESPIRA_S * (1.0f + 0.15f * regras_anim_ruido(a->t * 0.11f)) *
                        (1.0f + 0.5f * a->w_escuta));
}

// Os braços pendem pesados: uma mola amortecida segue o balanço oposto ao da perna do mesmo lado, com atraso.
static void atualiza_bracos(AnimCriatura *a, const EntradaAnim *in, float dt) {
    float frente_e = para_corpo(in, a->pe[0].agora).z, frente_d = para_corpo(in, a->pe[1].agora).z;
    float k = Clamp((frente_e - frente_d) / (0.5f * a->passada_m), -1.0f, 1.0f) * a->w_anda;
    float w = ANIM_BRACO_MOLA, zeta = 0.45f;
    for (int l = 0; l < 2; l++) { // padrão MAP: um braço de cada vez
        float alvo = (l == 0 ? -k : k) * ANIM_BRACO_GRAUS;
        Mola *m = &a->braco[l];
        m->v += (w * w * (alvo - m->x) - 2.0f * zeta * w * m->v) * dt;
        m->x += m->v * dt;
    }
}

// ================================================================== a cabeça
// Para onde ela tende a olhar (graus, no corpo): o alvo dado pelo jogo, ou uma espiada lenta para um lado
// de tempos em tempos; ao virar o corpo, a cabeça vai primeiro.
static void espiadas(AnimCriatura *a) {
    if (a->espiada_t > 0.0f || a->t < a->espiada_prox) return;
    int n = (int)(a->t * 7.0f);
    a->espiada_graus = (regras_anim_sorteio(n, 21) < 0.5f ? -1.0f : 1.0f) * ANIM_ESPIADA_GRAUS *
                       (0.5f + 0.5f * regras_anim_sorteio(n, 22));
    a->espiada_t = 1.2f + 1.4f * regras_anim_sorteio(n, 23);
    a->espiada_prox = a->t + a->espiada_t + ANIM_ESPIADA_MIN_S +
                      (ANIM_ESPIADA_MAX_S - ANIM_ESPIADA_MIN_S) * regras_anim_sorteio(n, 24);
}

static void atualiza_olhar(AnimCriatura *a, const EntradaAnim *in, float dt) {
    float yaw = 0.0f, pitch = 0.0f;
    a->espiada_t = fmaxf(0.0f, a->espiada_t - dt);
    if (in->tem_olhar) {
        Vector3 rel = para_corpo(in, in->olhar);
        yaw = atan2f(rel.x, rel.z) * RAD2DEG;
        pitch = atan2f(2.3f - rel.y, fmaxf(0.3f, sqrtf(rel.x * rel.x + rel.z * rel.z))) * RAD2DEG;
    } else {
        espiadas(a);
        yaw = a->espiada_t > 0.0f ? a->espiada_graus : 0.0f;
    }
    yaw += Clamp(a->giro * RAD2DEG * 0.35f, -30.0f, 30.0f);
    yaw = Clamp(yaw, -65.0f, 65.0f) * (1.0f - a->w_luz);
    pitch = Clamp(pitch, -25.0f, 35.0f) * (1.0f - a->w_luz);
    float teto = ANIM_OLHAR_GRAUS_S * (1.0f + 1.5f * a->w_persegue) * dt;
    a->olhar_yaw += Clamp((yaw - a->olhar_yaw) * 3.0f * dt, -teto, teto);
    a->olhar_pitch += Clamp((pitch - a->olhar_pitch) * 3.0f * dt, -teto, teto);
}

// ================================================================== tiques
// Raros e pequenos: um dedo que se contrai, a cabeça que inclina, um ombro que se ajusta, a mandíbula.
#define TIQUE_SOBE  0.09f
#define TIQUE_DESCE 0.5f

static float tique_fica(const AnimCriatura *a) {
    return 0.15f + 0.55f * regras_anim_sorteio(a->tique_n, 3);
}

static void atualiza_tiques(AnimCriatura *a, float dt) {
    bool cena = a->w_bueiro > 0.1f || a->w_distorcao > 0.1f || a->w_convulsao > 0.1f;
    if (a->tique_t > 0.0f) { // um tique por vez: o próximo só depois deste acabar
        a->tique_t += dt;
        if (a->tique_t > TIQUE_SOBE + tique_fica(a) + TIQUE_DESCE) a->tique_t = 0.0f;
        return;
    }
    if (cena || a->t < a->tique_prox) return;
    a->tique_n++;
    a->tique_tipo = (int)(regras_anim_sorteio(a->tique_n, 1) * 6.0f) % 6;
    a->tique_t = 1e-4f;
    float espera =
        ANIM_TIQUE_MIN_S + (ANIM_TIQUE_MAX_S - ANIM_TIQUE_MIN_S) * regras_anim_sorteio(a->tique_n, 2);
    a->tique_prox = a->t + espera;
}

static void camada_tiques(const AnimCriatura *a, Vector3 *ang) {
    float e = envelope(a->tique_t, TIQUE_SOBE, tique_fica(a), TIQUE_DESCE) * ANIM_TIQUE_GRAUS;
    float lado = regras_anim_sorteio(a->tique_n, 4) < 0.5f ? -1.0f : 1.0f;
    if (e <= 0.0f) return;
    switch (a->tique_tipo) {
    case 0:
        soma(ang, OSSO_DEDOS_E, 2.0f * e, 0, -0.5f * e);
        break;
    case 1:
        soma(ang, OSSO_DEDOS_D, 2.0f * e, 0, 0.5f * e);
        break;
    case 2:
        soma(ang, OSSO_CABECA, 0, 0.5f * e * lado, e * lado);
        break;
    case 3:
        soma(ang, OSSO_OMBRO_E, 0, -0.4f * e, 0.7f * e);
        break;
    case 4:
        soma(ang, OSSO_OMBRO_D, 0, 0.4f * e, -0.7f * e);
        break;
    default:
        soma(ang, OSSO_MANDIBULA, 0.6f * e, 0, 0);
        break;
    }
}

// ================================================================== camadas
// Sempre: coluna curvada, ombros fechados, cabeça baixa e projetada à frente (o pescoço leva, ela compensa).
static void camada_postura(Vector3 *ang) {
    soma(ang, OSSO_LOMBAR, ANIM_CURVA_GRAUS * 0.5f, 0, 0);
    soma(ang, OSSO_PEITO, ANIM_CURVA_GRAUS * 0.6f, 0, 0);
    soma(ang, OSSO_PESCOCO, ANIM_CABECA_FRENTE, 0, 0);
    soma(ang, OSSO_CABECA, -ANIM_CABECA_FRENTE * 0.6f, 0, 0);
    soma(ang, OSSO_OMBRO_E, 0, -ANIM_OMBROS_GRAUS, 3.0f);
    soma(ang, OSSO_OMBRO_D, 0, ANIM_OMBROS_GRAUS, -3.0f);
    for (int l = 0; l < 2; l++) { // padrão MAP: os dois braços pendem, cotovelos e dedos meio dobrados
        soma(ang, OSSO_OMBRO(l) + 1, -4.0f, 0, 0);
        soma(ang, OSSO_OMBRO(l) + 2, -8.0f, 0, 0);
        soma(ang, OSSO_OMBRO(l) + 4, 8.0f, 0, 0);
    }
}

// Parado: respiração lenta no peito e nos ombros, o peso passando de uma perna para a outra, a cabeça viva.
static void camada_parado(const AnimCriatura *a, Vector3 *ang, Vector3 *raiz, float w) {
    float r = sinf(2.0f * PI * a->respira) * ANIM_RESPIRA_GRAUS * w * (1.0f - 0.5f * a->w_escuta);
    soma(ang, OSSO_PEITO, -0.6f * r, 0, 0);
    soma(ang, OSSO_PESCOCO, 0.3f * r, 0, 0);
    soma(ang, OSSO_OMBRO_E, 0, 0, 0.5f * r);
    soma(ang, OSSO_OMBRO_D, 0, 0, -0.5f * r);
    float peso = regras_anim_ruido(a->t / ANIM_PESO_S) * w;
    raiz->x += ANIM_PESO_M * peso;
    raiz->y -= 0.012f * fabsf(peso);
    soma(ang, OSSO_QUADRIL, 0, 0, 2.0f * peso);
    soma(ang, OSSO_LOMBAR, 0, 0, -1.5f * peso);
    soma(ang, OSSO_CABECA, 1.5f * w * regras_anim_ruido(a->t * 0.23f + 3.0f),
         2.5f * w * regras_anim_ruido(a->t * 0.31f + 11.0f), 0);
}

// Andando: o quadril sobe no meio do passo, vai para o lado do pé de apoio, cai do lado do pé no ar (mais
// do lado direito: a perna "mais curta") e gira com a perna que avança; o tronco gira ao contrário.
static void camada_anda(const AnimCriatura *a, const EntradaAnim *in, Vector3 *ang, Vector3 *raiz, float w) {
    float ze = para_corpo(in, a->pe[0].agora).z, zd = para_corpo(in, a->pe[1].agora).z;
    float k = Clamp((ze - zd) / (0.5f * a->passada_m), -1.0f, 1.0f) * w;
    float ar_e = a->pe[0].no_ar ? sinf(PI * a->pe[0].s) : 0.0f,
          ar_d = a->pe[1].no_ar ? sinf(PI * a->pe[1].s) : 0.0f;
    raiz->y += (ANIM_SOBE_M * fmaxf(ar_e, ar_d) - 0.05f - 0.06f * a->w_persegue) * w;
    raiz->x += ANIM_BALANCO_M * (ar_d - ar_e) * w;
    soma(ang, OSSO_QUADRIL, 0, -ANIM_QUADRIL_GRAUS * k, (-3.0f * ar_e + 4.2f * ar_d) * w);
    soma(ang, OSSO_LOMBAR, 4.0f * w, 0.55f * ANIM_QUADRIL_GRAUS * k, 0);
    soma(ang, OSSO_PEITO, 3.0f * w, 0.55f * ANIM_QUADRIL_GRAUS * k, 0);
    for (int l = 0; l < 2; l++) { // padrão MAP: o balanço dos braços vem da mola (com atraso)
        soma(ang, OSSO_OMBRO(l) + 1, -a->braco[l].x, 0, 0);
        soma(ang, OSSO_OMBRO(l) + 2, -0.4f * fmaxf(0.0f, a->braco[l].x), 0, 0);
    }
}

static void camada_estados(const AnimCriatura *a, Vector3 *ang, float w) {
    float p = a->w_persegue * w, e = a->w_escuta * w, f = a->w_fala * w;
    soma(ang, OSSO_LOMBAR, ANIM_INCLINA_CORRE * 0.5f * p + 5.0f * e, 0, 0);
    soma(ang, OSSO_PEITO, ANIM_INCLINA_CORRE * 0.5f * p + 4.0f * e, 0, 0);
    soma(ang, OSSO_PESCOCO, 4.0f * p + 8.0f * e, 0, 0);
    soma(ang, OSSO_MANDIBULA, 6.0f * p + f * (4.0f + 7.0f * fabsf(regras_anim_ruido(a->t * 5.5f))), 0, 0);
    float fungada = fmodf(a->t, 3.2f) < 0.6f ? sinf(fmodf(a->t, 3.2f) / 0.6f * 2.0f * PI) * 2.5f : 0.0f;
    soma(ang, OSSO_CABECA, -4.0f * e + fungada * e + 2.0f * f * regras_anim_ruido(a->t * 1.7f), 0, 12.0f * e);
    for (int l = 0; l < 2; l++) { // padrão MAP: na perseguição, braços um pouco à frente e dedos abertos
        soma(ang, OSSO_OMBRO(l) + 1, -12.0f * p, 0, 0);
        soma(ang, OSSO_OMBRO(l) + 2, -18.0f * p, 0, 0);
        soma(ang, OSSO_OMBRO(l) + 4, -12.0f * p, 0, 0);
        soma(ang, OSSO_OMBRO(l), 0, 0, LADO_SINAL(l) * 3.0f * e);
    }
}

// A lanterna nele (na cela): vira o rosto, abaixa a cabeça, encolhe o ombro e gira o tronco. Contido: o
// braço fica onde está (erguido pela rotação do cotovelo, o antebraço do modelo se torcia).
static void camada_luz_e_vidro(const AnimCriatura *a, Vector3 *ang, float w) {
    float l = a->w_luz * w, v = a->w_vidro * w;
    soma(ang, OSSO_CABECA, 10.0f * l, -22.0f * l, 0);
    soma(ang, OSSO_PESCOCO, 6.0f * l + 6.0f * v, 0, 0);
    soma(ang, OSSO_LOMBAR, -4.0f * v, -6.0f * l, 0);
    soma(ang, OSSO_OMBRO_E, 0, 0, 6.0f * l);
    soma(ang, OSSO_OMBRO_D, 0, 0, -4.0f * l);
    soma(ang, OSSO_DEDOS_E, 12.0f * l, 0, 0);
    soma(ang, OSSO_MAO_E, -20.0f * v, 0, 0);
    soma(ang, OSSO_DEDOS_E, -15.0f * v, 0, 0);
}

static void soma_cena(Vector3 *ang, Vector3 *raiz, const CenaAnim *c) {
    for (int i = 0; i < OSSOS_TOTAL; i++) // padrão MAP: os ângulos da cena por cima de tudo
        ang[i] = Vector3Add(ang[i], c->ang[i]);
    *raiz = Vector3Add(*raiz, c->raiz);
}

static void camadas(const AnimCriatura *a, const EntradaAnim *in, const CenaAnim *c, Vector3 *ang,
                    Vector3 *raiz) {
    float livre = (1.0f - a->w_bueiro) * (1.0f - a->w_convulsao); // fora das cenas
    camada_postura(ang);
    camada_parado(a, ang, raiz, (1.0f - a->w_anda) * livre);
    camada_anda(a, in, ang, raiz, a->w_anda * livre);
    camada_estados(a, ang, livre);
    camada_luz_e_vidro(a, ang, livre);
    camada_tiques(a, ang);
    soma(ang, OSSO_PESCOCO, 0.3f * a->olhar_pitch * livre, 0.35f * a->olhar_yaw * livre,
         0); // o resto: cabeça
    soma_cena(ang, raiz, c);
    for (int i = 0; i < OSSOS_TOTAL; i++) { // padrão MAP: cada osso dentro do seu limite
        ang[i].x = Clamp(ang[i].x, LIMITE_MIN[i].x, LIMITE_MAX[i].x);
        ang[i].y = Clamp(ang[i].y, LIMITE_MIN[i].y, LIMITE_MAX[i].y);
        ang[i].z = Clamp(ang[i].z, LIMITE_MIN[i].z, LIMITE_MAX[i].z);
    }
}

// ================================================================== cenas
static void soma_peso(CenaAnim *c, const CenaAnim *de, float w) {
    for (int i = 0; i < OSSOS_TOTAL; i++) // padrão MAP: a cena entra com o seu peso
        c->ang[i] = Vector3Add(c->ang[i], Vector3Scale(de->ang[i], w));
    c->raiz = Vector3Add(c->raiz, Vector3Scale(de->raiz, w));
    c->w_pes *= 1.0f - w * (1.0f - de->w_pes);
}

static void monta_cena(AnimCriatura *a, const EntradaAnim *in, float dt, CenaAnim *c) {
    memset(c, 0, sizeof(*c));
    c->w_pes = 1.0f;
    CenaAnim parte;
    if (a->w_bueiro > 0.001f) {
        float fim = ANIM_BUEIRO_PRE_S + ANIM_BUEIRO_SAI_S;
        regras_anim_bueiro(in->bueiro_t >= 0.0f ? in->bueiro_t : fim, &parte);
        soma_peso(c, &parte, a->w_bueiro);
        for (int l = 0; l < 2; l++) { // padrão MAP: as mãos na borda
            c->mao[l] = parte.mao[l];
            c->w_mao[l] = parte.w_mao[l] * a->w_bueiro;
        }
    }
    if (a->w_distorcao > 0.001f) {
        regras_anim_distorcao(a->w_distorcao, a->t, &parte);
        soma_peso(c, &parte, 1.0f);
    }
    if (a->w_convulsao > 0.001f) {
        regras_anim_convulsao(a, dt, &parte);
        soma_peso(c, &parte, a->w_convulsao);
    }
}

// ================================================================== IK e cabeça
// A cabeça segura o olhar enquanto o corpo balança: ANIM_ESTABILIZA da rotação dela vem do alvo no mundo
// (inverso do pescoço), o resto segue o corpo.
static void estabiliza_cabeca(AnimCriatura *a, Vector3 ang_cabeca) {
    float w = ANIM_ESTABILIZA * (1.0f - a->w_bueiro) * (1.0f - a->w_convulsao);
    Quaternion extra = euler(ang_cabeca);
    Quaternion olhar = euler((Vector3){a->olhar_pitch, a->olhar_yaw, 0.0f});
    Quaternion parcial = euler((Vector3){a->olhar_pitch * 0.65f, a->olhar_yaw * 0.65f, 0.0f});
    Quaternion firme = QuaternionMultiply(QuaternionInvert(a->global.global[OSSO_PESCOCO]),
                                          QuaternionMultiply(olhar, extra));
    a->pose.local[OSSO_CABECA] = QuaternionSlerp(QuaternionMultiply(parcial, extra), firme, w);
}

static void pernas_ik(AnimCriatura *a, const EntradaAnim *in, float w_pes) {
    if (w_pes <= 0.001f) return;
    for (int l = 0; l < 2; l++) { // padrão MAP: cada perna alcança o seu tornozelo
        int coxa = OSSO_COXA(l);
        const OssoRepouso *pe = esqueleto_osso(coxa + 2);
        Vector3 b = para_corpo(in, a->pe[l].agora);
        Vector3 alvo = corpo_para_modelo(in, (Vector3){b.x, 0.0f, b.z});
        alvo.y = pe->cabeca.y + a->pe[l].agora.y / in->escala;
        Vector3 polo = Vector3RotateByQuaternion((Vector3){0.15f * LADO_SINAL(l), 0.0f, 1.0f},
                                                 a->global.global[OSSO_QUADRIL]);
        Quaternion fk1 = a->pose.local[coxa], fk2 = a->pose.local[coxa + 1];
        esqueleto_ik(&a->pose, &a->global, coxa, alvo, polo);
        a->pose.local[coxa] = QuaternionSlerp(fk1, a->pose.local[coxa], w_pes);
        a->pose.local[coxa + 1] = QuaternionSlerp(fk2, a->pose.local[coxa + 1], w_pes);
    }
}

// O pé fica rente ao chão; no passo, o calcanhar sobe primeiro e o pé direito arrasta a ponta.
static void pes_no_chao(AnimCriatura *a, float w_pes) {
    esqueleto_fk(&a->pose, &a->global);
    for (int l = 0; l < 2; l++) { // padrão MAP: a rotação do pé a partir da canela
        int pe = OSSO_COXA(l) + 2;
        float s = a->pe[l].no_ar ? a->pe[l].s : 0.0f;
        float graus = 18.0f * sinf(PI * s) * (1.0f - 2.0f * s) + (l == 1 ? 10.0f * sinf(PI * s) : 0.0f);
        Quaternion chao = euler((Vector3){graus, 0.0f, 0.0f});
        Quaternion local = QuaternionMultiply(QuaternionInvert(a->global.global[pe - 1]), chao);
        a->pose.local[pe] = QuaternionSlerp(a->pose.local[pe], local, w_pes);
    }
}

// As mãos alcançam um alvo (o punho vai até lá). `polo`: para onde o cotovelo aponta (no peito); `dedos`:
// para onde a mão aponta depois (no modelo), já que ela não deve só seguir a linha do antebraço.
typedef struct {
    Vector3 alvo, polo, dedos;
    float w;
} AlcanceMao;

static void mao_ik(AnimCriatura *a, const EntradaAnim *in, int lado, AlcanceMao m) {
    if (m.w <= 0.001f) return;
    int braco = OSSO_OMBRO(lado) + 1;
    Vector3 polo = Vector3RotateByQuaternion(m.polo, a->global.global[OSSO_PEITO]);
    Quaternion fk1 = a->pose.local[braco], fk2 = a->pose.local[braco + 1];
    esqueleto_ik(&a->pose, &a->global, braco, corpo_para_modelo(in, m.alvo), polo);
    a->pose.local[braco] = QuaternionSlerp(fk1, a->pose.local[braco], m.w);
    a->pose.local[braco + 1] = QuaternionSlerp(fk2, a->pose.local[braco + 1], m.w);
    esqueleto_fk(&a->pose, &a->global);
    const OssoRepouso *mao = esqueleto_osso(braco + 2);
    Quaternion rumo = QuaternionFromVector3ToVector3(
        Vector3Normalize(Vector3Subtract(mao->cauda, mao->cabeca)), Vector3Normalize(m.dedos));
    Quaternion local = QuaternionMultiply(QuaternionInvert(a->global.global[braco + 1]), rumo);
    a->pose.local[braco + 2] = QuaternionSlerp(a->pose.local[braco + 2], local, m.w);
}

// Na borda do bueiro: cotovelos para fora e para trás, dedos por cima da borda. No vidro: cotovelo para
// baixo, mão espalmada com os dedos para cima.
static void bracos_ik(AnimCriatura *a, const EntradaAnim *in, const CenaAnim *c) {
    for (int l = 0; l < 2; l++) { // padrão MAP: as mãos na borda do bueiro
        float s = LADO_SINAL(l);
        mao_ik(a, in, l, (AlcanceMao){c->mao[l], {0.7f * s, -0.3f, -0.7f}, {0.0f, -0.3f, 1.0f}, c->w_mao[l]});
    }
    if (a->w_vidro > 0.001f && c->w_mao[0] < 0.5f)
        mao_ik(
            a, in, 0,
            (AlcanceMao){para_corpo(in, in->vidro), {0.3f, -1.0f, -0.25f}, {0.1f, 1.0f, 0.15f}, a->w_vidro});
}

static void monta_pose(AnimCriatura *a, const EntradaAnim *in, const CenaAnim *c) {
    Vector3 ang[OSSOS_TOTAL] = {0}, raiz = {0};
    camadas(a, in, c, ang, &raiz);
    for (int i = 0; i < OSSOS_TOTAL; i++) // padrão MAP: graus → rotação local
        a->pose.local[i] = euler(ang[i]);
    a->pose.raiz = Vector3Scale(raiz, 1.0f / in->escala);
    esqueleto_fk(&a->pose, &a->global);
    estabiliza_cabeca(a, ang[OSSO_CABECA]);
    pernas_ik(a, in, c->w_pes);
    pes_no_chao(a, c->w_pes);
    bracos_ik(a, in, c);
    esqueleto_fk(&a->pose, &a->global);
    esqueleto_matrizes(&a->global, a->ossos);
}

// [REQ:PURA-regras_anim_passo] um quadro: mede, pesa as camadas, anda com os pés, monta a pose
void regras_anim_passo(AnimCriatura *a, const EntradaAnim *in, float dt) {
    if (!a->iniciado || in->teleporte) reinicia(a, in);
    if (in->congelado && a->w_convulsao > 0.01f) return; // final bom: a pose para onde está
    a->t += dt;
    mede(a, in, dt);
    atualiza_pesos(a, in, dt);
    CenaAnim cena;
    monta_cena(a, in, dt, &cena);
    regras_anim_pes(a, in, dt, cena.w_pes);
    atualiza_bracos(a, in, dt);
    atualiza_olhar(a, in, dt);
    atualiza_tiques(a, dt);
    monta_pose(a, in, &cena);
}
