// cameras.c — o modelo tem a lente para +Z e a placa do suporte atrás (z mínimo). Cada câmera gira em torno
// da placa: primeiro inclina para baixo, depois vira na direção em que olha; a placa fica rente à parede.
#include "cameras.h"
#include "ui.h"
#include <math.h>

typedef struct {
    Vector3 parede; // ponto da parede onde a placa encosta (m)
    float olha;     // para onde a lente aponta: a normal da parede (0 = +Z/sul, 90 = +X/leste, 180 = norte)
} CameraSeguranca;

// No alto (CAMERA_ALTURA_M), com o suporte reto na parede (a lente aponta na direção da normal dela: o
// suporte é rígido, e na diagonal dos cantos a placa ficava desencostada e a câmera parecia torta).
static const CameraSeguranca CAMERAS[] = {
    {{13.0f, 0, 14.0f}, 180.0f}, // sala de observação: parede sul, perto do canto leste, olhando o console
    {{14.0f, 0, 18.9f}, 270.0f}, // Arquivo: parede leste, perto do canto norte, olhando a mesa do bilhete
    {{34.0f, 0, 3.0f}, 90.0f},   // sala do gerador: parede oeste, olhando o painel 1
    {{18.0f, 0, 10.0f}, 0.0f},   // corredor norte, na saída da sala de observação
    {{44.0f, 0, 11.0f}, 270.0f}, // corredor norte, ponta leste, olhando o corredor todo
    {{17.0f, 0, 24.0f}, 180.0f}, // corredor oeste, ponta sul, olhando para o norte
    {{44.0f, 0, 23.0f}, 270.0f}, // corredor sul, ponta leste, olhando para o duto
};
#define N_CAMERAS ((int)(sizeof(CAMERAS) / sizeof(CAMERAS[0])))

void cameras_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_cameras = false;
    if (!ui_caminho_asset("modelos/camera.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/camera.glb; sem câmeras de segurança");
        return;
    }
    rec->modelo_camera = LoadModel(caminho);
    rec->tem_cameras = IsModelValid(rec->modelo_camera) && rec->modelo_camera.meshCount > 0;
    if (!rec->tem_cameras) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; sem câmeras de segurança", caminho);
        return;
    }
    for (int i = 0; i < rec->modelo_camera.materialCount && rec->luz_ok; i++) // padrão MAP: luz do jogo
        rec->modelo_camera.materials[i].shader = rec->luz;
}

void cameras_descarrega(Recursos *rec) {
    if (rec->tem_cameras) UnloadModel(rec->modelo_camera);
}

static Matrix transforma(const Model *m, const CameraSeguranca *c) {
    BoundingBox b = GetModelBoundingBox(*m);
    float s = CAMERA_COMPRIMENTO_M / fmaxf(0.01f, b.max.z - b.min.z);
    Vector3 placa = {(b.min.x + b.max.x) * 0.5f, b.min.y + (b.max.y - b.min.y) * 0.3f, b.min.z};
    Matrix m1 = MatrixMultiply(MatrixTranslate(-placa.x, -placa.y, -placa.z), MatrixScale(s, s, s));
    Matrix m2 =
        MatrixMultiply(MatrixRotateX(CAMERA_INCLINA_GRAUS * DEG2RAD), MatrixRotateY(c->olha * DEG2RAD));
    return MatrixMultiply(MatrixMultiply(m1, m2), MatrixTranslate(c->parede.x, CAMERA_ALTURA_M, c->parede.z));
}

void cameras_desenha(const Estado *e) {
    if (!e->rec.tem_cameras) return;
    const Model *m = &e->rec.modelo_camera;
    BoundingBox b = GetModelBoundingBox(*m);
    bool led = fmodf(e->tempo_total, 1.6f) < 0.5f; // o REC pisca
    for (int i = 0; i < N_CAMERAS; i++) {          // padrão MAP: desenha cada câmera e o LED dela
        Matrix t = transforma(m, &CAMERAS[i]);
        for (int k = 0; k < m->meshCount; k++) // laço de desenho: as peças do modelo
            DrawMesh(m->meshes[k], m->materials[m->meshMaterial[k]], t);
        Vector3 frente = Vector3Transform((Vector3){b.max.x * 0.6f, b.max.y * 0.55f, b.max.z * 0.7f}, t);
        DrawSphereEx(frente, 0.018f, 6, 6, led ? (Color){255, 30, 30, 255} : (Color){60, 10, 10, 255});
    }
}
