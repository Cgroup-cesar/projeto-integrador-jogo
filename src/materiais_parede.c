// materiais_parede.c — as paredes e o teto como funções puras (declaradas em materiais.h), na direção de arte
// de docs/analise_referencias.md: azulejo esmaltado embaixo, tinta descascando em cima, a laje do teto com o
// mesmo descascado. Não inclui raylib.h: testado sem janela em tests/test_regras.c.
#include "materiais.h"
#include <math.h>

static float fracao(float x) {
    return x - floorf(x);
}

// ================================================================== auxiliares compartilhados
float materiais_passo(float a, float b, float x) {
    float t = Clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Voronoi periódico (nx × ny células, um ponto sorteado por célula): f1 e f2 são as distâncias (em células)
// ao ponto mais próximo e ao segundo; `id` sorteia um número por célula (para variar cada lasca).
void materiais_voronoi(float u, float v, int nx, int ny, unsigned s, float *f1, float *f2, float *id) {
    float gx = u * (float)nx, gy = v * (float)ny;
    int cx = (int)floorf(gx), cy = (int)floorf(gy);
    float d1 = 9.0f, d2 = 9.0f, qual = 0.0f;
    for (int j = -1; j <= 1; j++) // padrão FOLD: as 9 células vizinhas → as duas menores distâncias
        for (int i = -1; i <= 1; i++) {
            int x = cx + i, y = cy + j;
            int px = ((x % nx) + nx) % nx, py = ((y % ny) + ny) % ny; // periódico: emenda nas bordas
            float ox = (float)x + materiais_hash(px, py, s), oy = (float)y + materiais_hash(px, py, s + 1u);
            float d = sqrtf((gx - ox) * (gx - ox) + (gy - oy) * (gy - oy));
            if (d < d1) {
                d2 = d1;
                d1 = d;
                qual = materiais_hash(px, py, s + 2u);
            } else if (d < d2) {
                d2 = d;
            }
        }
    *f1 = d1;
    *f2 = d2;
    *id = qual;
}

// ================================================================== azulejo (a metade de baixo)
// 15 cm por peça: a textura cobre PE_DIREITO (3 m), então são 20 × 20 peças.
#define AZULEJO_PECAS 20

// Azulejo esmaltado: cada peça com o seu tom, a sua inclinação (o reflexo muda de uma para a outra, como na
// referência) e o seu brilho; rejunte escuro e afundado; peças encardidas pelos escorridos, trincadas,
// lascadas e algumas faltando (o reboco escuro aparece no buraco).
PixelMaterial materiais_azulejo(float u, float v, float sujeira, unsigned s) {
    float gx = u * AZULEJO_PECAS, gy = v * AZULEJO_PECAS;
    int px = (int)floorf(gx), py = (int)floorf(gy);
    float lx = fracao(gx), ly = fracao(gy);
    float borda = fminf(fminf(lx, 1.0f - lx), fminf(ly, 1.0f - ly));
    float rejunte = 1.0f - materiais_passo(0.01f, 0.026f, borda);
    float sorte = materiais_hash(px, py, s), sorte2 = materiais_hash(px, py, s + 1u);
    bool falta = sorte < 0.015f * sujeira * 1.25f;            // peça que caiu
    bool trincada = !falta && sorte2 < 0.12f * sujeira * 1.25f; // peça com trinca
    // cada peça com o seu tom, e uma variação suave dentro dela
    float brilho = 0.84f + 0.3f * sorte2 + 0.1f * (materiais_fbm(u, v, 40, 40, 2, s + 3u) - 0.5f);
    float escorre = materiais_escorridos(u, v, 60, s + 5u) * sujeira;
    float encardido = materiais_passo(0.45f, 0.85f, materiais_fbm(u, v, 6, 6, 4, s + 7u)) * sujeira;
    float trinca = trincada ? materiais_rachadura(u, v, 10, 10, s + 9u) : 0.0f;
    float qx = fminf(lx, 1.0f - lx), qy = fminf(ly, 1.0f - ly); // a quina mais perto
    bool lascada = materiais_hash(px, py, s + 11u) < 0.25f;
    float lasca = lascada ? 1.0f - materiais_passo(0.06f, 0.1f, qx + qy) : 0.0f;
    // a inclinação: um plano levemente torto por peça (a normal tirada da altura entorta o reflexo)
    float inclina = 0.06f * ((lx - 0.5f) * (sorte - 0.5f) + (ly - 0.5f) * (sorte2 - 0.5f));
    float altura = 0.85f + inclina - 0.65f * rejunte - 0.35f * trinca - 0.3f * lasca;
    float rug = 0.1f + 0.12f * sorte2 + 0.45f * encardido + 0.3f * escorre + 0.7f * rejunte;
    float mistura = fmaxf(0.55f * rejunte, fmaxf(0.55f * encardido, 0.8f * escorre));
    brilho *= (1.0f - 0.5f * trinca) * (1.0f - 0.3f * lasca);
    if (falta) { // o buraco: reboco escuro, áspero e fundo, com a marca da cola
        float cola = materiais_passo(0.4f, 0.7f, materiais_fbm(u, v, 60, 60, 2, s + 13u));
        return (PixelMaterial){0.7f + 0.3f * cola, 1.0f, 0.0f, 0.25f + 0.1f * cola, 0.95f, 0.0f};
    }
    return (PixelMaterial){brilho, Clamp(mistura, 0.0f, 1.0f), 0.0f, Clamp(altura, 0.0f, 1.0f),
                           Clamp(rug, 0.0f, 1.0f), 0.0f};
}

// ================================================================== tinta descascando (a metade de cima)
// Tijolo (12 fiadas na textura, meia-vez desencontradas): 1 no tijolo, 0 na argamassa esfarelada.
static float tijolo(float u, float v, unsigned s) {
    float fiada = v * 24.0f;
    float deslocado = u * 24.0f + (((int)floorf(fiada) & 1) ? 0.5f : 0.0f);
    float bu = fracao(deslocado), bv = fracao(fiada);
    float borda = fminf(fminf(bu, 1.0f - bu) * 2.0f, fminf(bv, 1.0f - bv));
    float gasta = 0.05f * (materiais_fbm(u, v, 48, 48, 2, s) - 0.5f);
    return materiais_passo(0.03f, 0.08f, borda + gasta);
}

// Quanto a tinta já soltou (0 = inteira, 1 = caiu). As manchas grandes vêm de um FBM com o domínio entortado
// e serrilhado (o contorno torto e quebrado); perto da borda de cada mancha, a tinta solta em lascas (as
// células de um Voronoi): cada lasca cai inteira ou fica, conforme o sorteio dela e o quanto a mancha já
// avançou ali. A umidade (`umido`: perto do teto e do chão) faz descascar mais.
static float descasca(float u, float v, int fx, int fy, float umido, float limiar, unsigned s) {
    float wu = u + 0.04f * (materiais_fbm(u, v, fx * 3, fy * 3, 2, s + 1u) - 0.5f);
    float wv = v + 0.04f * (materiais_fbm(u, v, fx * 3, fy * 3, 2, s + 2u) - 0.5f);
    float p = materiais_fbm(wu, wv, fx, fy, 5, s) + 0.22f * umido;
    p += 0.035f * (materiais_fbm(u, v, fx * 24, fy * 24, 2, s + 5u) - 0.5f); // serrilhado da borda
    float mancha = materiais_passo(limiar, limiar + 0.005f, p);
    float f1, f2, id;
    materiais_voronoi(u, v, fx * 14, fy * 14, s + 3u, &f1, &f2, &id);
    float avanco = Clamp((p - (limiar - 0.06f)) / 0.06f, 0.0f, 1.0f); // 0 longe da mancha, 1 na borda dela
    float lasca = avanco > 0.0f && id < avanco * 0.7f ? 1.0f : 0.0f;
    (void)f1;
    (void)f2;
    return fmaxf(mancha, lasca);
}

// Reboco pintado (6 m × 3 m; v = 0 no teto, v = 1 no chão — a parte de baixo fica atrás do azulejo): a tinta
// clara descasca em placas grandes e deixa a camada escura (reboco úmido, mofo) à mostra; a borda da tinta
// levanta (mais clara) e faz sombra; nos buracos mais fundos o reboco caiu até o tijolo, com óxido em volta.
// Por cima de tudo: rachaduras ramificadas e escorridos pretos verticais. A mistura leva à camada escura.
PixelMaterial materiais_tinta(float u, float v, float sujeira, unsigned s) {
    float umido = materiais_passo(0.35f, 0.0f, v) * 0.8f + materiais_passo(0.55f, 0.9f, v) * 0.5f;
    float limiar = 0.78f - 0.12f * sujeira;
    float caiu = descasca(u, v, 6, 3, umido, limiar, s);
    // a borda que levantou e a sombra dela: a mesma máscara, deslocada meio milímetro para cima e para baixo
    float acima = descasca(u, v - 0.0012f, 6, 3, umido, limiar, s);
    float labio = fmaxf(0.0f, acima - caiu), sombra = fmaxf(0.0f, caiu - acima);
    float p = materiais_fbm(u, v, 6, 3, 5, s) + 0.22f * umido; // o quanto fundo o buraco foi
    float fundo = caiu * materiais_passo(limiar + 0.1f, limiar + 0.13f, p) *
                  materiais_passo(0.45f, 0.6f, materiais_fbm(u, v, 12, 6, 2, s + 25u));
    float tb = tijolo(u, v, s + 27u);
    float rach = sujeira * materiais_rachadura(u, v, 6, 3, s + 4u) *
                 materiais_passo(0.45f, 0.6f, materiais_fbm(u, v, 4, 2, 3, s + 31u));
    float escorre = materiais_escorridos(u, v, 128, s + 7u) * sujeira;
    // tinta: tom gasto e manchas de água; camada escura: reboco úmido, áspero
    float brilho = 0.78f + 0.35f * materiais_fbm(u, v, 12, 6, 4, s + 11u);
    brilho *= 0.9f + 0.2f * materiais_fbm(u, v, 96, 48, 2, s + 12u) * (0.4f + 0.6f * caiu);
    brilho *= (1.0f + 0.25f * labio) * (1.0f - 0.45f * sombra);
    brilho *= (1.0f - 0.55f * rach) * (1.0f - 0.6f * escorre);
    brilho *= 1.0f - 0.3f * umido * (1.0f - caiu); // a tinta úmida escurece antes de cair
    float tij = 0.8f * fundo * tb;
    brilho *= 1.0f - 0.5f * fundo * (1.0f - tb); // a argamassa entre os tijolos, funda e escura
    float altura = 1.0f - 0.4f * caiu - 0.3f * fundo - 0.15f * fundo * (1.0f - tb);
    altura += 0.08f * labio - 0.5f * rach;
    float rug = Lerp(0.62f, 0.93f, caiu) - 0.25f * escorre - 0.15f * umido * (1.0f - caiu);
    brilho *= 0.94f + 0.12f * materiais_fbm(u, v, 30, 15, 2, s + 13u) * (1.0f - caiu); // a tinta gasta
    float mistura = Clamp(caiu + 0.5f * escorre, 0.0f, 1.0f);
    return (PixelMaterial){brilho, mistura, tij, Clamp(altura, 0.0f, 1.0f), Clamp(rug, 0.0f, 1.0f), 0.0f};
}

// ================================================================== laje do teto
// A laje pintada (2 m): o mesmo descascado, em placas maiores e mais escuras, e manchas de infiltração com o
// anel da água que secou.
PixelMaterial materiais_laje(float u, float v, float sujeira, unsigned s) {
    float limiar = 0.8f - 0.1f * sujeira;
    float caiu = descasca(u, v, 3, 3, 0.35f, limiar, s + 41u);
    float labio = fmaxf(0.0f, descasca(u, v - 0.002f, 3, 3, 0.35f, limiar, s + 41u) - caiu);
    float f = materiais_fbm(u, v, 2, 2, 5, s + 43u);
    float mancha = materiais_passo(0.58f, 0.66f, f) * sujeira;
    float anel = fmaxf(0.0f, materiais_passo(0.55f, 0.58f, f) - materiais_passo(0.58f, 0.61f, f)) * sujeira;
    float brilho = (0.85f + 0.3f * materiais_fbm(u, v, 8, 8, 3, s + 45u)) * (1.0f + 0.2f * labio) *
                   (1.0f - 0.35f * anel) * (1.0f - 0.25f * mancha);
    float altura = 1.0f - 0.45f * caiu + 0.08f * labio;
    float mistura = Clamp(caiu + 0.6f * mancha + anel, 0.0f, 1.0f);
    return (PixelMaterial){brilho, mistura, 0.0f, Clamp(altura, 0.0f, 1.0f), Lerp(0.75f, 0.95f, caiu), 0.0f};
}
