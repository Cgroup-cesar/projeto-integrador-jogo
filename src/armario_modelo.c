// armario_modelo.c — o armário de metal de duas portas. O modelo tem a porta para +Z; ele gira para a porta
// olhar para o corredor, é escalado pela altura da caixa de colisão (2,1 m) e encosta a frente na face da
// caixa, para as frestas desenhadas em nivel.c caírem em cima da porta. Ao carregar, as malhas da porta (as
// que ficam na frente do modelo) são separadas em duas folhas pelo meio, e cada folha gira na dobradiça
// dela quando o jogador entra ou sai (jogador_porta_aberta). O corpo só tem as faces de fora: ele é
// desenhado dos dois lados, para ter o lado de dentro (a normal do shader de luz sai das derivadas da
// posição, então a luz fica certa dos dois lados).
#include "armario_modelo.h"
#include "ui.h"
#include "posfx.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>

static int indice(const Mesh *m, int t, int k) {
    return m->indices ? m->indices[3 * t + k] : 3 * t + k;
}

// O lado do triângulo t: 0 = folha esquerda (x do centro antes da emenda), 1 = direita.
static int lado_do(const Mesh *m, int t, float emenda) {
    float cx = 0.0f;
    for (int k = 0; k < 3; k++)
        cx += m->vertices[3 * indice(m, t, k)] / 3.0f;
    return cx < emenda ? 0 : 1;
}

// Copia os triângulos de um lado para uma malha nova, sem índices, e manda para a GPU.
static Mesh separa(const Mesh *m, int lado, float emenda) {
    int total = 0;
    for (int t = 0; t < m->triangleCount; t++) // padrão FILTER: conta os triângulos daquele lado
        total += lado_do(m, t, emenda) == lado;
    Mesh r = {0};
    r.vertexCount = total * 3;
    r.triangleCount = total;
    r.vertices = MemAlloc(r.vertexCount * 3 * sizeof(float));
    r.texcoords = m->texcoords ? MemAlloc(r.vertexCount * 2 * sizeof(float)) : NULL;
    r.normals = m->normals ? MemAlloc(r.vertexCount * 3 * sizeof(float)) : NULL;
    int n = 0;
    for (int t = 0; t < m->triangleCount; t++) { // padrão FILTER: copia os triângulos daquele lado
        if (lado_do(m, t, emenda) != lado) continue;
        for (int k = 0; k < 3; k++, n++) {
            int i = indice(m, t, k);
            memcpy(&r.vertices[3 * n], &m->vertices[3 * i], 3 * sizeof(float));
            if (r.texcoords) memcpy(&r.texcoords[2 * n], &m->texcoords[2 * i], 2 * sizeof(float));
            if (r.normals) memcpy(&r.normals[3 * n], &m->normals[3 * i], 3 * sizeof(float));
        }
    }
    if (total > 0) UploadMesh(&r, false);
    return r;
}

// As malhas na frente do modelo são a porta: cada uma vira duas folhas. A dobradiça de cada folha fica na
// ponta de fora dela, na frente da porta (assim ela abre para fora sem entrar no corpo).
static void separa_porta(Recursos *rec) {
    const Model *m = &rec->modelo_armario;
    BoundingBox b = GetModelBoundingBox(*m);
    float emenda = (b.min.x + b.max.x) * 0.5f, meio_z = (b.min.z + b.max.z) * 0.5f;
    rec->armario_n_folhas = 0;
    rec->armario_dobradica[0] = (Vector3){b.max.x, 0.0f, b.min.z};
    rec->armario_dobradica[1] = (Vector3){b.min.x, 0.0f, b.min.z};
    for (int i = 0; i < m->meshCount && i < ARMARIO_MAX_PARTES; i++) { // padrão FILTER: as malhas da porta
        BoundingBox p = GetMeshBoundingBox(m->meshes[i]);
        rec->armario_eh_porta[i] = p.min.z > meio_z && rec->armario_n_folhas + 2 <= ARMARIO_MAX_PARTES;
        if (!rec->armario_eh_porta[i]) continue;
        for (int lado = 0; lado < 2; lado++) { // cada metade vira uma folha
            int f = rec->armario_n_folhas++;
            rec->armario_folha[f] = separa(&m->meshes[i], lado, emenda);
            rec->armario_folha_lado[f] = lado;
            rec->armario_folha_mat[f] = m->meshMaterial[i];
        }
        Vector3 *esq = &rec->armario_dobradica[0], *dir = &rec->armario_dobradica[1];
        esq->x = fminf(esq->x, p.min.x);
        dir->x = fmaxf(dir->x, p.max.x);
        esq->z = dir->z = fmaxf(esq->z, p.max.z);
    }
}

