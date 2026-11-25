// cameras.c — o modelo tem a lente para +Z e a traseira em z mínimo. Uma placa fica na parede e um braço
// curto sai dela; a câmera gira na ponta do braço (primeiro inclina, depois vira), longe o bastante da parede
// para virar até CAMERA_GIRO_MAX_GRAUS sem encostar nela. As câmeras que enxergam o jogador viram para ele.
#include "cameras.h"
#include "ui.h"
#include "jogador.h"
#include "posfx.h"
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
#define N_ATIVAS  (N_CAMERAS < MAX_CAMERAS ? N_CAMERAS : MAX_CAMERAS) // o estado só tem MAX_CAMERAS vagas

// Para onde a câmera aponta de fábrica (a normal da parede), no plano.
static Vector3 normal_de(const CameraSeguranca *c) {
    return (Vector3){sinf(c->olha * DEG2RAD), 0.0f, cosf(c->olha * DEG2RAD)};
}

// O eixo em que ela gira: na ponta do braço, a CAMERA_ALTURA_M.
static Vector3 eixo_de(const CameraSeguranca *c) {
    Vector3 p = Vector3Add(c->parede, Vector3Scale(normal_de(c), CAMERA_BRACO_M));
    p.y = CAMERA_ALTURA_M;
    return p;
}

static float limita_angulo(float graus) { // em (-180, 180]
    while (graus > 180.0f)
        graus -= 360.0f;
    while (graus <= -180.0f)
        graus += 360.0f;
    return graus;
}

// ================================================================== atualiza
// A câmera i vê o jogador? As contas baratas primeiro (distância e o cone em volta de para onde ela aponta);
// só então o raio contra as paredes. Escondido no armário, ninguém vê.
static bool ve_jogador(const Estado *e, int i, Vector3 olho, Vector3 alvo) {
    if (e->jog.modo == MODO_ARMARIO) return false;
    Vector3 para = Vector3Subtract(alvo, olho);
    float dist = Vector3Length(para), plano = sqrtf(para.x * para.x + para.z * para.z);
    if (dist > CAMERA_VISAO_M || plano < 0.3f) return false;
    Vector3 n = normal_de(&CAMERAS[i]);
    if ((para.x * n.x + para.z * n.z) / plano < cosf((CAMERA_GIRO_MAX_GRAUS + 15.0f) * DEG2RAD)) return false;
    return col_paredes_distancia_raio(&e->paredes, olho, para, dist) >= dist - 0.3f;
}

// Leva `atual` até `alvo` como um motor: proporcional à distância (desacelera perto do alvo, sem travar de
// uma vez), limitado à velocidade máxima.
static void aproxima(float *atual, float alvo, float dt) {
    float passo = fminf(CAMERA_VELOCIDADE * dt, fabsf(alvo - *atual) * CAMERA_FIRMEZA * dt);
    *atual += Clamp(alvo - *atual, -passo, passo);
}

static bool fase_com_camera(Fase f) {
    return regras_fase_eh_rodada(f) || regras_fase_eh_saida(f) || f == FASE_RESPOSTA_FINAL;
}

// [REQ:CAMERAS-SEGUEM] só as câmeras que enxergam o jogador viram para ele; perdido, esperam e voltam
void cameras_atualiza(Estado *e, float dt) {
    if (!e->rec.tem_cameras) return;
    bool jogando = fase_com_camera(e->fase);
    Vector3 alvo = Vector3Subtract(jogador_olho(e), (Vector3){0.0f, 0.2f, 0.0f}); // o peito
    for (int i = 0; i < N_ATIVAS; i++) { // padrão MAP: cada câmera persegue (ou não) o jogador
        float *giro = &e->camera_giro[i], *inclina = &e->camera_inclina[i];
        Vector3 eixo = eixo_de(&CAMERAS[i]);
        bool ve = jogando && ve_jogador(e, i, eixo, alvo);
        bool em_repouso = fabsf(*giro) < 0.05f && fabsf(*inclina) < 0.05f;
        if (!ve && em_repouso && e->camera_perdeu_t[i] > CAMERA_ESPERA_S) { // parada e sem ninguém: nada a
            e->camera_ve_t[i] = 0.0f;                                          // fazer (quase sempre)
            continue;
        }
        float *mira_giro = &e->camera_mira_giro[i], *mira_inclina = &e->camera_mira_inclina[i];
        if (ve && e->camera_ve_t[i] <= 0.0f) { // acabou de avistar: a mira parte de onde ela está
            *mira_giro = *giro;
            *mira_inclina = *inclina;
        }
        e->camera_ve_t[i] = ve ? e->camera_ve_t[i] + dt : 0.0f;
        e->camera_perdeu_t[i] = ve ? 0.0f : e->camera_perdeu_t[i] + dt;
        float quer_giro = *mira_giro, quer_inclina = *mira_inclina; // sem novidade, a mira fica onde está
        if (ve && e->camera_ve_t[i] > CAMERA_REACAO_S) { // reagiu: quer olhar para o jogador
            Vector3 d = Vector3Subtract(alvo, eixo);
            float para = atan2f(d.x, d.z) * RAD2DEG, plano = sqrtf(d.x * d.x + d.z * d.z);
            float max = CAMERA_GIRO_MAX_GRAUS;
            quer_giro = Clamp(limita_angulo(para - CAMERAS[i].olha), -max, max);
            quer_inclina = Clamp(atan2f(-d.y, plano) * RAD2DEG - CAMERA_INCLINA_GRAUS, -20.0f, 45.0f);
        } else if (!ve && e->camera_perdeu_t[i] > CAMERA_ESPERA_S) { // perdeu faz tempo: volta para o lugar
            quer_giro = quer_inclina = 0.0f;
        }
        // a mira persegue o que ela quer com atraso (onde o jogador esteve há pouco), e o motor, a mira
        float k = 1.0f - expf(-dt / CAMERA_ATRASO_S);
        *mira_giro += (quer_giro - *mira_giro) * k;
        *mira_inclina += (quer_inclina - *mira_inclina) * k;
        aproxima(giro, *mira_giro, dt);
        aproxima(inclina, *mira_inclina, dt);
    }
}

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

