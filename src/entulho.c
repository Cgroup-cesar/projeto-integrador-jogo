// entulho.c — a sujeira do chão (ver entulho.h). Nada é guardado: a cada quadro, cada célula de chão
// refaz o mesmo sorteio (materiais_hash com a semente fixa) e desenha o que lhe coube. São ~200 células e
// contas baratas, então não vale uma tabela.
#include "entulho.h"
#include "nivel.h"
#include "texturas.h"
#include "rlgl.h"
#include <math.h>
#include <stdlib.h>

#define SEMENTE  0xe27u
#define METADE   (TAMANHO_CELULA * 0.5f)
#define CHAO_Y   0.004f // a poeira fica rente ao chão, abaixo das manchas d'água

// Os quatro lados de uma célula: o vizinho (coluna, linha) e para onde fica a parede.
static const int LADOS[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

static float sorteio(int coluna, int linha, unsigned k) {
    return materiais_hash(coluna * 7 + (int)k, linha * 13 + (int)k * 3, SEMENTE + k);
}

// Chão onde se anda e não há nada em cima (corredores, salas, Arquivo): '.' e a sala de observação.
static bool chao_livre(char c) {
    return c == '.' || c == 'O';
}

// Poeira acumulada no pé da parede: uma faixa que encosta na parede e some para o meio da célula.
static void poeira(const Estado *e, Vector3 centro, int dx, int dz, float f) {
    Vector3 ao_longo = {(float)abs(dz), 0.0f, (float)abs(dx)};
    Vector3 para_parede = {(float)dx, 0.0f, (float)dz};
    float largura = 0.3f + 0.35f * f, comprimento = METADE * (0.6f + 0.4f * f);
    Vector3 p = Vector3Add(centro, Vector3Scale(para_parede, METADE - largura));
    p = Vector3Add(p, Vector3Scale(ao_longo, (f - 0.5f) * METADE * 0.6f));
    p.y = CHAO_Y;
    // a frente do quadro (direita × cima) para cima; `cima` aponta para a parede (v = 0 encostado nela)
    Vector3 direita = Vector3Scale(ao_longo, comprimento), cima = Vector3Scale(para_parede, largura);
    if (Vector3CrossProduct(direita, cima).y < 0.0f) direita = Vector3Negate(direita);
    texturas_quadro(e->rec.decalque[DECALQUE_POEIRA], p, direita, cima, WHITE);
}

// Um caco: caixa pequena, achatada e girada. Metade é reboco (cinza), metade é azulejo que caiu da parede
// (verde-água, mais fino): `tom` sorteia qual e o quanto de sujeira.
static void caco(Vector3 p, float tamanho, float giro, float tom) {
    rlPushMatrix();
    rlTranslatef(p.x, tamanho * 0.25f, p.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    rlRotatef(giro * 0.37f, 1.0f, 0.0f, 0.0f); // não fica de pé nem deitado certinho
    if (tom < 0.5f) { // azulejo: o esmalte por cima, a peça fina
        float k = 0.7f + 0.6f * tom;
        DrawCubeV((Vector3){0}, (Vector3){tamanho, 0.008f, tamanho * 0.8f},
                  (Color){(unsigned char)(0x5c * k), (unsigned char)(0x7e * k), (unsigned char)(0x76 * k),
                          255});
    } else {
        unsigned char c = (unsigned char)(70.0f + 60.0f * (tom - 0.5f) * 2.0f);
        DrawCubeV((Vector3){0}, (Vector3){tamanho, tamanho * 0.45f, tamanho * 0.75f},
                  (Color){c, (unsigned char)(c * 0.98f), (unsigned char)(c * 0.9f), 255});
    }
    rlPopMatrix();
}

// Cacos de reboco que caíram da parede: um montinho encostado nela, maiores perto, miúdos espalhados.
static void montinho(Vector3 centro, int dx, int dz, int coluna, int linha, int lado) {
    int n = 4 + (int)(sorteio(coluna, linha, 20u + (unsigned)lado) * 8.0f);
    float ao_longo0 = (sorteio(coluna, linha, 21u + (unsigned)lado) - 0.5f) * METADE * 1.2f;
    for (int i = 0; i < n; i++) { // laço de desenho: cada caco do montinho
        unsigned k = 30u + (unsigned)(lado * 16 + i);
        float a = sorteio(coluna, linha, k), b = sorteio(coluna, linha, k + 100u);
        float longe = 0.05f + a * a * 0.55f; // a maioria colada na parede
        float ao_longo = ao_longo0 + (b - 0.5f) * (0.3f + longe);
        Vector3 p = {centro.x + dx * (METADE - longe) + dz * ao_longo, 0.0f,
                     centro.z + dz * (METADE - longe) + dx * ao_longo};
        caco(p, 0.025f + 0.09f * (1.0f - a) * b, 360.0f * b, sorteio(coluna, linha, k + 200u));
    }
}

// Uma folha de papel largada no chão, amarelada e um pouco erguida numa ponta.
static void papel(Vector3 p, float giro, float tom) {
    rlPushMatrix();
    rlTranslatef(p.x, 0.008f, p.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    rlRotatef(3.0f, 1.0f, 0.0f, 0.0f);
    unsigned char c = (unsigned char)(62.0f + 30.0f * tom); // papel velho, encardido
    DrawCubeV((Vector3){0}, (Vector3){0.21f, 0.002f, 0.297f}, (Color){c, (unsigned char)(c * 0.95f),
                                                                       (unsigned char)(c * 0.78f), 255});
    rlPopMatrix();
}

// Lasca de piso solta: uma placa quebrada, escura, meio fora do lugar.
static void lasca(Vector3 p, float giro) {
    rlPushMatrix();
    rlTranslatef(p.x, 0.012f, p.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    rlRotatef(6.0f, 0.0f, 0.0f, 1.0f);
    DrawCubeV((Vector3){0}, (Vector3){0.16f, 0.018f, 0.11f}, (Color){0x26, 0x2c, 0x27, 255});
    rlPopMatrix();
}

// `so_poeira`: a primeira passada (decalques, sem profundidade) só põe a poeira; a segunda, o resto.
static void desenha_celula(const Estado *e, int coluna, int linha, bool so_poeira) {
    Vector3 centro = nivel_centro_celula(coluna, linha);
    for (int lado = 0; lado < 4; lado++) { // laço de desenho: o que encosta em cada parede da célula
        int dx = LADOS[lado][0], dz = LADOS[lado][1];
        if (nivel_celula(&e->nivel, coluna + dx, linha + dz) != '#') continue;
        float f = sorteio(coluna, linha, (unsigned)lado);
        if (so_poeira && f < 0.7f) poeira(e, centro, dx, dz, f / 0.7f);
        if (!so_poeira && sorteio(coluna, linha, 10u + (unsigned)lado) < 0.45f)
            montinho(centro, dx, dz, coluna, linha, lado);
    }
    if (so_poeira) return;
    float x = (sorteio(coluna, linha, 50u) - 0.5f) * TAMANHO_CELULA * 0.7f;
    float z = (sorteio(coluna, linha, 51u) - 0.5f) * TAMANHO_CELULA * 0.7f;
    Vector3 solto = {centro.x + x, 0.0f, centro.z + z};
    if (sorteio(coluna, linha, 52u) < 0.2f)
        papel(solto, 360.0f * sorteio(coluna, linha, 53u), sorteio(coluna, linha, 54u));
    else if (sorteio(coluna, linha, 55u) < 0.25f)
        lasca(solto, 360.0f * sorteio(coluna, linha, 56u));
}

// [REQ:ENTULHO] poeira no pé das paredes, cacos de reboco, papéis e lascas de piso, sorteados por célula
static void percorre(const Estado *e, bool so_poeira) {
    for (int linha = 0; linha < MAPA_ALTURA; linha++)       // laço de desenho: célula por célula
        for (int coluna = 0; coluna < MAPA_LARGURA; coluna++) // (não é map/filter/fold)
            if (chao_livre(nivel_celula(&e->nivel, coluna, linha)))
                desenha_celula(e, coluna, linha, so_poeira);
}

void entulho_desenha(const Estado *e) {
    rlDrawRenderBatchActive();
    rlDisableDepthMask(); // a poeira é decalque: por cima do chão, sem esconder nada
    percorre(e, true);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    percorre(e, false);
}
