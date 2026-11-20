// cenario.c — ambientação sem colisão (ver cenario.h). As posições saem do mapa ASCII de nivel.c
// (1 célula = TAMANHO_CELULA): mudou o mapa, confira as tabelas abaixo.
#include "cenario.h"
#include "texturas.h"
#include "rlgl.h"
#include <math.h>

#define BORDA(n)     ((n) * TAMANHO_CELULA) // a borda da célula n, em metros
#define DESCOLA      0.012f                 // o decalque fica 1,2 cm à frente da superfície (sem z-fighting)
#define ALTURA_DUTOS (PE_DIREITO - 0.18f)

// ------------------------------------------------------------------ decalques
typedef struct {
    Decalque tipo;
    Vector3 centro;
    Vector3 direita, cima; // semieixos inteiros; a frente do decalque é direita × cima
    Color tinta;
} Placa;

static const Placa PLACAS[] = {
    // corredor norte (linha 5): parede de cima (z = 10), vista do corredor (+z)
    {DECALQUE_POSTO, {19.0f, 2.0f, BORDA(5) + DESCOLA}, {2.0f, 0, 0}, {0, 0.5f, 0}, {0xc8, 0xc0, 0xa8, 190}},
    {DECALQUE_RESTRITA,
     {35.0f, 2.0f, BORDA(5) + DESCOLA},
     {2.4f, 0, 0},
     {0, 0.6f, 0},
     {0xd9, 0xa4, 0x41, 180}},
    // corredor oeste (coluna 8): parede da esquerda (x = 16), junto da porta do Arquivo, vista de +x
    {DECALQUE_ARQUIVO,
     {BORDA(8) + DESCOLA, 2.0f, 19.0f},
     {0, 0, -1.6f},
     {0, 0.4f, 0},
     {0xc8, 0xc0, 0xa8, 190}},
    // umidade: corredor norte (parede de baixo), corredor leste e parede norte do Arquivo
    {DECALQUE_UMIDADE, {27.0f, 1.7f, BORDA(6) - DESCOLA}, {-1.8f, 0, 0}, {0, 2.2f, 0}, {255, 255, 255, 230}},
    {DECALQUE_UMIDADE, {BORDA(22) - DESCOLA, 1.5f, 17.0f}, {0, 0, 1.6f}, {0, 2.0f, 0}, {255, 255, 255, 230}},
    {DECALQUE_UMIDADE, {9.0f, 1.8f, BORDA(9) + DESCOLA}, {1.8f, 0, 0}, {0, 2.2f, 0}, {255, 255, 255, 230}},
    // marcas de arrasto no chão, saindo da boca do duto (coluna 14, linha 11) para oeste
    {DECALQUE_ARRASTO, {26.15f, DESCOLA * 0.5f, 23.0f}, {4.5f, 0, 0}, {0, 0, -1.2f}, {255, 255, 255, 255}},
};