// ================================================================== desenho
// O modelo: a traseira no eixo, escalado pelo comprimento, inclinado e virado (o normal + o que o motor
// mexeu), na ponta do braço.
static Matrix transforma(const Model *m, const CameraSeguranca *c, float giro, float inclina) {
    BoundingBox b = GetModelBoundingBox(*m);
    float s = CAMERA_COMPRIMENTO_M / fmaxf(0.01f, b.max.z - b.min.z);
    Vector3 traseira = {(b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f, b.min.z};
    Matrix m1 = MatrixMultiply(MatrixTranslate(-traseira.x, -traseira.y, -traseira.z), MatrixScale(s, s, s));
    Matrix m2 = MatrixMultiply(MatrixRotateX((CAMERA_INCLINA_GRAUS + inclina) * DEG2RAD),
                               MatrixRotateY((c->olha + giro) * DEG2RAD));
    Vector3 eixo = eixo_de(c);
    return MatrixMultiply(MatrixMultiply(m1, m2), MatrixTranslate(eixo.x, eixo.y, eixo.z));
}

// A placa na parede e o braço até o eixo (aço, fora das texturas do modelo).
static void suporte(const CameraSeguranca *c) {
    Vector3 n = normal_de(c), placa = {c->parede.x + n.x * 0.01f, CAMERA_ALTURA_M, c->parede.z + n.z * 0.01f};
    Vector3 tam = fabsf(n.x) > fabsf(n.z) ? (Vector3){0.02f, 0.2f, 0.14f} : (Vector3){0.14f, 0.2f, 0.02f};
    DrawCubeV(placa, tam, (Color){0x2a, 0x2e, 0x2a, 255});
    DrawCylinderEx(placa, eixo_de(c), 0.022f, 0.022f, 8, (Color){0x5a, 0x5e, 0x58, 255});
    DrawSphereEx(eixo_de(c), 0.035f, 8, 8, (Color){0x3a, 0x3e, 0x3a, 255}); // a junta que gira
}

void cameras_desenha(const Estado *e) {
    if (!e->rec.tem_cameras) return;
    const Model *m = &e->rec.modelo_camera;
    BoundingBox b = GetModelBoundingBox(*m);
    bool led = fmodf(e->tempo_total, 1.6f) < 0.5f; // o REC pisca
    posfx_superficie(e, 0.35f, 0.8f);               // o suporte é de aço
    for (int i = 0; i < N_CAMERAS; i++)             // laço de desenho: o suporte de cada câmera
        suporte(&CAMERAS[i]);
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
    for (int i = 0; i < N_CAMERAS; i++) { // padrão MAP: desenha cada câmera e o LED dela
        float giro = i < MAX_CAMERAS ? e->camera_giro[i] : 0.0f;
        float inclina = i < MAX_CAMERAS ? e->camera_inclina[i] : 0.0f;
        Matrix t = transforma(m, &CAMERAS[i], giro, inclina);
        for (int k = 0; k < m->meshCount; k++) // laço de desenho: as peças do modelo
            DrawMesh(m->meshes[k], m->materials[m->meshMaterial[k]], t);
        Vector3 frente = Vector3Transform((Vector3){b.max.x * 0.6f, b.max.y * 0.55f, b.max.z * 0.7f}, t);
        DrawSphereEx(frente, 0.018f, 6, 6, led ? (Color){255, 30, 30, 255} : (Color){60, 10, 10, 255});
    }
}
