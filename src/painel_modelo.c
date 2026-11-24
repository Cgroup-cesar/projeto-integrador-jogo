// painel_modelo.c — o quadro elétrico dos painéis (ver painel_modelo.h). As chapas usam o material de aço
// pintado (lascas, ferrugem, riscos); as peças de aço nu (dobradiças, puxador, eletrodutos) são metálicas e
// lisas, sem mapa.
#include "painel_modelo.h"
#include "texturas.h"
#include "posfx.h"
#include "rlgl.h"

#define FRENTE 0.225f // a face da frente do corpo, a partir do centro (o corpo tem 0,45 m de fundo)

static const Color TINTA = {0x4a, 0x56, 0x4c, 255};      // verde-acinzentado de quadro industrial
static const Color TINTA_PORTA = {0x56, 0x62, 0x56, 255}; // a porta, um pouco mais clara (repintada)
static const Color ESCURO = {0x22, 0x26, 0x22, 255};
static const Color ACO = {0x9a, 0x9e, 0x98, 255};

// A placa de perigo: amarela, com faixas pretas inclinadas (paralelogramos no plano da frente).
static void placa_perigo(Vector3 c, float largura, float altura) {
    DrawCubeV(c, (Vector3){largura, altura, 0.006f}, (Color){0xc4, 0x9c, 0x22, 255});
    float z = c.z + 0.0035f, x0 = c.x - largura * 0.5f, y0 = c.y - altura * 0.5f;
    float passo = largura / 6.0f, faixa = passo * 0.5f, inclina = altura * 0.6f;
    rlBegin(RL_QUADS);
    rlColor4ub(0x14, 0x14, 0x12, 255);
    rlNormal3f(0.0f, 0.0f, 1.0f);
    for (int k = 0; k < 5; k++) { // laço de desenho: cada faixa preta (o topo deslocado: a inclinação)
        float x = x0 + passo * 0.3f + k * passo * 1.05f;
        rlVertex3f(x, y0, z);
        rlVertex3f(x + faixa, y0, z);
        rlVertex3f(x + faixa + inclina * 0.5f, y0 + altura, z);
        rlVertex3f(x + inclina * 0.5f, y0 + altura, z);
    }
    rlEnd();
}

static void chapas(const Estado *e, Vector3 b, int var) {
    Texture2D aco = e->rec.material[MATERIAL_METAL];
    posfx_material(e, MATERIAL_METAL);
    ModoUV uv = texturas_modo(MATERIAL_METAL, 70 + var);
    texturas_caixa(aco, (Vector3){b.x, 0.95f, b.z}, (Vector3){1.2f, 1.7f, 0.45f}, uv, TINTA);  // corpo
    texturas_caixa(aco, (Vector3){b.x, 0.05f, b.z}, (Vector3){1.24f, 0.1f, 0.47f}, uv, ESCURO); // base
    texturas_caixa(aco, (Vector3){b.x, 1.82f, b.z}, (Vector3){1.26f, 0.04f, 0.5f}, uv, TINTA);  // tampa
    // a porta, saliente; em volta dela, o vão escuro de 1 cm entre a porta e o batente
    texturas_caixa(aco, (Vector3){b.x, 0.97f, b.z + FRENTE + 0.004f}, (Vector3){1.1f, 1.58f, 0.006f}, uv,
                   ESCURO);
    texturas_caixa(aco, (Vector3){b.x, 0.97f, b.z + FRENTE + 0.012f}, (Vector3){1.08f, 1.56f, 0.018f}, uv,
                   TINTA_PORTA);
    // as venezianas embaixo da porta: um fundo preto e as lâminas por cima
    texturas_caixa(aco, (Vector3){b.x, 0.42f, b.z + FRENTE + 0.022f}, (Vector3){0.64f, 0.26f, 0.004f}, uv,
                   (Color){0x08, 0x09, 0x08, 255});
    for (int k = 0; k < 6; k++) // laço de desenho: cada lâmina da veneziana
        texturas_caixa(aco, (Vector3){b.x, 0.31f + k * 0.045f, b.z + FRENTE + 0.03f},
                       (Vector3){0.62f, 0.012f, 0.016f}, uv, TINTA_PORTA);
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
}

// Aço nu: dobradiças, puxador e eletrodutos (metálicos, lisos, sem textura).
static void ferragens(const Estado *e, Vector3 b) {
    posfx_superficie(e, 0.28f, 0.9f);
    float z = b.z + FRENTE + 0.024f;
    for (int k = 0; k < 3; k++) // laço de desenho: as três dobradiças, na borda esquerda da porta
        DrawCubeV((Vector3){b.x - 0.55f, 0.4f + k * 0.58f, z}, (Vector3){0.028f, 0.1f, 0.03f}, ACO);
    DrawCubeV((Vector3){b.x + 0.45f, 1.02f, z - 0.006f}, (Vector3){0.06f, 0.3f, 0.008f}, ESCURO); // espelho
    DrawCubeV((Vector3){b.x + 0.45f, 1.02f, z + 0.012f}, (Vector3){0.03f, 0.22f, 0.03f}, ACO);   // puxador
    for (int lado = -1; lado <= 1; lado += 2) { // laço de desenho: os dois eletrodutos até o teto
        Vector3 de = {b.x + lado * 0.36f, 1.84f, b.z - 0.08f}, ate = {de.x, PE_DIREITO, de.z};
        DrawCylinderEx(de, ate, 0.035f, 0.035f, 10, ACO);
        DrawCylinderEx((Vector3){de.x, 1.84f, de.z}, (Vector3){de.x, 1.92f, de.z}, 0.048f, 0.048f, 10,
                       ESCURO); // a luva na tampa
        DrawCylinderEx((Vector3){de.x, 2.45f, de.z}, (Vector3){de.x, 2.5f, de.z}, 0.045f, 0.045f, 10, ACO);
    }
    posfx_superficie(e, 0.45f, 0.0f);
    placa_perigo((Vector3){b.x - 0.12f, 0.72f, b.z + FRENTE + 0.024f}, 0.42f, 0.1f);
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
}

void painel_modelo_desenha(const Estado *e, Vector3 base, int variacao) {
    chapas(e, base, variacao);
    ferragens(e, base);
}
