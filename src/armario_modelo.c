// armario_modelo.c — o armário de metal de duas portas. O modelo tem a porta para +Z; ele gira para a porta
// olhar para o corredor, é escalado pela altura da caixa de colisão (2,1 m) e encosta a frente na face da
// caixa, para as frestas desenhadas em nivel.c caírem em cima da porta.
#include "armario_modelo.h"
#include "ui.h"
#include "posfx.h"
#include <math.h>

void armario_modelo_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_armario = false;
    if (!ui_caminho_asset("modelos/armario.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/armario.glb; usando os armários de caixa");
        return;
    }
    rec->modelo_armario = LoadModel(caminho);
    rec->tem_armario = IsModelValid(rec->modelo_armario) && rec->modelo_armario.meshCount > 0;
    if (!rec->tem_armario) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; usando os armários de caixa", caminho);
        return;
    }
    for (int i = 0; i < rec->modelo_armario.materialCount && rec->luz_ok; i++) // padrão MAP: luz do jogo
        rec->modelo_armario.materials[i].shader = rec->luz;
}

void armario_modelo_descarrega(Recursos *rec) {
    if (rec->tem_armario) UnloadModel(rec->modelo_armario);
}

bool armario_modelo_desenha(const Estado *e, Vector3 centro, Vector3 tam, Vector3 frente) {
    if (!e->rec.tem_armario) return false;
    const Model *m = &e->rec.modelo_armario;
    BoundingBox b = GetModelBoundingBox(*m);
    float s = tam.y / fmaxf(0.01f, b.max.y - b.min.y);
    float fundo = (b.max.z - b.min.z) * s; // profundidade do modelo já escalado
    float meia =
        fabsf(frente.x) > fabsf(frente.z) ? tam.x * 0.5f : tam.z * 0.5f; // meia profundidade da caixa
    Vector3 alvo = Vector3Add(centro, Vector3Scale(frente, meia - fundo * 0.5f)); // a frente rente à caixa
    float giro = atan2f(frente.x, frente.z); // leva o +Z do modelo para `frente`
    Vector3 meio = {(b.min.x + b.max.x) * 0.5f, b.min.y, (b.min.z + b.max.z) * 0.5f};
    Vector3 desloca = Vector3RotateByAxisAngle(Vector3Scale(meio, s), (Vector3){0, 1, 0}, giro);
    Vector3 pos = {alvo.x - desloca.x, -desloca.y, alvo.z - desloca.z};
    // aço pintado de verdade: os mapas do metal do jogo (lascas, ferrugem, riscos, amassados) projetados pela
    // posição no mundo, porque a UV do modelo não combina com as texturas do jogo
    posfx_material_triplanar(e, MATERIAL_METAL, 0.9f);
    DrawModelEx(*m, pos, (Vector3){0, 1, 0}, giro * RAD2DEG, (Vector3){s, s, s},
                (Color){0xa8, 0xb0, 0xa0, 255}); // um pouco mais escuro: a lanterna estourava o metal claro
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
    return true;
}