// Decalques com alfa e sem escrever profundidade: a criatura e as partículas passam por cima.
static void desenha_decalques(const Estado *e) {
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    // laço de desenho: só percorre a tabela para desenhar (não é map/filter/fold)
    for (size_t i = 0; i < sizeof(PLACAS) / sizeof(PLACAS[0]); i++) {
        const Placa *p = &PLACAS[i];
        texturas_quadro(e->rec.decalque[p->tipo], p->centro, p->direita, p->cima, p->tinta);
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}

// ------------------------------------------------------------------ eletrodutos, grelhas e fiação
// Um anel de eletroduto pelo teto dos corredores (linha 5, coluna 8, coluna 21 e linha 11).
static const Vector3 DUTOS[][2] = {
    {{16.0f, ALTURA_DUTOS, 10.25f}, {44.0f, ALTURA_DUTOS, 10.25f}},
    {{16.25f, ALTURA_DUTOS, 10.25f}, {16.25f, ALTURA_DUTOS, 23.75f}},
    {{43.75f, ALTURA_DUTOS, 10.25f}, {43.75f, ALTURA_DUTOS, 23.75f}},
    {{16.25f, ALTURA_DUTOS, 23.75f}, {43.75f, ALTURA_DUTOS, 23.75f}},
};

// Grelhas de ventilação no alto das paredes (centro, largura ao longo de x ou z).
static const Vector3 GRELHAS[] = {{33.0f, 2.45f, BORDA(12) - 0.02f}, {27.0f, 2.5f, BORDA(5) + 0.02f}};

// Fios soltos: dos painéis até o teto, e um pendurado perto do duto (início, meio, fim).
static const Vector3 FIOS[][3] = {
    {{38.8f, 1.8f, 5.1f}, {38.5f, 2.3f, 5.3f}, {38.6f, 3.0f, 4.9f}},
    {{39.3f, 1.8f, 5.0f}, {39.6f, 2.5f, 4.8f}, {39.5f, 3.0f, 5.3f}},
    {{9.0f, 1.8f, 20.7f}, {9.3f, 2.4f, 20.5f}, {9.1f, 3.0f, 20.3f}},
    {{30.0f, 3.0f, 22.6f}, {30.1f, 2.6f, 22.7f}, {29.95f, 2.2f, 22.55f}},
};

static void desenha_dutos(const Estado *e) {
    Texture2D metal = e->rec.material[MATERIAL_METAL];
    Color cor = {0x30, 0x33, 0x2e, 255};
    // laço de desenho: cada trecho vira uma caixa fina entre os dois pontos
    for (size_t i = 0; i < sizeof(DUTOS) / sizeof(DUTOS[0]); i++) {
        Vector3 a = DUTOS[i][0], b = DUTOS[i][1];
        Vector3 tam = {fabsf(b.x - a.x) + 0.07f, 0.07f, fabsf(b.z - a.z) + 0.07f};
        texturas_caixa(metal, Vector3Lerp(a, b, 0.5f), tam, texturas_modo(MATERIAL_METAL, 30 + (int)i), cor);
        texturas_caixa(metal, a, (Vector3){0.2f, 0.16f, 0.2f}, texturas_modo(MATERIAL_METAL, 40 + (int)i),
                       cor);
    }
}

static void desenha_grelhas(const Estado *e) {
    // laço de desenho: moldura de metal e quatro aletas escuras por grelha
    for (size_t i = 0; i < sizeof(GRELHAS) / sizeof(GRELHAS[0]); i++) {
        Vector3 c = GRELHAS[i];
        texturas_caixa(e->rec.material[MATERIAL_METAL], c, (Vector3){0.7f, 0.36f, 0.04f},
                       texturas_modo(MATERIAL_METAL, 50 + (int)i), (Color){0x2a, 0x2d, 0x28, 255});
        for (int k = 0; k < 4; k++)
            DrawCubeV((Vector3){c.x, c.y - 0.12f + k * 0.08f, c.z}, (Vector3){0.6f, 0.035f, 0.06f},
                      (Color){0x08, 0x09, 0x08, 255});
    }
}

static void desenha_fios(void) {
    Color cor = {0x10, 0x10, 0x0e, 255};
    // laço de desenho: dois trechos por fio (a curva caída)
    for (size_t i = 0; i < sizeof(FIOS) / sizeof(FIOS[0]); i++) {
        DrawCylinderEx(FIOS[i][0], FIOS[i][1], 0.012f, 0.012f, 5, cor);
        DrawCylinderEx(FIOS[i][1], FIOS[i][2], 0.012f, 0.012f, 5, cor);
    }
}

// [REQ:CENARIO] ambientação sem colisão: placas, umidade, arrasto, eletrodutos, grelhas e fios
void cenario_desenha(const Estado *e) {
    desenha_dutos(e);
    desenha_grelhas(e);
    desenha_fios();
    desenha_decalques(e);
}

