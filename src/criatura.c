// criatura.c — o Precursor. (M2: só o placeholder contido na cela.)
#include "criatura.h"
#include <math.h>
#include <string.h>

void criatura_contem(Estado *e) {
    Criatura *c = &e->cri;
    memset(c, 0, sizeof(*c));
    c->contido = true;
    c->pos = e->nivel.cela_precursor;
    c->yaw = 0.0f; // olhando para o vidro (sul)
}

void criatura_inicia_saida(Estado *e) {
    Criatura *c = &e->cri;
    memset(c, 0, sizeof(*c));
    c->pos = e->nivel.duto;
    c->tempo_para_surgir = CRIATURA_SURGE_S;
}

void criatura_atualiza(Estado *e, float dt) {
    (void)e;
    (void)dt;
}

void criatura_desenha(const Estado *e) {
    const Criatura *c = &e->cri;
    if (!c->contido && !c->ativo) return;
    Vector3 p = c->pos;
    DrawCylinderEx((Vector3){p.x, 0.0f, p.z}, (Vector3){p.x, 2.0f, p.z}, 0.16f, 0.22f, 6, COR_PELE);
    DrawSphereEx((Vector3){p.x, 2.3f, p.z}, 0.22f, 8, 8, COR_PELE);
}

void criatura_desenha_jumpscare(const Estado *e) {
    (void)e;
}
