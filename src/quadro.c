// quadro.c — o modelo tem a frente para +Z; girado 180°, ele olha para o norte (para dentro da sala), com o
// fundo encostado na parede sul. Os papéis do modelo têm a face virada ao contrário: o quadro é desenhado sem
// descartar as faces de trás, senão eles somem de frente.
#include "quadro.h"
#include "rlgl.h"
#include "ui.h"
#include <math.h>

void quadro_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_quadro = false;
    if (!ui_caminho_asset("modelos/quadro.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/quadro.glb; sem o quadro de investigação");
        return;
    }
    rec->modelo_quadro = LoadModel(caminho);
    rec->tem_quadro = IsModelValid(rec->modelo_quadro) && rec->modelo_quadro.meshCount > 0;
    if (!rec->tem_quadro) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; sem o quadro de investigação", caminho);
        return;
    }
    for (int i = 0; i < rec->modelo_quadro.materialCount && rec->luz_ok; i++) // padrão MAP: luz do jogo
        rec->modelo_quadro.materials[i].shader = rec->luz;
}

void quadro_descarrega(Recursos *rec) {
    if (rec->tem_quadro) UnloadModel(rec->modelo_quadro);
}

void quadro_desenha(const Estado *e) {
    if (!e->rec.tem_quadro) return;
    const Model *m = &e->rec.modelo_quadro;
    BoundingBox b = GetModelBoundingBox(*m);
    float s = QUADRO_LARGURA_M / fmaxf(0.01f, b.max.x - b.min.x);
    Vector3 meio = {(b.min.x + b.max.x) * 0.5f, b.min.y, b.min.z}; // o fundo do quadro, embaixo no meio
    // girar 180° em Y leva (x, y, z) para (-x, y, -z): o fundo (z mínimo) vai para a parede, ao sul
    Vector3 pos = {QUADRO_X + meio.x * s, QUADRO_BASE_Y - meio.y * s, QUADRO_PAREDE_Z + meio.z * s};
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    DrawModelEx(*m, pos, (Vector3){0, 1, 0}, 180.0f, (Vector3){s, s, s}, WHITE);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
}
