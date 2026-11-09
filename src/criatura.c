// criatura.c — o Precursor: corpo procedural, a cela, o susto e (nas saídas) a IA.
#include "criatura.h"
#include "audio_sintese.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>

// ================================================================== estados iniciais
void criatura_contem(Estado *e) {
    Criatura *c = &e->cri;
    memset(c, 0, sizeof(*c));
    c->contido = true;
    c->pos = e->nivel.cela_precursor;
    c->yaw = 0.0f; // olhando para o vidro (sul, +z)
}

void criatura_inicia_saida(Estado *e) {
    Criatura *c = &e->cri;
    memset(c, 0, sizeof(*c));
    c->pos = e->nivel.duto;
    c->tempo_para_surgir = CRIATURA_SURGE_S;
}

// ================================================================== susto (jumpscare)
void criatura_susto(Estado *e, EventoFase depois) {
    if (criatura_em_susto(e)) return;
    e->silencio_t = JUMPSCARE_SILENCIO_S; // 0,2 s de silêncio antes do estrondo
    e->jumpscare_t = JUMPSCARE_S;
    e->evento_pos_susto = depois;
    audio_silencia(e);
}

bool criatura_em_susto(const Estado *e) {
    return e->silencio_t > 0.0f || e->jumpscare_t > 0.0f;
}

void criatura_atualiza_susto(Estado *e, float dt) {
    if (e->silencio_t > 0.0f) {
        e->silencio_t -= dt;
        if (e->silencio_t <= 0.0f) audio_toca(e, SOM_ESTRONDO, 1.0f, 1.0f);
        return;
    }
    if (e->jumpscare_t <= 0.0f) return;
    e->jumpscare_t -= dt;
    if (e->jumpscare_t > 0.0f) return;
    e->jumpscare_t = 0.0f;
    if (e->evento_pos_susto != EV_NENHUM) e->evento = e->evento_pos_susto;
    e->evento_pos_susto = EV_NENHUM;
}

// ================================================================== na cela
static void atualiza_contido(Estado *e, float dt) {
    Criatura *c = &e->cri;
    Vector3 base = e->nivel.cela_precursor;
    float alvo_z = base.z + 1.3f * c->avanco_vidro + (c->mao_no_vidro ? 0.15f : 0.0f);
    c->pos = (Vector3){base.x, 0.0f, alvo_z};
    c->passo_t += dt;
}

void criatura_atualiza(Estado *e, float dt) {
    Criatura *c = &e->cri;
    c->fala_t = fmaxf(0.0f, c->fala_t - dt);
    if (c->contido) atualiza_contido(e, dt);
}

// ================================================================== corpo procedural
// Humanoide de ~2,6 m: magro demais, cabeça elipsoide, dedos longos. Move-se com pausas e trancos.
static void membro(Vector3 a, Vector3 b, float raio, Color cor) {
    DrawCylinderEx(a, b, raio, raio * 0.8f, 5, cor);
}

static void mao(Vector3 pulso, Vector3 direcao, Color cor) {
    for (int d = -1; d <= 1; d++) {
        Vector3 ponta = Vector3Add(pulso, Vector3Add(Vector3Scale(direcao, 0.32f), (Vector3){d * 0.05f, -0.02f, 0}));
        membro(pulso, ponta, 0.012f, cor);
    }
}

static void cabeca(float altura, float fenda, Color cor) {
    rlPushMatrix();
    rlTranslatef(0.0f, altura, 0.0f);
    rlScalef(0.16f, 0.25f, 0.18f);
    DrawSphereEx((Vector3){0, 0, 0}, 1.0f, 8, 10, cor);
    rlPopMatrix();
    // A fenda vertical no rosto liso: se abre quando ele "fala".
    DrawCubeV((Vector3){0.0f, altura - 0.02f, 0.175f}, (Vector3){0.012f + fenda * 0.05f, 0.26f, 0.02f}, (Color){18, 4, 6, 255});
}

// Brilho do corpo na cela: com as luzes A e B apagadas, ele é quase invisível.
static float visibilidade(const Estado *e) {
    if (!e->cri.contido) return 1.0f;
    if (e->apagao_t > 0.0f) return 0.05f;
    bool menu = e->fase == FASE_MENU || e->fase == FASE_INTRO;
    float v = 0.12f + (menu || e->rod.luz_a ? 0.4f : 0.0f) + (e->rod.luz_b ? 0.6f : 0.0f);
    if (e->fase == FASE_RESPOSTA_FINAL) v = 1.0f;
    return fminf(1.0f, v);
}

