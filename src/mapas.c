// mapas.c — os mapas PBR dos materiais (ver mapas.h). Cada pixel sai de materiais_pixel (função pura): a cor
// vai para o albedo, a altura vira normal (diferenças centrais) e oclusão (o quanto o ponto está abaixo da
// média em volta), e a rugosidade e o metal vão direto para o ORM.
#include "mapas.h"
#include <math.h>
#include <stdlib.h>

// ------------------------------------------------------------------ cores da paleta
typedef struct {
    Color base, secundaria; // a mistura do pixel vai de uma à outra
} CoresMaterial;

static const Color COR_TIJOLO = {0x4a, 0x30, 0x26, 255}; // onde a parede descascou até o tijolo

static CoresMaterial cores(TipoMaterial m) {
    switch (m) {
    case MATERIAL_CONCRETO: // tinta clara, verde-acinzentada; por baixo, o reboco úmido e escuro
        return (CoresMaterial){{0x8c, 0x9c, 0x92, 255}, {0x24, 0x28, 0x24, 255}};
    case MATERIAL_AZULEJO: // esmalte verde-água; sujeira e rejunte escuros
        return (CoresMaterial){{0x5c, 0x7e, 0x76, 255}, {0x1c, 0x22, 0x20, 255}};
    case MATERIAL_PISO:
        return (CoresMaterial){{0x24, 0x2b, 0x26, 255}, {0x17, 0x16, 0x12, 255}};
    case MATERIAL_TETO: // a laje pintada; a mancha de infiltração, escura
        return (CoresMaterial){{0x5e, 0x68, 0x62, 255}, {0x1e, 0x22, 0x1e, 255}};
    case MATERIAL_METAL:
        return (CoresMaterial){{0xf0, 0xf0, 0xf0, 255}, {0xa0, 0x5a, 0x34, 255}}; // neutro; ferrugem
    default:
        return (CoresMaterial){{0x35, 0x38, 0x2a, 255}, {0x44, 0x2c, 0x1c, 255}}; // porta
    }
}

static unsigned char canal(float valor) {
    return (unsigned char)Clamp(valor, 0.0f, 255.0f);
}

static Color cor_do_pixel(CoresMaterial c, PixelMaterial p) {
    float t = Clamp(p.mistura, 0.0f, 1.0f), k = Clamp(p.tijolo, 0.0f, 1.0f);
    return (Color){canal(Lerp(Lerp(c.base.r, c.secundaria.r, t), COR_TIJOLO.r, k) * p.brilho),
                   canal(Lerp(Lerp(c.base.g, c.secundaria.g, t), COR_TIJOLO.g, k) * p.brilho),
                   canal(Lerp(Lerp(c.base.b, c.secundaria.b, t), COR_TIJOLO.b, k) * p.brilho), 255};
}

// ------------------------------------------------------------------ escala do relevo
float mapas_metros(TipoMaterial m) {
    switch (m) {
    case MATERIAL_CONCRETO:
        return TEXTURA_METROS_CONCRETO;
    case MATERIAL_AZULEJO:
        return PE_DIREITO;
    case MATERIAL_PISO:
        return TEXTURA_METROS_PISO;
    case MATERIAL_TETO:
        return TEXTURA_METROS_TETO;
    default:
        return TEXTURA_METROS_METAL;
    }
}

// A diferença entre o fundo (altura 0) e a face (altura 1), em metros: o rejunte do azulejo é raso; o
// buraco no reboco, fundo.
static float profundidade(TipoMaterial m) {
    switch (m) {
    case MATERIAL_CONCRETO:
        return 0.007f;
    case MATERIAL_AZULEJO:
        return 0.004f;
    case MATERIAL_PISO:
        return 0.004f;
    case MATERIAL_TETO:
        return 0.005f;
    case MATERIAL_METAL:
        return 0.0015f;
    default:
        return 0.006f;
    }
}

// ------------------------------------------------------------------ altura → normal e oclusão
static float amostra(const float *a, int w, int h, int x, int y) {
    x = ((x % w) + w) % w; // dá a volta: a textura repete sem emenda
    y = ((y % h) + h) % h;
    return a[y * w + x];
}

