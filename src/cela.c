// cela.c — o Precursor dentro da cela: sempre no centro, olhando o vidro; mais perto dele depois da R1 e
// com a mão no vidro no fim da R3. A lanterna não o move: ela falha quando o ilumina (lanterna.c).
#include "cela.h"

// O centro de sempre: perto do vidro depois da R1, e com a mão no vidro no fim da R3.
static Vector3 centro_da_cela(const Estado *e) {
    const Criatura *c = &e->cri;
    Vector3 base = e->nivel.cela_precursor;
    return (Vector3){base.x, 0.0f, base.z + 1.3f * c->avanco_vidro + (c->mao_no_vidro ? 0.15f : 0.0f)};
}

void cela_atualiza(Estado *e, float dt) {
    e->cri.passo_t += dt;
    e->cri.pos = centro_da_cela(e);
}
