// caixas.c — as pilhas de caixotes das salas. O modelo é uma pilha só, comprida no eixo Z dele; cada pilha é
// escalada para CAIXAS_COMPRIMENTO_M e girada para correr ao longo da parede. A colisão é uma caixa com as
// medidas da config.h, então funciona igual com o modelo ou com o placeholder.
#include "caixas.h"
#include "texturas.h"
#include "ui.h"
#include <math.h>

typedef struct {
    float x, z;      // centro no chão (m)
    bool ao_longo_x; // true: a pilha corre na direção X do mundo (encostada numa parede norte ou sul)
} Pilha;

// Encostadas nas paredes, longe das portas, dos móveis e do caminho do Precursor. Edite à vontade.
static const Pilha PILHAS[] = {
    {3.3f, 13.5f, true},  // sala de observação: canto sudoeste
    {11.0f, 25.5f, true}, // Arquivo: parede sul
    {42.7f, 2.5f, true},  // sala do painel 1: parede norte, no canto leste
};
#define N_PILHAS ((int)(sizeof(PILHAS) / sizeof(PILHAS[0])))

void caixas_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_caixas = false;
    if (!ui_caminho_asset("modelos/caixas.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/caixas.glb; usando caixas simples");
        return;
    }
    rec->modelo_caixas = LoadModel(caminho);
    rec->tem_caixas = IsModelValid(rec->modelo_caixas) && rec->modelo_caixas.meshCount > 0;
    if (!rec->tem_caixas) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; usando caixas simples", caminho);
        return;
    }
    for (int i = 0; i < rec->modelo_caixas.materialCount && rec->luz_ok; i++) // padrão MAP: luz do jogo
        rec->modelo_caixas.materials[i].shader = rec->luz;
}

void caixas_descarrega(Recursos *rec) {
    if (rec->tem_caixas) UnloadModel(rec->modelo_caixas);
}

static Vector3 tamanho(const Pilha *p) {
    float comprimento = CAIXAS_COMPRIMENTO_M, fundo = CAIXAS_PROFUNDIDADE_M;
    return p->ao_longo_x ? (Vector3){comprimento, CAIXAS_ALTURA_M, fundo}
                         : (Vector3){fundo, CAIXAS_ALTURA_M, comprimento};
}

void caixas_adiciona_paredes(ColParedes *paredes) {
    for (int i = 0; i < N_PILHAS; i++) { // padrão MAP: uma caixa de colisão por pilha
        Vector3 t = tamanho(&PILHAS[i]), meio = Vector3Scale(t, 0.5f);
        Vector3 c = {PILHAS[i].x, meio.y, PILHAS[i].z};
        col_paredes_adiciona(paredes, (Parede){Vector3Subtract(c, meio), Vector3Add(c, meio), PAREDE_MOVEL});
    }
}

// Os "Fragile" pintados no modelo, em coordenadas dele: o centro de cada rótulo, a direção para fora da face
// e o tamanho da placa (m) que o cobre. Os dois primeiros são as faces largas do caixote grande (malha 4);
// os outros, as do caixote de cima (malha 3), girado 46° na pilha (eixos medidos nos vértices).
typedef struct {
    Vector3 centro, fora;
    float largura, altura;
} Rotulo;

static const Rotulo ROTULOS[] = {
    {{0.83f, 1.07f, -20.70f}, {1, 0, 0}, CAIXAS_PLACA_LARGURA_M, CAIXAS_PLACA_ALTURA_M},
    {{-0.75f, 1.07f, -20.70f}, {-1, 0, 0}, CAIXAS_PLACA_LARGURA_M, CAIXAS_PLACA_ALTURA_M},
    {{0.48f, 2.95f, -18.64f}, {0.719f, 0, 0.695f}, 0.45f, 0.2f},
    {{-0.42f, 2.95f, -19.52f}, {-0.719f, 0, -0.695f}, 0.45f, 0.2f},
};
#define N_ROTULOS ((int)(sizeof(ROTULOS) / sizeof(ROTULOS[0])))

// Um ponto (ou direção) do modelo girado como em desenha_modelo: 90° em Y leva (x, y, z) para (z, y, -x).
static Vector3 gira(const Pilha *p, Vector3 v) {
    return p->ao_longo_x ? (Vector3){v.z, v.y, -v.x} : v;
}

