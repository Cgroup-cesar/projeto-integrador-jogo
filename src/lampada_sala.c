// lampada_sala.c — a lâmpada pendurada da sala de observação, no lugar da luminária de caixa que ficava
// em cima de onde o jogador começa. O modelo é escalado pela altura (do gancho ao fundo da gaiola), com o
// topo encostado no teto; a escala e a posição são calculadas uma vez, ao carregar.
#include "lampada_sala.h"
#include "ui.h"
#include "rlgl.h"
#include <math.h>

void lampada_sala_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_lampada = false;
    if (!ui_caminho_asset("modelos/lampada.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/lampada.glb; usando a lâmpada simples");
        return;
    }
    rec->modelo_lampada = LoadModel(caminho);
    rec->tem_lampada = IsModelValid(rec->modelo_lampada) && rec->modelo_lampada.meshCount > 0;
    if (!rec->tem_lampada) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; usando a lâmpada simples", caminho);
        return;
    }
    for (int i = 0; i < rec->modelo_lampada.materialCount && rec->luz_ok; i++) // padrão MAP: luz do jogo
        rec->modelo_lampada.materials[i].shader = rec->luz;
    BoundingBox caixa = GetModelBoundingBox(rec->modelo_lampada);
    float escala = LAMPADA_SALA_ALTURA_M / fmaxf(0.001f, caixa.max.y - caixa.min.y);
    Vector3 centro = Vector3Scale(Vector3Add(caixa.min, caixa.max), 0.5f);
    rec->lampada_escala = escala;
    rec->lampada_pos = (Vector3){LAMPADA_SALA_X - centro.x * escala, PE_DIREITO - caixa.max.y * escala,
                                 LAMPADA_SALA_Z - centro.z * escala};
}

void lampada_sala_descarrega(Recursos *rec) {
    if (rec->tem_lampada) UnloadModel(rec->modelo_lampada);
}

Vector3 lampada_sala_bulbo(void) {
    return (Vector3){LAMPADA_SALA_X, PE_DIREITO - LAMPADA_SALA_ALTURA_M * LAMPADA_SALA_BULBO, LAMPADA_SALA_Z};
}

// Sem o modelo: o fio, a cúpula de metal pintado e um aro em volta do bulbo.
static void desenha_simples(void) {
    Vector3 b = lampada_sala_bulbo();
    Vector3 topo_cupula = {b.x, b.y + 0.16f, b.z}, boca = {b.x, b.y - 0.02f, b.z};
    Color metal = {0x8a, 0x90, 0x84, 255};
    DrawCylinderEx((Vector3){b.x, PE_DIREITO, b.z}, topo_cupula, 0.006f, 0.006f, 4,
                   (Color){20, 20, 18, 255}); // o fio
    DrawCylinderEx(topo_cupula, boca, 0.035f, 0.13f, 12, metal);
    DrawCylinderWiresEx((Vector3){b.x, b.y - 0.09f, b.z}, boca, 0.07f, 0.1f, 8, metal); // a gaiola
}

// O vidro do modelo vem transparente de todo (cor com alfa 0 e "transmissão", que a raylib não lê): desenhado
// como veio, ele não aparece, mas escreve profundidade e esconde o bulbo aceso atrás dele.
static bool eh_vidro(const Model *m, int malha) {
    return m->materials[m->meshMaterial[malha]].maps[MATERIAL_MAP_DIFFUSE].color.a == 0;
}

void lampada_sala_desenha(const Estado *e) {
    if (!e->rec.tem_lampada) {
        desenha_simples();
        return;
    }
    const Model *m = &e->rec.modelo_lampada;
    float s = e->rec.lampada_escala;
    Vector3 p = e->rec.lampada_pos;
    Matrix t =
        MatrixMultiply(m->transform, MatrixMultiply(MatrixScale(s, s, s), MatrixTranslate(p.x, p.y, p.z)));
    for (int i = 0; i < m->meshCount; i++) // laço de desenho: as peças opacas
        if (!eh_vidro(m, i)) DrawMesh(m->meshes[i], m->materials[m->meshMaterial[i]], t);
    rlDisableDepthMask();                    // o vidro por último, com um tom fraco e sem esconder o bulbo
    for (int i = 0; i < m->meshCount; i++) { // laço de desenho: só o vidro
        if (!eh_vidro(m, i)) continue;
        Material vidro = m->materials[m->meshMaterial[i]];
        vidro.maps[MATERIAL_MAP_DIFFUSE].color = LAMPADA_SALA_COR_VIDRO;
        DrawMesh(m->meshes[i], vidro, t);
    }
    rlEnableDepthMask();
}

void lampada_sala_desenha_bulbo(Color cor, float forca) {
    if (forca <= 0.0f) return;
    DrawSphereEx(lampada_sala_bulbo(), LAMPADA_SALA_BULBO_RAIO, 10, 10, ColorBrightness(cor, forca - 1.0f));
}
