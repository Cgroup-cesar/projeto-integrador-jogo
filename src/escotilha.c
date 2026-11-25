// escotilha.c — a escotilha no teto da sala de observação e a escada de marinheiro até ela, ao lado dos
// caixotes (atrás de quem está no console). Só cenário, em código (sem modelo): o jogador não sobe (a escada
// tem colisão) e nada aqui é interativo. A tampa só abre na cena do final bom (regras_cena_escotilha): ele
// sobe, empurra, ela emperra, cede e abre, e o clarão lá de fora desce pelo vão (cena_final.c).
#include "escotilha.h"
#include "posfx.h"
#include "texturas.h"

static const Color COR_FERRO = {0x2a, 0x2e, 0x2b, 255}; // ferro pintado de escuro, gasto
static const Color COR_TAMPA = {0x3a, 0x40, 0x36, 255}; // verde-oliva do Exército

static void ferro(const Estado *e, Vector3 centro, Vector3 tamanho) {
    texturas_caixa(e->rec.material[MATERIAL_METAL], centro, tamanho, texturas_modo(MATERIAL_METAL, 7),
                   COR_FERRO);
}

void escotilha_adiciona_paredes(ColParedes *paredes) {
    Vector3 c = {ESCOTILHA_X, PE_DIREITO * 0.5f, 13.8f}, meio = {0.3f, PE_DIREITO * 0.5f, 0.2f};
    col_paredes_adiciona(paredes, (Parede){Vector3Subtract(c, meio), Vector3Add(c, meio), PAREDE_MOVEL});
}

// A laje do teto em quatro peças em volta do vão (a UV é de mundo: as emendas não aparecem) e, acima dele, o
// poço curto de concreto, fechado em cima.
void escotilha_desenha_teto(const Estado *e) {
    float largura = MAPA_LARGURA * TAMANHO_CELULA, fundo = MAPA_ALTURA * TAMANHO_CELULA;
    float meio = ESCOTILHA_LADO_M * 0.5f, x0 = ESCOTILHA_X - meio, x1 = ESCOTILHA_X + meio;
    float z0 = ESCOTILHA_Z - meio, z1 = ESCOTILHA_Z + meio, y = PE_DIREITO + 0.05f;
    ModoUV uv = texturas_modo(MATERIAL_TETO, 0);
    const Vector3 pecas[4][2] = {
        // centro e tamanho: ao norte, ao sul, a oeste e a leste do vão
        {{largura * 0.5f, y, z0 * 0.5f}, {largura, 0.1f, z0}},
        {{largura * 0.5f, y, (z1 + fundo) * 0.5f}, {largura, 0.1f, fundo - z1}},
        {{x0 * 0.5f, y, ESCOTILHA_Z}, {x0, 0.1f, ESCOTILHA_LADO_M}},
        {{(x1 + largura) * 0.5f, y, ESCOTILHA_Z}, {largura - x1, 0.1f, ESCOTILHA_LADO_M}}};
    for (int i = 0; i < 4; i++) // padrão MAP: cada peça da laje
        texturas_caixa_mat(e, MATERIAL_TETO, pecas[i][0], pecas[i][1], uv, WHITE);
    float parede = ESCOTILHA_POCO_M - 0.1f, alto = PE_DIREITO + 0.1f + parede * 0.5f,
          lado = ESCOTILHA_LADO_M + 0.1f;
    ModoUV concreto = texturas_modo(MATERIAL_CONCRETO, 41);
    const Vector3 poco[5][2] = {
        // as paredes do poço (sobre a laje: nada no mesmo plano da borda dela) e o fundo
        {{ESCOTILHA_X, alto, z0 - 0.025f}, {lado, parede, 0.05f}},
        {{ESCOTILHA_X, alto, z1 + 0.025f}, {lado, parede, 0.05f}},
        {{x0 - 0.025f, alto, ESCOTILHA_Z}, {0.05f, parede, lado}},
        {{x1 + 0.025f, alto, ESCOTILHA_Z}, {0.05f, parede, lado}},
        {{ESCOTILHA_X, PE_DIREITO + ESCOTILHA_POCO_M, ESCOTILHA_Z}, {lado, 0.02f, lado}}};
    for (int i = 0; i < 5; i++) // padrão MAP: cada parede do poço
        texturas_caixa_mat(e, MATERIAL_CONCRETO, poco[i][0], poco[i][1], concreto,
                           (Color){0x9a, 0x9a, 0x94, 255});
}

