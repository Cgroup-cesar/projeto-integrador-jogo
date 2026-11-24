// regras_animacao_cenas.c — os passos (pés plantados no mundo, sem deslizar) e as cenas roteirizadas do
// Precursor: a saída do bueiro, a distorção (puxado pela contenção) e a convulsão do final bom. Puro.
#include "regras_animacao.h"
#include <math.h>
#include <string.h>

// ================================================================== acaso repetível
float regras_anim_sorteio(int n, int canal) {
    unsigned int h = (unsigned int)n * 374761393u + (unsigned int)canal * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xffffu) / 65535.0f;
}

// Ruído de valor 1D, suave (−1..1): para coisas lentas e sem ritmo fixo (peso, respiração, cabeça).
float regras_anim_ruido(float x) {
    float i = floorf(x), f = x - i;
    f = f * f * (3.0f - 2.0f * f);
    float a = regras_anim_sorteio((int)i, 99), b = regras_anim_sorteio((int)i + 1, 99);
    return (a + (b - a) * f) * 2.0f - 1.0f;
}

static float trecho(float t, float de, float ate) {
    float x = Clamp((t - de) / (ate - de), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

static float lombada(float t, float de, float ate) { // 0 → 1 → 0
    return sinf(PI * Clamp((t - de) / (ate - de), 0.0f, 1.0f));
}

// ================================================================== os pés
// Onde o tornozelo descansa (mundo, no chão). Andando, os pés ficam mais juntos e embaixo do quadril (o
// modelo, em repouso, tem os pés à frente dele: a passada não alcançaria).
Vector3 regras_anim_neutro(const AnimCriatura *a, const EntradaAnim *in, int lado) {
    const OssoRepouso *coxa = esqueleto_osso(OSSO_COXA(lado)), *pe = esqueleto_osso(OSSO_COXA(lado) + 2);
    float x = pe->cabeca.x * (1.0f - 0.2f * a->w_anda) * in->escala;
    float z = Lerp(pe->cabeca.z, coxa->cabeca.z + 0.04f, a->w_anda) * in->escala;
    float c = cosf(in->yaw), s = sinf(in->yaw);
    return (Vector3){in->pos.x + x * c + z * s, 0.0f, in->pos.z - x * s + z * c};
}

static void replanta(AnimCriatura *a, const EntradaAnim *in) {
    for (int l = 0; l < 2; l++) { // padrão MAP: os dois pés no lugar neutro
        PeAnim *p = &a->pe[l];
        p->ancora = regras_anim_neutro(a, in, l);
        p->agora = p->ancora;
        p->no_ar = false;
        p->s = 0.0f;
        p->parado_t = 0.0f;
    }
}

static void levanta(AnimCriatura *a, PeAnim *p, int lado, bool acomoda) {
    p->no_ar = true;
    p->s = 0.0f;
    p->de = p->ancora;
    p->passada = a->passada_m;
    p->altura = ANIM_PE_ALTURA_M * (0.8f + 0.4f * regras_anim_sorteio(a->passo_n, 7 + lado)) *
                (lado == 1 ? ANIM_PE_ARRASTA : 1.0f) * (acomoda ? 0.5f : 1.0f);
}

// No ar: vai de onde saiu até um pouco à frente do lugar neutro (o corpo continua andando enquanto o pé
// desce). O avanço segue a distância andada, mas nunca demora mais que ANIM_PASSO_LENTO_S.
static void voa(AnimCriatura *a, const EntradaAnim *in, int lado, float andou, float dt, float beta) {
    PeAnim *p = &a->pe[lado];
    float s_passo = (1.0f - beta) * p->passada;
    p->s = fminf(1.0f, p->s + fmaxf(andou / s_passo, dt / ANIM_PASSO_LENTO_S));
    Vector3 alvo = regras_anim_neutro(a, in, lado);
    float v = sqrtf(a->vel.x * a->vel.x + a->vel.z * a->vel.z);
    if (v > 0.05f) { // à frente na medida da velocidade (contínuo: parar não faz o pé pular)
        float frente = ((1.0f - p->s) * s_passo + beta * p->passada * 0.5f) * trecho(v, 0.1f, 0.7f);
        alvo = Vector3Add(alvo, (Vector3){a->vel.x / v * frente, 0.0f, a->vel.z / v * frente});
    }
    float e = p->s * p->s * (3.0f - 2.0f * p->s);
    p->agora = Vector3Lerp(p->de, alvo, e);
    p->agora.y = p->altura * sinf(PI * p->s);
    if (p->s < 1.0f) return;
    p->no_ar = false;
    p->ancora = (Vector3){alvo.x, 0.0f, alvo.z};
    p->agora = p->ancora;
    p->parado_t = 0.0f;
}

// Plantado: fica onde está no mundo (não desliza). Levanta na sua vez da passada, se o outro pé estiver no
// chão; parado, só dá um passinho para se acomodar se ficou longe do lugar (ou depois de girar).
static void pe_plantado(AnimCriatura *a, const EntradaAnim *in, int lado, bool anda, float dt, float beta) {
    PeAnim *p = &a->pe[lado];
    p->agora = p->ancora;
    Vector3 neutro = regras_anim_neutro(a, in, lado);
    float erro = sqrtf((p->ancora.x - neutro.x) * (p->ancora.x - neutro.x) +
                       (p->ancora.z - neutro.z) * (p->ancora.z - neutro.z));
    p->parado_t = anda ? 0.0f : p->parado_t + dt;
    float janela = a->fase - (lado == 1 ? 0.53f : 0.0f); // o direito atrasa um pouco: o passo manca
    janela -= floorf(janela);
    bool vez = anda && janela < 1.0f - beta;
    bool acomoda = !anda && ((erro > 0.12f && p->parado_t > 0.5f) || erro > 0.35f);
    if ((vez || acomoda) && !a->pe[1 - lado].no_ar) levanta(a, p, lado, acomoda);
}

void regras_anim_pes(AnimCriatura *a, const EntradaAnim *in, float dt, float w_pes) {
    if (w_pes < 0.5f) { // pernas soltas (bueiro, distorção): os pés esperam no lugar neutro
        replanta(a, in);
        return;
    }
    float v = sqrtf(a->vel.x * a->vel.x + a->vel.z * a->vel.z), andou = v * dt;
    float beta = Lerp(0.62f, 0.55f, a->w_persegue); // quanto do ciclo cada pé fica no chão
    float antes = a->fase;
    a->fase += andou / a->passada_m;
    if (floorf(a->fase * 2.0f) != floorf(antes * 2.0f)) { // um passo novo: comprimento sorteado
        a->passo_n++;
        float base = Lerp(ANIM_PASSADA_M, ANIM_PASSADA_CORRE_M, a->w_persegue);
        a->passada_m = base * (1.0f + ANIM_PASSO_VARIA * (2.0f * regras_anim_sorteio(a->passo_n, 5) - 1.0f));
    }
    for (int l = 0; l < 2; l++) { // padrão MAP: cada pé, no ar ou plantado
        if (a->pe[l].no_ar)
            voa(a, in, l, andou, dt, beta);
        else
            pe_plantado(a, in, l, v > 0.2f, dt, beta);
    }
}

// ================================================================== saída do bueiro
// O roteiro tem 6,5 s (escala para ANIM_BUEIRO_PRE_S + ANIM_BUEIRO_SAI_S). Ele sobe por trás da boca do duto
// (o corpo 15 cm atrás do centro) e agarra a borda da frente.
//   0,0–1,3  só as mãos sobem e agarram a borda          1,3–2,3  pausa (a mão esquerda reajusta)
//   2,3–3,9  a cabeça sobe devagar e olha em volta       3,9–5,0  puxa o corpo: um ombro, depois o outro
//   5,0–6,0  o tronco passa da borda; o joelho esquerdo vem para o chão
//   6,0–6,5  solta as mãos, a outra perna sai e ele se recompõe curvado
#define BUEIRO_ROTEIRO_S 6.5f

static void bueiro_raiz_e_maos(float r, CenaAnim *c) {
    c->raiz.y = -2.6f + 0.45f * trecho(r, 2.3f, 3.9f) + 0.25f * trecho(r, 3.9f, 4.35f) +
                0.40f * trecho(r, 4.5f, 5.0f) + 0.40f * trecho(r, 5.0f, 5.5f) +
                0.55f * trecho(r, 5.5f, 6.0f) + 0.55f * trecho(r, 6.0f, 6.5f);
    c->raiz.z = -0.15f + 0.15f * trecho(r, 5.0f, 6.4f);
    float reajusta = 0.05f * lombada(r, 1.7f, 2.05f);
    c->mao[0] = (Vector3){0.30f, -0.70f + 0.75f * trecho(r, 0.0f, 1.0f) + reajusta, 0.55f};
    c->mao[1] = (Vector3){-0.32f, -0.70f + 0.75f * trecho(r, 0.3f, 1.3f), 0.53f};
    c->w_mao[0] = c->w_mao[1] = 1.0f - trecho(r, 5.9f, 6.3f);
    c->w_pes = trecho(r, 6.1f, 6.5f);
}

static void bueiro_corpo(float r, CenaAnim *c) {
    // olha em volta com a cabeça rente ao chão (queixo um pouco para cima), depois olha para baixo no esforço
    float yaw =
        -35.0f * trecho(r, 2.7f, 3.2f) + 65.0f * trecho(r, 3.4f, 3.9f) - 30.0f * trecho(r, 4.0f, 4.5f);
    float pitch =
        -12.0f * trecho(r, 2.3f, 2.8f) + 12.0f * trecho(r, 4.0f, 4.6f) + 15.0f * lombada(r, 4.8f, 6.3f);
    c->ang[OSSO_CABECA] = (Vector3){pitch, yaw, 6.0f * lombada(r, 2.7f, 4.4f)};
    float curva = 25.0f * trecho(r, 5.0f, 5.6f) - 33.0f * trecho(r, 6.0f, 6.5f);
    c->ang[OSSO_LOMBAR] = (Vector3){8.0f + curva, 0, 0};
    c->ang[OSSO_PEITO] = (Vector3){6.0f + curva * 0.6f, 0, 0};
    c->ang[OSSO_OMBRO_E] = (Vector3){0, 0, 12.0f * lombada(r, 3.9f, 4.45f)};
    c->ang[OSSO_OMBRO_D] = (Vector3){0, 0, -12.0f * lombada(r, 4.4f, 4.95f)};
    float agarra = 35.0f * trecho(r, 0.6f, 1.2f) * (1.0f - trecho(r, 5.9f, 6.3f));
    c->ang[OSSO_DEDOS_E] = (Vector3){agarra, 0, 0};
    c->ang[OSSO_DEDOS_D] = (Vector3){agarra, 0, 0};
}

// Embaixo do chão as pernas pendem; o joelho esquerdo sobe para a borda e depois o direito.
static void bueiro_pernas(float r, CenaAnim *c) {
    float sobe_e = trecho(r, 5.5f, 5.9f), desce_e = trecho(r, 6.0f, 6.4f);
    float sobe_d = trecho(r, 5.8f, 6.1f), desce_d = trecho(r, 6.1f, 6.45f);
    c->ang[OSSO_COXA_E] = (Vector3){-25.0f - 60.0f * sobe_e + 85.0f * desce_e, 0, 0};
    c->ang[OSSO_CANELA_E] = (Vector3){40.0f + 50.0f * sobe_e - 90.0f * desce_e, 0, 0};
    c->ang[OSSO_COXA_D] = (Vector3){-25.0f - 40.0f * sobe_d + 65.0f * desce_d, 0, 0};
    c->ang[OSSO_CANELA_D] = (Vector3){40.0f + 35.0f * sobe_d - 75.0f * desce_d, 0, 0};
}

void regras_anim_bueiro(float t, CenaAnim *c) {
    memset(c, 0, sizeof(*c));
    float r = t * BUEIRO_ROTEIRO_S / (ANIM_BUEIRO_PRE_S + ANIM_BUEIRO_SAI_S);
    bueiro_raiz_e_maos(r, c);
    bueiro_corpo(r, c);
    bueiro_pernas(r, c);
}

// ================================================================== distorção
// Puxado pela contenção: o corpo enrijece e é erguido, a cabeça vai para trás, os braços se abrem e a boca
// abre. O deslocamento não é um tremor contínuo: são saltos discretos de imagem (~9 por segundo).
void regras_anim_distorcao(float d, float t, CenaAnim *c) {
    memset(c, 0, sizeof(*c));
    int salto = (int)(t * 9.0f);
    c->raiz = (Vector3){(regras_anim_sorteio(salto, 31) * 2.0f - 1.0f) * 0.05f * d, 0.25f * d, 0.0f};
    c->ang[OSSO_LOMBAR] = (Vector3){-8.0f * d, 0, 0};
    c->ang[OSSO_PEITO] = (Vector3){-10.0f * d, 0, 0};
    c->ang[OSSO_PESCOCO] = (Vector3){-8.0f * d, 0, 0};
    c->ang[OSSO_CABECA] = (Vector3){-18.0f * d, 0, 0};
    c->ang[OSSO_MANDIBULA] = (Vector3){16.0f * d, 0, 0};
    for (int l = 0; l < 2; l++) { // padrão MAP: os dois braços se abrem, as pontas dos pés descem
        float s = LADO_SINAL(l);
        c->ang[OSSO_OMBRO(l)] = (Vector3){0, 0, 12.0f * d * s};
        c->ang[OSSO_OMBRO(l) + 1] = (Vector3){0, 0, 30.0f * d * s};
        c->ang[OSSO_OMBRO(l) + 2] = (Vector3){-15.0f * d, 0, 0};
        c->ang[OSSO_OMBRO(l) + 4] = (Vector3){-20.0f * d, 0, 0};
        c->ang[OSSO_COXA(l) + 1] = (Vector3){15.0f * d, 0, 0};
        c->ang[OSSO_COXA(l) + 2] = (Vector3){25.0f * d, 0, 0};
    }
    c->w_pes = 1.0f - 0.8f * d;
}

// ================================================================== convulsão (final bom)
// Trancos com intervalos irregulares (0,16 a 0,46 s): cada tranco escolhe uma pose nova, alcançada rápido e
// segurada até o próximo. Amplitudes limitadas; nada gira além do que um corpo gira.
static const float CONV_AMPLITUDE[8] = {8.0f, 12.0f, 16.0f, 14.0f, 18.0f, 18.0f, 25.0f, 18.0f};

void regras_anim_convulsao(AnimCriatura *a, float dt, CenaAnim *c) {
    memset(c, 0, sizeof(*c));
    c->w_pes = 1.0f;
    if (a->t >= a->conv_prox) {
        a->conv_n++;
        for (int k = 0; k < 8; k++) // padrão MAP: uma pose nova por tranco
            a->conv_alvo[k] = (regras_anim_sorteio(a->conv_n, 40 + k) * 2.0f - 1.0f) * CONV_AMPLITUDE[k];
        a->conv_prox = a->t + 0.16f + 0.3f * regras_anim_sorteio(a->conv_n, 50);
    }
    for (int k = 0; k < 8; k++) // padrão MAP: cada canal vai rápido até o alvo
        a->conv_atual[k] += (a->conv_alvo[k] - a->conv_atual[k]) * fminf(1.0f, 16.0f * dt);
    const float *v = a->conv_atual;
    c->ang[OSSO_LOMBAR] = (Vector3){-6.0f, 0, v[0]};
    c->ang[OSSO_PEITO] = (Vector3){0, v[1], 0};
    c->ang[OSSO_CABECA] = (Vector3){v[3], 0, v[2]};
    c->ang[OSSO_BRACO_E] = (Vector3){v[4], 0, 0};
    c->ang[OSSO_BRACO_D] = (Vector3){v[5], 0, 0};
    c->ang[OSSO_ANTEBRACO_E] = (Vector3){-fabsf(v[6]), 0, 0};
    c->ang[OSSO_ANTEBRACO_D] = (Vector3){-fabsf(v[6]) * 0.7f, 0, 0};
    c->ang[OSSO_MANDIBULA] = (Vector3){fabsf(v[7]), 0, 0};
}
