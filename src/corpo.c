// corpo.c — o modelo está deitado ao longo de X, que já é a direção do corredor sul: não gira. É escalado
// pelo comprimento e apoiado no chão, rente à parede sul, no fim do arrasto que sai do duto (cenario.c).
#include "corpo.h"
#include "ui.h"
#include <math.h>

void corpo_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_corpo = false;
    if (!ui_caminho_asset("modelos/corpo.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/corpo.glb; usando um volume simples");
        return;
    }
    rec->modelo_corpo = LoadModel(caminho);
    rec->tem_corpo = IsModelValid(rec->modelo_corpo) && rec->modelo_corpo.meshCount > 0;
    if (!rec->tem_corpo) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; usando um volume simples", caminho);
        return;
    }
    for (int i = 0; i < rec->modelo_corpo.materialCount && rec->luz_ok; i++) // padrão MAP: luz do jogo
        rec->modelo_corpo.materials[i].shader = rec->luz;
}

void corpo_descarrega(Recursos *rec) {
    if (rec->tem_corpo) UnloadModel(rec->modelo_corpo);
}

void corpo_desenha(const Estado *e) {
    if (!e->rec.tem_corpo) {
        DrawCubeV((Vector3){CORPO_X, 0.1f, CORPO_Z}, (Vector3){CORPO_COMPRIMENTO_M, 0.2f, 0.7f},
                  (Color){0x1c, 0x1e, 0x1a, 255});
        return;
    }
    const Model *m = &e->rec.modelo_corpo;
    BoundingBox b = GetModelBoundingBox(*m);
    float s = CORPO_COMPRIMENTO_M / fmaxf(0.01f, b.max.x - b.min.x);
    Vector3 pos = {CORPO_X - (b.min.x + b.max.x) * 0.5f * s, -b.min.y * s,
                   CORPO_Z - (b.min.z + b.max.z) * 0.5f * s};
    DrawModelEx(*m, pos, (Vector3){0, 1, 0}, 0.0f, (Vector3){s, s, s}, WHITE);
}
