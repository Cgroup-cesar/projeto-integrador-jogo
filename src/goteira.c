// goteira.c — o cano pingando na sala do gerador (ver goteira.h). O ritmo das gotas sai do tempo do jogo
// (sem estado além do último pingo tocado): o desenho e o som concordam sem combinar nada.
#include "goteira.h"
#include "audio_sintese.h"
#include "rlgl.h"
#include <math.h>

#define PAREDE_X   44.0f // a face da parede leste da sala do gerador
#define CANO_Y     1.95f // altura do trecho que sai da parede (na altura da lanterna, para ser visto)
#define BOCA_Y     1.72f // a boca do cano, virada para baixo: de onde a gota cai
#define CANO_RAIO  0.055f
#define GOTA_RAIO  0.02f
#define ONDA_LARGO 0.012f // espessura do anel da onda
#define SEMENTE    0x601au

static const Color FERRUGEM = {0x4a, 0x2e, 0x20, 255};
static const Color FERRUGEM_ESCURA = {0x2c, 0x22, 0x1c, 255};
static const Color AGUA = {0x9c, 0xb2, 0xb6, 210};

// Quando a gota do intervalo n chega na poça: sorteado dentro do intervalo (o pingo não soa como relógio).
static float impacto(int n) {
    return ((float)n + 0.35f + 0.3f * regras_sorteio(n, SEMENTE)) * GOTA_INTERVALO_S;
}

// A última gota que já chegou até `t` (a deste intervalo, se já caiu; senão a do anterior).
static int ultima_gota(float t) {
    int n = (int)floorf(t / GOTA_INTERVALO_S);
    return t >= impacto(n) ? n : n - 1;
}

void goteira_atualiza(Estado *e, float dt) {
    (void)dt;
    if (!regras_fase_eh_saida(e->fase)) return;
    int n = ultima_gota(e->tempo_total);
    if (n == e->goteira_gota) return;
    bool fresca = e->tempo_total - impacto(n) < 0.1f; // depois de uma pausa, não toca as gotas perdidas
    e->goteira_gota = n;
    if (!fresca) return;
    float tom = 0.85f + 0.3f * regras_sorteio(n, SEMENTE + 1u);
    audio_toca_em(e, SOM_GOTA, (Vector3){GOTEIRA_X, 0.0f, GOTEIRA_Z}, GOTA_ALCANCE_M, tom);
}

// ------------------------------------------------------------------ desenho
static void desenha_cano(void) {
    Vector3 parede = {PAREDE_X, CANO_Y, GOTEIRA_Z}, cotovelo = {GOTEIRA_X, CANO_Y, GOTEIRA_Z};
    DrawCylinderEx(parede, (Vector3){PAREDE_X - 0.04f, CANO_Y, GOTEIRA_Z}, 0.1f, 0.1f, 12,
                   FERRUGEM_ESCURA); // flange na parede
    DrawCylinderEx(parede, cotovelo, CANO_RAIO, CANO_RAIO, 10, FERRUGEM);
    DrawSphereEx(cotovelo, CANO_RAIO, 8, 8, FERRUGEM);
    Vector3 desce = {GOTEIRA_X, BOCA_Y + 0.04f, GOTEIRA_Z};
    DrawCylinderEx(cotovelo, desce, CANO_RAIO, CANO_RAIO, 10, FERRUGEM);
    DrawCylinderEx((Vector3){GOTEIRA_X, BOCA_Y + 0.05f, GOTEIRA_Z}, (Vector3){GOTEIRA_X, BOCA_Y, GOTEIRA_Z},
                   CANO_RAIO + 0.008f, CANO_RAIO + 0.008f, 10, FERRUGEM_ESCURA); // a boca, mais grossa
}

// A próxima gota: incha na boca do cano e cai acelerando (a queda vai de BOCA_Y ao chão em GOTA_QUEDA_S).
static void desenha_gota(float t) {
    float falta = impacto(ultima_gota(t) + 1) - t;
    Vector3 p = {GOTEIRA_X, BOCA_Y - GOTA_RAIO, GOTEIRA_Z};
    float raio = GOTA_RAIO;
    if (falta < GOTA_QUEDA_S) {
        float caiu = 1.0f - falta / GOTA_QUEDA_S;
        p.y = (BOCA_Y - GOTA_RAIO) * (1.0f - caiu * caiu);
    } else {
        raio *= fminf(1.0f, 0.35f + 0.65f * (GOTA_INCHA_S - (falta - GOTA_QUEDA_S)) / GOTA_INCHA_S);
        p.y = BOCA_Y - raio;
    }
    DrawSphereEx(p, raio, 6, 6, AGUA);
}

// Um anel achatado no chão (quadriláteros, não linhas: o shader de luz precisa de uma face para a normal).
static void desenha_anel(float raio, Color cor) {
    const int lados = 20;
    rlBegin(RL_QUADS);
    rlColor4ub(cor.r, cor.g, cor.b, cor.a);
    rlNormal3f(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < lados; i++) { // laço de desenho: cada lado do anel vira um quadrilátero
        float a0 = 2.0f * PI * i / lados, a1 = 2.0f * PI * (i + 1) / lados;
        float r0 = raio, r1 = raio + ONDA_LARGO, y = 0.014f;
        rlVertex3f(GOTEIRA_X + cosf(a0) * r0, y, GOTEIRA_Z + sinf(a0) * r0);
        rlVertex3f(GOTEIRA_X + cosf(a0) * r1, y, GOTEIRA_Z + sinf(a0) * r1);
        rlVertex3f(GOTEIRA_X + cosf(a1) * r1, y, GOTEIRA_Z + sinf(a1) * r1);
        rlVertex3f(GOTEIRA_X + cosf(a1) * r0, y, GOTEIRA_Z + sinf(a1) * r0);
    }
    rlEnd();
}

// As ondas das duas últimas gotas: dois anéis por gota, abrindo e sumindo em GOTA_ONDA_S.
static void desenha_ondas(float t) {
    int n = ultima_gota(t);
    for (int k = n - 1; k <= n; k++) { // laço de desenho: a gota anterior ainda pode ter onda à mostra
        float idade = t - impacto(k);
        for (int anel = 0; anel < 2; anel++) { // laço de desenho: o segundo anel sai um pouco depois
            float vida = (idade - anel * 0.18f) / GOTA_ONDA_S;
            if (vida <= 0.0f || vida >= 1.0f) continue;
            float raio = GOTA_ONDA_RAIO_M * sqrtf(vida); // abre rápido e vai freando
            desenha_anel(raio, Fade(AGUA, 0.55f * (1.0f - vida) * (anel == 0 ? 1.0f : 0.6f)));
        }
    }
}

// [REQ:GOTEIRA] cano enferrujado na parede leste da sala do gerador, a gota caindo e as ondas na poça
void goteira_desenha(const Estado *e) {
    desenha_cano();
    desenha_gota(e->tempo_total);
    rlDrawRenderBatchActive();
    rlDisableDepthMask(); // as ondas ficam por cima da poça (também sem profundidade) sem brigar com ela
    desenha_ondas(e->tempo_total);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}