static Color escurece(Color c, float f) {
    return (Color){(unsigned char)(c.r * f), (unsigned char)(c.g * f), (unsigned char)(c.b * f), c.a};
}

static void desenha_corpo(const Estado *e, Color pele) {
    const Criatura *c = &e->cri;
    float t = e->tempo_total;
    float tranco = (fmodf(t * 0.7f, 1.0f) > 0.85f) ? sinf(t * 40.0f) * 0.04f : 0.0f; // pausa... tranco
    float cabeca_y = 2.42f + sinf(t * 1.3f) * 0.015f + tranco;
    membro((Vector3){-0.1f, 0, 0}, (Vector3){-0.09f, 1.25f, 0}, 0.05f, pele);
    membro((Vector3){0.1f, 0, 0}, (Vector3){0.09f, 1.25f, 0}, 0.05f, pele);
    membro((Vector3){0, 1.2f, 0}, (Vector3){0, 2.05f, 0}, 0.13f, pele);
    membro((Vector3){0, 2.0f, 0}, (Vector3){0, 2.22f, 0}, 0.04f, pele);
    Vector3 ombro_e = {-0.2f, 2.0f, 0}, ombro_d = {0.2f, 2.0f, 0};
    Vector3 pulso_e = {-0.26f, 0.85f, 0.05f}, pulso_d = {0.26f, 0.85f, 0.05f};
    if (c->mao_no_vidro) pulso_d = (Vector3){0.3f, 1.75f, 0.25f}; // a mão encostada no vidro
    membro(ombro_e, pulso_e, 0.035f, pele);
    membro(ombro_d, pulso_d, 0.035f, pele);
    mao(pulso_e, (Vector3){0, -1, 0}, pele);
    mao(pulso_d, c->mao_no_vidro ? (Vector3){0, 0.3f, 1} : (Vector3){0, -1, 0}, pele);
    cabeca(cabeca_y, c->fala_t > 0.0f ? 0.6f + 0.4f * sinf(t * 18.0f) : 0.0f, pele);
}

void criatura_desenha(const Estado *e) {
    const Criatura *c = &e->cri;
    if (!c->contido && !c->ativo) return;
    float t = e->tempo_total;
    float jitter = (fmodf(t * 0.9f, 1.0f) > 0.9f) ? sinf(t * 55.0f) * 4.0f : sinf(t * 0.8f) * 1.5f;
    Color pele = escurece(COR_PELE, visibilidade(e));
    rlPushMatrix();
    rlTranslatef(c->pos.x, c->pos.y, c->pos.z);
    rlRotatef(c->yaw * RAD2DEG + jitter, 0.0f, 1.0f, 0.0f);
    desenha_corpo(e, pele);
    rlPopMatrix();
}

// ================================================================== o rosto na câmera
void criatura_desenha_jumpscare(const Estado *e) {
    if (e->jumpscare_t <= 0.0f || e->silencio_t > 0.0f) return;
    float w = (float)e->rec.alvo_largura, h = (float)e->rec.alvo_altura;
    float t = 1.0f - e->jumpscare_t / JUMPSCARE_S;
    float tremor = sinf(e->tempo_total * 95.0f) * w * 0.012f;
    DrawRectangle(0, 0, (int)w, (int)h, (Color){4, 2, 2, 235});
    int cx = (int)(w * 0.5f + tremor), cy = (int)(h * 0.56f);
    DrawEllipse(cx, cy, w * (0.26f + t * 0.08f), h * (0.72f + t * 0.1f), escurece(COR_PELE, 0.55f));
    DrawEllipse(cx, cy, w * (0.02f + t * 0.06f), h * 0.42f, (Color){24, 2, 6, 255}); // a fenda aberta
    for (int i = -3; i <= 3; i++)
        DrawLineEx((Vector2){cx + i * w * 0.008f, cy - h * 0.3f}, (Vector2){cx + i * w * 0.006f, cy + h * 0.3f}, 1.2f, (Color){90, 20, 24, 200});
    if (t < 0.15f) DrawRectangle(0, 0, (int)w, (int)h, Fade(WHITE, 0.6f * (1.0f - t / 0.15f)));
}
