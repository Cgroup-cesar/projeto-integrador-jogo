// mesa_bilhete.c — a mesa do bilhete, encostada na parede do fundo do Arquivo e virada para a porta: é a
// primeira coisa no centro da visão de quem entra. O modelo da mesa olha para +X (gavetas e vão das pernas),
// que já é a direção da porta, então não gira; é escalado pela altura. As folhas vêm de um modelo com 5 peças
// (em centímetros); cada peça é desenhada sozinha, no lugar dela, com DrawMesh.
#include "mesa_bilhete.h"
#include "cenario.h"
#include "ui.h"
#include <math.h>

// As peças de papeis.glb: 0 folha dobrada, 1 bola amassada, 2 folha enrolada, 3 pilha, 4 folha lisa.
enum { PAPEL_DOBRADO, PAPEL_BOLA, PAPEL_ENROLADO, PAPEL_PILHA, PAPEL_LISO, PAPEIS };

typedef struct {
    int peca;
    float dx, dz; // em relação ao bilhete (m)
    bool no_chao; // false: em cima da mesa
    float giro;   // graus em Y
} Folha;

static const Folha FOLHAS[] = {
    {PAPEL_LISO, 0.0f, 0.0f, false, 12.0f},      // o bilhete, no meio da mesa
    {PAPEL_PILHA, -0.18f, -0.55f, false, -8.0f}, // uma pilha de papéis no canto
    {PAPEL_DOBRADO, 0.08f, 0.52f, false, -24.0f},
    {PAPEL_BOLA, 0.75f, 0.55f, true, 0.0f}, // no chão, na frente da mesa
    {PAPEL_ENROLADO, 0.6f, -0.7f, true, 40.0f},
};
#define N_FOLHAS ((int)(sizeof(FOLHAS) / sizeof(FOLHAS[0])))

static bool carrega(const char *relativo, Model *m) {
    char caminho[512];
    if (!ui_caminho_asset(relativo, caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/%s; usando a versão simples", relativo);
        return false;
    }
    *m = LoadModel(caminho);
    if (!IsModelValid(*m) || m->meshCount == 0) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; usando a versão simples", caminho);
        return false;
    }
    return true;
}

void mesa_bilhete_carrega(Recursos *rec) {
    rec->tem_mesa = carrega("modelos/mesa.glb", &rec->modelo_mesa);
    rec->tem_papeis =
        carrega("modelos/papeis.glb", &rec->modelo_papeis) && rec->modelo_papeis.meshCount >= PAPEIS;
    for (int i = 0; rec->tem_mesa && rec->luz_ok && i < rec->modelo_mesa.materialCount; i++) // padrão MAP
        rec->modelo_mesa.materials[i].shader = rec->luz;
    for (int i = 0; rec->tem_papeis && rec->luz_ok && i < rec->modelo_papeis.materialCount; i++) // padrão MAP
        rec->modelo_papeis.materials[i].shader = rec->luz;
}

void mesa_bilhete_descarrega(Recursos *rec) {
    if (rec->tem_mesa) UnloadModel(rec->modelo_mesa);
    if (rec->modelo_papeis.meshCount > 0) UnloadModel(rec->modelo_papeis);
}

// A lâmpada não fica firme: um tremor rápido e, de vez em quando, uma queda curta (mau contato).
float mesa_bilhete_piscada(const Estado *e) {
    float t = e->tempo_total;
    float tremor = 0.9f + 0.1f * sinf(t * 23.0f) * sinf(t * 7.3f);
    float ciclo = fmodf(t, 4.7f);
    float queda = (ciclo > 4.35f && ciclo < 4.42f) || (ciclo > 4.5f && ciclo < 4.55f) ? 0.25f : 1.0f;
    return tremor * queda;
}

