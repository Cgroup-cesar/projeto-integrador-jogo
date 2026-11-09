// glifos.c — os glifos da língua do Precursor e o desenho deles (no terminal e projetados no vidro).
// Os traços foram desenhados para NÃO lembrar os símbolos lógicos reais (não entregam a resposta).
#include "glifos.h"
#include "rodadas.h"
#include <math.h>

typedef struct {
    signed char x1, y1, x2, y2;
} Traco;

typedef struct {
    Traco tracos[5];
    int n;
} Glifo;

static const Glifo GLIFOS[GLIFO_TOTAL] = {
    [GLIFO_NAO] = {{{2, 0, 2, 4}, {0, 1, 4, 1}, {0, 4, 2, 4}}, 3},
    [GLIFO_E] = {{{0, 2, 2, 0}, {2, 0, 4, 2}, {2, 0, 2, 4}, {1, 4, 3, 4}}, 4},
    [GLIFO_OU] = {{{0, 1, 4, 1}, {4, 1, 4, 0}, {2, 1, 2, 4}, {0, 4, 4, 4}}, 4},
    [GLIFO_SE_ENTAO] = {{{0, 0, 4, 4}, {0, 4, 2, 2}, {4, 0, 4, 2}, {2, 4, 4, 4}}, 4},
    [GLIFO_SSE] = {{{0, 0, 2, 4}, {2, 4, 4, 0}, {0, 4, 2, 0}, {2, 0, 4, 4}}, 4},
    [GLIFO_LUZ_A] = {{{0, 0, 4, 0}, {2, 0, 2, 4}, {1, 4, 3, 4}}, 3},
    [GLIFO_LUZ_B] = {{{0, 4, 4, 4}, {2, 0, 2, 4}, {1, 0, 3, 0}, {0, 2, 2, 2}}, 4},
    [GLIFO_NOS] = {{{0, 0, 0, 4}, {4, 0, 4, 4}, {0, 2, 4, 2}, {2, 2, 2, 4}}, 4},
    [GLIFO_VOCE] = {{{0, 0, 4, 0}, {4, 0, 0, 4}, {0, 4, 4, 4}}, 3},
    [GLIFO_PERIGO] = {{{2, 0, 0, 4}, {2, 0, 4, 4}, {0, 4, 4, 4}, {2, 2, 2, 3}}, 4},
    [GLIFO_FALAR] = {{{0, 1, 4, 1}, {0, 3, 4, 3}, {2, 1, 2, 3}, {0, 0, 0, 1}, {4, 3, 4, 4}}, 5},
    [GLIFO_PORTA] = {{{0, 0, 0, 4}, {4, 0, 4, 4}, {0, 0, 4, 0}, {3, 2, 3, 3}}, 4},
    [GLIFO_ABRIR] = {{{0, 0, 2, 2}, {2, 2, 0, 4}, {4, 0, 4, 4}, {2, 2, 4, 2}}, 4},
};

void desenha_glifo(IdGlifo id, Vector2 pos, float tamanho, Color cor, float espessura) {
    if (id < 0 || id >= GLIFO_TOTAL) return;
    float passo = tamanho / 4.0f;
    const Glifo *g = &GLIFOS[id];
    for (int i = 0; i < g->n; i++) {
        Vector2 a = {pos.x + g->tracos[i].x1 * passo, pos.y + g->tracos[i].y1 * passo};
        Vector2 b = {pos.x + g->tracos[i].x2 * passo, pos.y + g->tracos[i].y2 * passo};
        DrawLineEx(a, b, espessura, cor);
    }
}

int glifo_conectivo(IdGlifo id) {
    switch (id) {
    case GLIFO_NAO: return 0;
    case GLIFO_E: return 1;
    case GLIFO_OU: return 2;
    case GLIFO_SE_ENTAO: return 3;
    default: return -1;
    }
}

Color glifo_cor(const Estado *e, IdGlifo id) {
    int c = glifo_conectivo(id);
    if (c >= 0 && !e->conectivo_decifrado[c]) return COR_AMBAR;
    return COR_FOSFORO;
}

void desenha_frase_glifos(const Estado *e, const FraseGlifos *frase, Vector2 pos, float tamanho, float espessura) {
    for (int i = 0; i < frase->n; i++) {
        Vector2 p = {pos.x + i * tamanho * 1.45f, pos.y};
        desenha_glifo(frase->glifos[i], p, tamanho, glifo_cor(e, frase->glifos[i]), espessura);
    }
}

// ------------------------------------------------------------------ projeção no vidro
// Um traço em 3D é um cilindro fino; o glifo fica num plano logo atrás do vidro (lado da cela).
static void glifo_no_vidro(IdGlifo id, Vector3 canto, float tamanho, Color cor, float tremor) {
    float passo = tamanho / 4.0f;
    const Glifo *g = &GLIFOS[id];
    for (int i = 0; i < g->n; i++) {
        Vector3 a = {canto.x + g->tracos[i].x1 * passo + tremor, canto.y - g->tracos[i].y1 * passo, canto.z};
        Vector3 b = {canto.x + g->tracos[i].x2 * passo - tremor, canto.y - g->tracos[i].y2 * passo, canto.z};
        DrawCylinderEx(a, b, 0.018f, 0.018f, 4, cor);
    }
}

void glifos_atualiza(Estado *e, float dt) {
    if (e->cri.fala_t <= 0.0f || GetRandomValue(0, 100) > 35) return;
    const Nivel *n = &e->nivel;
    float x = n->vidro_x0 + 3.0f + GetRandomValue(0, 600) / 100.0f;
    Vector3 pos = {x, 1.6f + GetRandomValue(0, 100) / 100.0f, n->vidro_z - 0.05f};
    Vector3 vel = {GetRandomValue(-50, 50) / 100.0f, GetRandomValue(0, 80) / 100.0f, GetRandomValue(10, 60) / 100.0f};
    col_particulas_adiciona(&e->particulas, (Particula){pos, vel, 0.6f + dt, 0.6f, PARTICULA_FAGULHA});
}

// Enquanto ele "fala", a frase aparece no vidro, tremendo levemente (até duas frases, uma por linha).
void glifos_desenha(const Estado *e) {
    FraseGlifos frases[2];
    int n_frases = e->cri.fala_t > 0.0f ? rodadas_frases_atuais(e, frases, 2) : 0;
    const Nivel *n = &e->nivel;
    float tamanho = 0.36f;
    for (int f = 0; f < n_frases; f++) {
        const FraseGlifos *frase = &frases[f];
        float largura = frase->n * tamanho * 1.45f - tamanho * 0.45f;
        float x0 = (n->vidro_x0 + n->vidro_x1) * 0.5f - largura * 0.5f;
        float topo = (n_frases > 1 ? 2.75f : 2.55f) - f * 0.62f;
        for (int i = 0; i < frase->n; i++) {
            float tremor = sinf(e->tempo_total * 23.0f + i * 1.7f + f) * 0.012f;
            Vector3 canto = {x0 + i * tamanho * 1.45f, topo + tremor, n->vidro_z - 0.06f};
            glifo_no_vidro(frase->glifos[i], canto, tamanho, glifo_cor(e, frase->glifos[i]), tremor);
        }
    }
}