// A placa: uma chapa de metal pintada de creme, pregada na face, com duas linhas de estêncil preto. A chapa
// fica 1 cm para fora da madeira e o texto 1 cm para fora da chapa: colados, eles brigavam (z-fighting).
static void placa(const Estado *e, const Pilha *p, Vector3 pos, float s, const Rotulo *r) {
    Vector3 normal = Vector3Normalize(gira(p, r->fora)), cima = {0, 1, 0};
    Vector3 direita = Vector3CrossProduct(Vector3Negate(normal), cima); // a direita de quem olha a face
    Vector3 centro = Vector3Add(pos, Vector3Scale(gira(p, r->centro), s));
    centro = Vector3Add(centro, Vector3Scale(normal, 0.012f));
    texturas_quadro(e->rec.material[MATERIAL_METAL], centro, Vector3Scale(direita, r->largura),
                    Vector3Scale(cima, r->altura), (Color){0xe0, 0xd4, 0xa8, 255});
    Color tinta = {0x14, 0x14, 0x10, 255};
    Vector3 frente = Vector3Add(centro, Vector3Scale(normal, 0.01f));
    float l = r->largura * 0.92f, a = r->altura;
    texturas_quadro(e->rec.decalque[DECALQUE_CAIXA_1], Vector3Add(frente, Vector3Scale(cima, a * 0.22f)),
                    Vector3Scale(direita, l), Vector3Scale(cima, l * 0.25f), tinta);
    texturas_quadro(e->rec.decalque[DECALQUE_CAIXA_2], Vector3Add(frente, Vector3Scale(cima, -a * 0.24f)),
                    Vector3Scale(direita, l * 0.82f), Vector3Scale(cima, l * 0.82f * 0.25f), tinta);
}

// O modelo: escalado pelo comprimento (eixo Z dele), com a base no chão e o centro no centro da pilha.
// Girar 90° em Y leva (x, y, z) para (z, y, -x): o comprimento passa a correr em X.
static void desenha_modelo(const Estado *e, const Pilha *p) {
    const Model *m = &e->rec.modelo_caixas;
    BoundingBox b = GetModelBoundingBox(*m);
    float s = CAIXAS_COMPRIMENTO_M / fmaxf(0.01f, b.max.z - b.min.z);
    Vector3 meio = Vector3Scale(Vector3Add(b.min, b.max), 0.5f);
    float giro = p->ao_longo_x ? 90.0f : 0.0f;
    Vector3 desloca = p->ao_longo_x ? (Vector3){meio.z, 0, -meio.x} : (Vector3){meio.x, 0, meio.z};
    Vector3 pos = {p->x - desloca.x * s, -b.min.y * s, p->z - desloca.z * s};
    DrawModelEx(*m, pos, (Vector3){0, 1, 0}, giro, (Vector3){s, s, s}, WHITE);
    for (int i = 0; i < N_ROTULOS; i++) // padrão MAP: uma placa sobre cada "Fragile"
        placa(e, p, pos, s, &ROTULOS[i]);
}

// Sem o modelo: dois caixotes lado a lado e um menor em cima, cor de madeira.
static void desenha_simples(const Pilha *p) {
    Vector3 t = tamanho(p);
    Color madeira = {0x5a, 0x45, 0x2c, 255}, escura = {0x3e, 0x2f, 0x1e, 255};
    Vector3 eixo = p->ao_longo_x ? (Vector3){1, 0, 0} : (Vector3){0, 0, 1};
    float metade = CAIXAS_COMPRIMENTO_M * 0.25f, baixo = t.y * 0.55f;
    Vector3 um = p->ao_longo_x ? (Vector3){t.x * 0.48f, baixo, t.z} : (Vector3){t.x, baixo, t.z * 0.48f};
    for (int lado = -1; lado <= 1; lado += 2) { // padrão MAP: os dois caixotes de baixo
        Vector3 c = Vector3Add((Vector3){p->x, baixo * 0.5f, p->z}, Vector3Scale(eixo, lado * metade));
        DrawCubeV(c, um, madeira);
        DrawCubeWiresV(c, um, escura);
    }
    Vector3 topo = {um.x * 0.6f, t.y - baixo, um.z * 0.8f};
    Vector3 c = {p->x, baixo + topo.y * 0.5f, p->z};
    DrawCubeV(c, topo, madeira);
    DrawCubeWiresV(c, topo, escura);
}

void caixas_desenha(const Estado *e) {
    for (int i = 0; i < N_PILHAS; i++) // padrão MAP: desenha cada pilha
        if (e->rec.tem_caixas)
            desenha_modelo(e, &PILHAS[i]);
        else
            desenha_simples(&PILHAS[i]);
}
