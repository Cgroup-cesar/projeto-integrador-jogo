// cenario.c — ambientação sem colisão (ver cenario.h). As posições saem do mapa ASCII de nivel.c
// (1 célula = TAMANHO_CELULA): mudou o mapa, confira as tabelas abaixo.
#include "cenario.h"
#include "texturas.h"
#include "posfx.h"
#include "goteira.h"
#include "entulho.h"
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
    // sangue, onde a história aconteceu (o apagão de 15 de junho) e onde quem passa olha:
    // corredor norte, ao lado do armário (coluna 12): logo à esquerda de quem sai da sala de observação
    {DECALQUE_SANGUE, {23.4f, 1.25f, BORDA(5) + DESCOLA}, {1.7f, 0, 0}, {0, 1.9f, 0}, {255, 255, 255, 235}},
    // fim do corredor norte (parede leste): de frente para quem anda até o gerador
    {DECALQUE_SANGUE, {BORDA(22) - DESCOLA, 1.35f, 11.0f}, {0, 0, 1.9f}, {0, 2.1f, 0}, {255, 255, 255, 240}},
    // sala do gerador, ao lado do painel 1: "o americano foi ver o gerador e não voltou"
    {DECALQUE_SANGUE, {36.4f, 1.5f, BORDA(1) + DESCOLA}, {1.6f, 0, 0}, {0, 1.8f, 0}, {255, 255, 255, 230}},
    // corredor oeste, baixo, logo antes do armário da coluna 8 (alguém se arrastou até ele)
    {DECALQUE_SANGUE, {BORDA(8) + DESCOLA, 0.85f, 14.6f}, {0, 0, -1.4f}, {0, 1.5f, 0}, {255, 255, 255, 225}},
    // corredor sul, perto da boca do duto, de onde ele sai
    {DECALQUE_SANGUE, {30.6f, 1.15f, BORDA(12) - DESCOLA}, {-1.8f, 0, 0}, {0, 2.0f, 0}, {255, 255, 255, 235}},
    // ferrugem escorrendo da goteira da sala do gerador (parede leste), embaixo do cano
    {DECALQUE_UMIDADE,
     {BORDA(22) - DESCOLA, 1.2f, GOTEIRA_Z},
     {0, 0, 0.55f},
     {0, 1.0f, 0},
     {0x8c, 0x52, 0x30, 210}},
    // marcas de arrasto no chão, saindo da boca do duto (coluna 14, linha 11) para oeste
    {DECALQUE_ARRASTO, {26.15f, DESCOLA * 0.5f, 23.0f}, {4.5f, 0, 0}, {0, 0, -1.2f}, {255, 255, 255, 255}},
};

// Chão molhado: poucas manchas, cada uma de um jeito (borrão, escorrido, respingos), de tamanho e
// ângulo próprios, onde a água teria vindo de algum lugar (a umidade da parede, a goteira). Com o reflexo
// ligado: a lanterna e as lâmpadas brilham nelas.
typedef struct {
    Decalque tipo;
    float x, z, comprimento, largura, graus; // semieixos em metros; graus: o giro em torno do eixo y
    unsigned char alfa;
} Mancha;

static const Mancha CHAO_MOLHADO[] = {
    // corredor norte: a umidade da parede sul (x = 27) escorreu na diagonal para o meio do corredor
    {DECALQUE_ESCORRIDO, 26.4f, 11.1f, 1.5f, 0.45f, 118.0f, 205},
    // corredor leste, encostado na parede, embaixo da mancha de umidade
    {DECALQUE_MOLHADO, 42.9f, 16.7f, 0.85f, 0.6f, 23.0f, 190},
    // corredor sul: respingos de algo que pingou do eletroduto
    {DECALQUE_RESPINGO, 34.1f, 22.7f, 0.55f, 0.55f, 71.0f, 175},
    // corredor oeste, perto do armário: um borrão comprido e meio seco
    {DECALQUE_MOLHADO, 17.2f, 20.9f, 0.6f, 1.05f, -34.0f, 150},
    // sala do gerador: a água da poça que correu para o meio da sala
    {DECALQUE_ESCORRIDO, 42.2f, 6.3f, 1.2f, 0.35f, 205.0f, 190},
    {DECALQUE_POCA, GOTEIRA_X - 0.05f, GOTEIRA_Z + 0.05f, 0.62f, 0.52f, 12.0f, 240}, // embaixo da goteira
};

static void desenha_tabela(const Estado *e, const Placa *tabela, size_t n) {
    // laço de desenho: só percorre a tabela para desenhar (não é map/filter/fold)
    for (size_t i = 0; i < n; i++)
        texturas_quadro(e->rec.decalque[tabela[i].tipo], tabela[i].centro, tabela[i].direita, tabela[i].cima,
                        tabela[i].tinta);
}

// Cada mancha vira um quadro no chão, girado: a frente (direita × cima) fica para cima.
static void desenha_manchas(const Estado *e) {
    // laço de desenho: só percorre a tabela para desenhar
    for (size_t i = 0; i < sizeof(CHAO_MOLHADO) / sizeof(CHAO_MOLHADO[0]); i++) {
        const Mancha *m = &CHAO_MOLHADO[i];
        float a = m->graus * DEG2RAD, y = DESCOLA * (0.3f + 0.05f * (float)i); // sem duas na mesma altura
        Vector3 direita = {cosf(a) * m->comprimento, 0.0f, sinf(a) * m->comprimento};
        Vector3 cima = {sinf(a) * m->largura, 0.0f, -cosf(a) * m->largura};
        texturas_quadro(e->rec.decalque[m->tipo], (Vector3){m->x, y, m->z}, direita, cima,
                        (Color){255, 255, 255, m->alfa});
    }
}

// Decalques com alfa e sem escrever profundidade: a criatura e as partículas passam por cima.
static void desenha_decalques(const Estado *e) {
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    desenha_tabela(e, PLACAS, sizeof(PLACAS) / sizeof(PLACAS[0]));
    posfx_molhado(e, CHAO_MOLHADO_BRILHO); // envia o lote antes: só o chão molhado reflete
    desenha_manchas(e);
    posfx_molhado(e, 0.0f);
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
    {{10.8f, 1.8f, 18.45f}, {10.6f, 2.4f, 18.3f}, {10.7f, 3.0f, 18.15f}}, // do painel 2 (parede norte do Arquivo)
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
    entulho_desenha(e);
    desenha_decalques(e);
    goteira_desenha(e);
}

// ------------------------------------------------------------------ halo das lâmpadas
void cenario_comeca_halos(void) {
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
}

// O halo some com a névoa, como a luz: de longe, a lâmpada não "fura" o escuro.
void cenario_halo(const Estado *e, Vector3 posicao, Color cor) {
    float distancia = Vector3Distance(e->camera.position, posicao);
    float alfa = HALO_LAMPADA_ALFA * expf(-NEVOA_DENSIDADE * distancia);
    DrawBillboard(e->camera, e->rec.halo, posicao, HALO_LAMPADA_M, Fade(cor, alfa));
}

void cenario_termina_halos(void) {
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}