static void desenha_folha(const Estado *e, const Folha *f, float tampo) {
    const Model *m = &e->rec.modelo_papeis;
    BoundingBox b = GetMeshBoundingBox(m->meshes[f->peca]);
    float s = MESA_PAPEL_ESCALA;
    Vector3 base = e->nivel.diario;
    Matrix centra = MatrixTranslate(-(b.min.x + b.max.x) * 0.5f, -b.min.y, -(b.min.z + b.max.z) * 0.5f);
    Matrix giro = MatrixRotateY(f->giro * DEG2RAD);
    Matrix lugar = MatrixTranslate(base.x + f->dx, f->no_chao ? 0.0f : tampo + 0.003f, base.z + f->dz);
    Matrix t = MatrixMultiply(MatrixMultiply(MatrixMultiply(centra, MatrixScale(s, s, s)), giro), lugar);
    DrawMesh(m->meshes[f->peca], m->materials[m->meshMaterial[f->peca]], t);
}

void mesa_bilhete_desenha(const Estado *e) {
    Vector3 d = e->nivel.diario;
    float tampo = MESA_ALTURA_M;
    if (e->rec.tem_mesa) {
        const Model *m = &e->rec.modelo_mesa;
        BoundingBox b = GetModelBoundingBox(*m);
        float s = MESA_ALTURA_M / fmaxf(0.01f, b.max.y - b.min.y);
        Vector3 pos = {d.x - (b.min.x + b.max.x) * 0.5f * s, -b.min.y * s,
                       d.z - (b.min.z + b.max.z) * 0.5f * s};
        DrawModelEx(*m, pos, (Vector3){0, 1, 0}, 0.0f, (Vector3){s, s, s}, WHITE);
    } else {
        DrawCubeV((Vector3){d.x, tampo * 0.5f, d.z}, (Vector3){MESA_FUNDO_M, tampo, MESA_COMPRIMENTO_M},
                  (Color){0x3a, 0x2c, 0x20, 255});
    }
    if (e->rec.tem_papeis) {
        for (int i = 0; i < N_FOLHAS; i++) // padrão MAP: cada folha no seu lugar
            desenha_folha(e, &FOLHAS[i], tampo);
    } else {
        DrawCubeV((Vector3){d.x, tampo + 0.01f, d.z}, (Vector3){0.22f, 0.02f, 0.3f},
                  (Color){0xc8, 0xc0, 0xa8, 255});
    }
    // o fio e a cúpula da lâmpada pendurada sobre a mesa
    Vector3 lamp = {d.x, MESA_LUZ_Y, d.z};
    DrawLine3D((Vector3){d.x, PE_DIREITO, d.z}, Vector3Add(lamp, (Vector3){0, 0.12f, 0}),
               (Color){20, 20, 18, 255});
    DrawCylinderEx(Vector3Add(lamp, (Vector3){0, 0.12f, 0}), Vector3Add(lamp, (Vector3){0, -0.02f, 0}), 0.03f,
                   0.16f, 10, (Color){0x2a, 0x30, 0x26, 255});
}

void mesa_bilhete_desenha_brilho(const Estado *e) {
    if (!regras_fase_eh_saida(e->fase)) return;
    Vector3 d = e->nivel.diario;
    float forca = mesa_bilhete_piscada(e) * (e->apagao_t > 0.0f ? 0.1f : 1.0f);
    cenario_halo(e, (Vector3){d.x, MESA_LUZ_Y - 0.05f, d.z}, Fade(MESA_COR_LUZ, forca));
    DrawBillboard(e->camera, e->rec.halo, (Vector3){d.x, MESA_LUZ_Y - 0.1f, d.z}, 1.6f,
                  Fade(MESA_COR_LUZ, 0.22f * forca)); // um brilho largo: dá para ver a lâmpada da porta
    if (e->diario_lido) return;                       // depois de lido, o bilhete para de chamar
    float respira = 0.5f + 0.5f * sinf(e->tempo_total * 2.2f);
    DrawBillboard(e->camera, e->rec.halo, (Vector3){d.x, MESA_ALTURA_M + 0.06f, d.z}, 0.55f,
                  Fade(MESA_COR_LUZ, 0.10f + 0.18f * respira));
}