// A escada de marinheiro presa na parede sul: dois trilhos, os degraus e dois suportes até a parede.
static void desenha_escada(const Estado *e) {
    float meia = ESCADA_LARGURA_M * 0.5f;
    for (int lado = -1; lado <= 1; lado += 2) { // padrão MAP: os dois trilhos e os seus suportes
        float x = ESCOTILHA_X + lado * meia;
        ferro(e, (Vector3){x, PE_DIREITO * 0.5f, ESCADA_PLANO_Z}, (Vector3){0.05f, PE_DIREITO, 0.05f});
        ferro(e, (Vector3){x, 0.5f, ESCADA_PLANO_Z + 0.09f}, (Vector3){0.04f, 0.04f, 0.18f});
        ferro(e, (Vector3){x, PE_DIREITO - 0.5f, ESCADA_PLANO_Z + 0.09f}, (Vector3){0.04f, 0.04f, 0.18f});
    }
    for (float y = ESCADA_DEGRAU_M; y < PE_DIREITO - 0.2f; y += ESCADA_DEGRAU_M) // padrão MAP: cada degrau
        ferro(e, (Vector3){ESCOTILHA_X, y, ESCADA_PLANO_Z}, (Vector3){ESCADA_LARGURA_M, 0.04f, 0.04f});
}

// O aro de ferro em volta do vão e a tampa, girada na dobradiça do lado sul (a = 0: fechada, rente ao teto).
static void desenha_tampa(const Estado *e, float abertura) {
    float meio = ESCOTILHA_LADO_M * 0.5f, y = PE_DIREITO - 0.03f;
    ferro(e, (Vector3){ESCOTILHA_X, y, ESCOTILHA_Z - meio - 0.03f},
          (Vector3){ESCOTILHA_LADO_M + 0.12f, 0.06f, 0.06f});
    ferro(e, (Vector3){ESCOTILHA_X, y, ESCOTILHA_Z + meio + 0.03f},
          (Vector3){ESCOTILHA_LADO_M + 0.12f, 0.06f, 0.06f});
    ferro(e, (Vector3){ESCOTILHA_X - meio - 0.03f, y, ESCOTILHA_Z},
          (Vector3){0.06f, 0.06f, ESCOTILHA_LADO_M});
    ferro(e, (Vector3){ESCOTILHA_X + meio + 0.03f, y, ESCOTILHA_Z},
          (Vector3){0.06f, 0.06f, ESCOTILHA_LADO_M});
    float a = ESCOTILHA_ABRE_GRAUS * DEG2RAD * abertura;
    Vector3 dobradica = {ESCOTILHA_X, PE_DIREITO - 0.005f, ESCOTILHA_Z + meio};
    Vector3 ao_longo = {0.0f, sinf(a) * ESCOTILHA_LADO_M, -cosf(a) * ESCOTILHA_LADO_M};
    Vector3 centro = Vector3Add(dobradica, Vector3Scale(ao_longo, 0.5f)), largura = {ESCOTILHA_LADO_M, 0, 0};
    Texture2D chapa = e->rec.material[MATERIAL_METAL];
    texturas_quadro(chapa, centro, largura, ao_longo, COR_TAMPA); // as duas faces da chapa
    texturas_quadro(chapa, centro, Vector3Negate(largura), ao_longo, COR_TAMPA);
    Vector3 ponta = {ESCOTILHA_X + meio, dobradica.y, dobradica.z},
            outra = {ESCOTILHA_X - meio, dobradica.y, dobradica.z};
    DrawCylinderEx(outra, ponta, 0.025f, 0.025f, 8, COR_FERRO); // a dobradiça
}

void escotilha_desenha(const Estado *e) {
    float abertura = e->fase == FASE_FIM_BOM ? regras_cena_escotilha(e->fim.tempo) : 0.0f;
    posfx_material(e, MATERIAL_METAL); // um material só para a escada, o aro e a tampa
    desenha_escada(e);
    desenha_tampa(e, abertura);
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
}