void armario_modelo_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_armario = false;
    rec->armario_n_folhas = 0;
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
    separa_porta(rec);
}

void armario_modelo_descarrega(Recursos *rec) {
    if (!rec->tem_armario) return;
    for (int f = 0; f < rec->armario_n_folhas; f++) // padrão MAP: libera cada folha
        if (rec->armario_folha[f].triangleCount > 0) UnloadMesh(rec->armario_folha[f]);
    UnloadModel(rec->modelo_armario);
}

// Uma malha com a cor do armário (como o DrawModelEx faz com o tom).
static void desenha_malha(const Model *m, Mesh malha, int mat, Matrix t) {
    Color *cor = &m->materials[mat].maps[MATERIAL_MAP_DIFFUSE].color, antes = *cor;
    *cor = (Color){(unsigned char)(antes.r * 0xa8 / 255), (unsigned char)(antes.g * 0xb0 / 255),
                   (unsigned char)(antes.b * 0xa0 / 255), antes.a}; // um pouco mais escuro: a lanterna
    DrawMesh(malha, m->materials[mat], t);                          // estourava o metal claro
    *cor = antes;
}

// A folha f aberta `graus`: gira em torno da dobradiça do lado dela (a esquerda para um lado, a direita
// para o outro, as duas para fora).
static Matrix folha(const Recursos *rec, int f, float graus, Matrix lugar) {
    int lado = rec->armario_folha_lado[f];
    Vector3 d = rec->armario_dobradica[lado];
    float giro = (lado == 0 ? -graus : graus) * DEG2RAD;
    Matrix gira = MatrixMultiply(MatrixMultiply(MatrixTranslate(-d.x, 0.0f, -d.z), MatrixRotateY(giro)),
                                 MatrixTranslate(d.x, 0.0f, d.z));
    return MatrixMultiply(gira, lugar);
}

// porta_graus < 0: sem as folhas (quem está dentro vê a porta pelas frestas desenhadas na tela).
bool armario_modelo_desenha(const Estado *e, Vector3 centro, Vector3 tam, Vector3 frente, float porta_graus) {
    if (!e->rec.tem_armario) return false;
    const Recursos *rec = &e->rec;
    const Model *m = &rec->modelo_armario;
    BoundingBox b = GetModelBoundingBox(*m);
    float s = tam.y / fmaxf(0.01f, b.max.y - b.min.y);
    float fundo = (b.max.z - b.min.z) * s; // profundidade do modelo já escalado
    float meia =
        fabsf(frente.x) > fabsf(frente.z) ? tam.x * 0.5f : tam.z * 0.5f; // meia profundidade da caixa
    Vector3 alvo = Vector3Add(centro, Vector3Scale(frente, meia - fundo * 0.5f)); // a frente rente à caixa
    float giro = atan2f(frente.x, frente.z); // leva o +Z do modelo para `frente`
    Vector3 meio = {(b.min.x + b.max.x) * 0.5f, b.min.y, (b.min.z + b.max.z) * 0.5f};
    Vector3 desloca = Vector3RotateByAxisAngle(Vector3Scale(meio, s), (Vector3){0, 1, 0}, giro);
    Matrix lugar = MatrixMultiply(MatrixMultiply(m->transform, MatrixScale(s, s, s)),
                                  MatrixMultiply(MatrixRotateY(giro), MatrixTranslate(alvo.x - desloca.x,
                                                                                      -desloca.y,
                                                                                      alvo.z - desloca.z)));
    // aço pintado de verdade: os mapas do metal do jogo (lascas, ferrugem, riscos, amassados) projetados pela
    // posição no mundo, porque a UV do modelo não combina com as texturas do jogo
    posfx_material_triplanar(e, MATERIAL_METAL, 0.9f);
    rlDisableBackfaceCulling(); // o lado de dentro do corpo e das folhas abertas
    for (int i = 0; i < m->meshCount; i++) // laço de desenho: o corpo (a porta vai pelas folhas)
        if (i >= ARMARIO_MAX_PARTES || !rec->armario_eh_porta[i])
            desenha_malha(m, m->meshes[i], m->meshMaterial[i], lugar);
    for (int f = 0; f < rec->armario_n_folhas && porta_graus >= 0.0f; f++) // laço de desenho: as folhas
        if (rec->armario_folha[f].triangleCount > 0) {
            Matrix t = folha(rec, f, porta_graus, lugar);
            desenha_malha(m, rec->armario_folha[f], rec->armario_folha_mat[f], t);
        }
    rlEnableBackfaceCulling();
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
    return true;
}
