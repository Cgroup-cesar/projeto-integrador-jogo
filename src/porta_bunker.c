// porta_bunker.c — o modelo vem com a folha entreaberta ~59° e o batente numa peça só com ela. Ao carregar,
// cada triângulo vai para o batente (atrás do plano z = 0 do modelo) ou para a folha (na frente: a chapa,
// o volante e as travas: ver da_folha), e as duas viram malhas próprias. A folha gira em torno da dobradiça.
// O modelo olha para +Z (o lado para onde a folha abre); girado 90° em Y, ele abre para o corredor (+X).
#include "porta_bunker.h"
#include "porta_sala.h"
#include "ui.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Medidas do modelo (unidades dele), tiradas dos vértices: o batente vai de y -160 a 100, centrado em x = 0,
// com a chapa em z ≈ -19; a dobradiça fica em x = 62, z = 4.
#define BATENTE_BASE_Y  -160.0f
#define BATENTE_ALTURA  260.0f
#define BATENTE_MEIO_Z  -19.0f
#define DOBRADICA_X     62.0f
#define DOBRADICA_Z     4.0f
#define FOLHA_ESPESSURA 30.0f  // meia espessura da folha com o volante e as travas
#define FOLHA_FECHADA   -59.0f // graus: leva a folha da posição do modelo até fechada no batente
#define FOLHA_ABERTA    31.0f  // graus: 90° depois de fechada, encostada na parede norte do corredor

// Um triângulo é da folha se está perto do plano dela (como veio no modelo: aberta, saindo da dobradiça na
// direção (-0,52; 0,86)) e na frente do batente. O resto (batente, trincos e suportes presos nele) fica
// parado. Só "na frente do batente" não bastava: os trincos do batente iam junto e boiavam quando a folha
// girava.
static bool da_folha(const Mesh *m, int t) {
    float cx = 0.0f, cz = 0.0f;
    for (int k = 0; k < 3; k++) {
        int i = m->indices ? m->indices[3 * t + k] : 3 * t + k;
        cx += m->vertices[3 * i] / 3.0f;
        cz += m->vertices[3 * i + 2] / 3.0f;
    }
    if (cz <= 0.0f) return false;
    float dx = cx - DOBRADICA_X, dz = cz - DOBRADICA_Z;
    float distancia = fabsf(0.86f * dx + 0.52f * dz), ao_longo = -0.52f * dx + 0.86f * dz;
    return distancia < FOLHA_ESPESSURA && ao_longo > -12.0f;
}

// Copia só os triângulos de uma das peças para uma malha nova, sem índices, e manda para a GPU.
static Mesh separa(const Mesh *m, bool frente) {
    int total = 0;
    for (int t = 0; t < m->triangleCount; t++) { // padrão FILTER: conta os triângulos daquele lado
        total += da_folha(m, t) == frente;
    }
    Mesh r = {0};
    r.vertexCount = total * 3;
    r.triangleCount = total;
    r.vertices = MemAlloc(r.vertexCount * 3 * sizeof(float));
    r.texcoords = m->texcoords ? MemAlloc(r.vertexCount * 2 * sizeof(float)) : NULL;
    r.normals = m->normals ? MemAlloc(r.vertexCount * 3 * sizeof(float)) : NULL;
    int n = 0;
    for (int t = 0; t < m->triangleCount; t++) { // padrão FILTER: copia os triângulos daquele lado
        if (da_folha(m, t) != frente) continue;
        int idx[3];
        for (int k = 0; k < 3; k++)
            idx[k] = m->indices ? m->indices[3 * t + k] : 3 * t + k;
        for (int k = 0; k < 3; k++, n++) {
            memcpy(&r.vertices[3 * n], &m->vertices[3 * idx[k]], 3 * sizeof(float));
            if (r.texcoords) memcpy(&r.texcoords[2 * n], &m->texcoords[2 * idx[k]], 2 * sizeof(float));
            if (r.normals) memcpy(&r.normals[3 * n], &m->normals[3 * idx[k]], 3 * sizeof(float));
        }
    }
    UploadMesh(&r, false);
    return r;
}

void porta_bunker_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_porta = false;
    if (!ui_caminho_asset("modelos/porta_bunker.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/porta_bunker.glb; usando a porta de correr");
        return;
    }
    rec->modelo_porta = LoadModel(caminho);
    if (!IsModelValid(rec->modelo_porta) || rec->modelo_porta.meshCount == 0) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; usando a porta de correr", caminho);
        return;
    }
    rec->porta_batente = separa(&rec->modelo_porta.meshes[0], false);
    rec->porta_folha = separa(&rec->modelo_porta.meshes[0], true);
    for (int i = 0; i < rec->modelo_porta.materialCount && rec->luz_ok; i++) // padrão MAP: luz do jogo
        rec->modelo_porta.materials[i].shader = rec->luz;
    rec->tem_porta = true;
}

void porta_bunker_descarrega(Recursos *rec) {
    if (!rec->tem_porta) return;
    UnloadMesh(rec->porta_batente);
    UnloadMesh(rec->porta_folha);
    UnloadModel(rec->modelo_porta);
}

bool porta_bunker_desenha(const Estado *e) {
    if (!e->rec.tem_porta) return false;
    const Model *m = &e->rec.modelo_porta;
    Material mat = m->materials[m->meshMaterial[0]];
    float s = PORTA_ALTURA_M / BATENTE_ALTURA;
    Vector3 p = e->nivel.porta_sala;
    Matrix lugar = MatrixMultiply(
        MatrixMultiply(MatrixTranslate(0.0f, -BATENTE_BASE_Y, -BATENTE_MEIO_Z), MatrixScale(s, s, s)),
        MatrixMultiply(MatrixRotateY(90.0f * DEG2RAD), MatrixTranslate(p.x, 0.0f, p.z)));
    DrawMesh(e->rec.porta_batente, mat, lugar);
    float giro = FOLHA_FECHADA + (FOLHA_ABERTA - FOLHA_FECHADA) * porta_sala_abertura(e);
    Matrix dobradica = MatrixMultiply(
        MatrixMultiply(MatrixTranslate(-DOBRADICA_X, 0.0f, -DOBRADICA_Z), MatrixRotateY(giro * DEG2RAD)),
        MatrixTranslate(DOBRADICA_X, 0.0f, DOBRADICA_Z));
    DrawMesh(e->rec.porta_folha, mat, MatrixMultiply(dobradica, lugar));
    return true;
}