// Média da altura numa janela de (2r+1)² pixels: dois passes de caixa (linha, depois coluna).
static void borra(const float *de, float *para, float *tmp, int w, int h, int r) {
    for (int y = 0; y < h; y++)     // carregamento: média por linha (não é map/filter/fold)
        for (int x = 0; x < w; x++) {
            float s = 0.0f;
            for (int k = -r; k <= r; k++)
                s += amostra(de, w, h, x + k, y);
            tmp[y * w + x] = s / (float)(2 * r + 1);
        }
    for (int y = 0; y < h; y++) // carregamento: média por coluna
        for (int x = 0; x < w; x++) {
            float s = 0.0f;
            for (int k = -r; k <= r; k++)
                s += amostra(tmp, w, h, x, y + k);
            para[y * w + x] = s / (float)(2 * r + 1);
        }
}

static void normal_e_oclusao(const float *alt, Color *normal, Color *orm, int w, int h, float forca) {
    float *media = malloc(sizeof(float) * (size_t)w * (size_t)h);
    float *tmp = malloc(sizeof(float) * (size_t)w * (size_t)h);
    if (media != NULL && tmp != NULL) borra(alt, media, tmp, w, h, w / 128 > 2 ? w / 128 : 2);
    for (int y = 0; y < h; y++) // padrão MAP: cada pixel → a normal e a oclusão
        for (int x = 0; x < w; x++) {
            float dx = (amostra(alt, w, h, x + 1, y) - amostra(alt, w, h, x - 1, y)) * 0.5f * forca;
            float dy = (amostra(alt, w, h, x, y + 1) - amostra(alt, w, h, x, y - 1)) * 0.5f * forca;
            Vector3 n = Vector3Normalize((Vector3){-dx, -dy, 1.0f});
            normal[y * w + x] = (Color){canal((n.x * 0.5f + 0.5f) * 255.0f),
                                        canal((n.y * 0.5f + 0.5f) * 255.0f),
                                        canal((n.z * 0.5f + 0.5f) * 255.0f), 255};
            float fundo = media != NULL ? media[y * w + x] - alt[y * w + x] : 0.0f; // > 0: abaixo do entorno
            orm[y * w + x].r = canal(255.0f * Clamp(1.0f - 2.2f * fundo, 0.25f, 1.0f));
        }
    free(media);
    free(tmp);
}

// ------------------------------------------------------------------ gerar
MapasMaterial mapas_gera(TipoMaterial m, int largura, int altura) {
    MapasMaterial r = {GenImageColor(largura, altura, BLANK), GenImageColor(largura, altura, BLANK),
                       GenImageColor(largura, altura, BLANK)};
    Color *cor = (Color *)r.cor.data, *orm = (Color *)r.orm.data;
    float *alt = malloc(sizeof(float) * (size_t)largura * (size_t)altura);
    CoresMaterial c = cores(m);
    // padrão MAP: cada pixel (u, v) → a cor, a altura, a rugosidade e o metal do material
    for (int y = 0; y < altura; y++)
        for (int x = 0; x < largura; x++) {
            float u = (x + 0.5f) / (float)largura, v = (y + 0.5f) / (float)altura;
            PixelMaterial p = materiais_pixel(m, u, v, TEXTURA_SUJEIRA, TEXTURA_SEMENTE);
            int i = y * largura + x;
            cor[i] = cor_do_pixel(c, p);
            orm[i] = (Color){255, canal(p.rugosidade * 255.0f), canal(p.metal * 255.0f), 255};
            if (alt != NULL) alt[i] = p.altura;
        }
    // força do relevo: a profundidade em metros dividida pelo tamanho de um pixel em metros
    float forca = profundidade(m) / (mapas_metros(m) / (float)largura);
    if (alt != NULL) normal_e_oclusao(alt, (Color *)r.normal.data, orm, largura, altura, forca);
    else ImageClearBackground(&r.normal, (Color){128, 128, 255, 255});
    free(alt);
    return r;
}

MapasMaterial mapas_lisos(float rugosidade) {
    MapasMaterial r = {{0}, GenImageColor(4, 4, (Color){128, 128, 255, 255}),
                       GenImageColor(4, 4, (Color){255, canal(rugosidade * 255.0f), 0, 255})};
    return r;
}
