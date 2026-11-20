// texturas.c — texturas do mapa: geradas UMA vez ao abrir o jogo (o desenho de cada material vem das funções
// puras de materiais.c), trocadas por assets/texturas/<material>.png quando o arquivo existe, e liberadas ao
// sair. Também desenha com elas: caixa (6 faces, UV em metros) e quadro (decalque).
// Nada aqui roda por quadro além do desenho: nenhum GenImage, LoadTexture nem upload depois do carregamento.
#include "texturas.h"
#include "dialogo.h"
#include "rlgl.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>

// ------------------------------------------------------------------ cores e nomes
static const char *const NOMES[MATERIAL_TOTAL] = {"concreto", "oliva", "piso", "teto", "metal", "porta"};

typedef struct {
    Color base, secundaria; // a mistura do pixel vai de uma à outra
} CoresMaterial;

static CoresMaterial cores(TipoMaterial m) {
    switch (m) {
    case MATERIAL_CONCRETO:
        return (CoresMaterial){COR_CONCRETO, {0x1c, 0x22, 0x1b, 255}}; // umidade
    case MATERIAL_OLIVA:
        return (CoresMaterial){COR_OLIVA, {0x48, 0x4b, 0x44, 255}}; // reboco por baixo da tinta
    case MATERIAL_PISO:
        return (CoresMaterial){{0x24, 0x2b, 0x26, 255}, {0x17, 0x16, 0x12, 255}};
    case MATERIAL_TETO:
        return (CoresMaterial){{0x18, 0x1c, 0x19, 255}, {0x2a, 0x24, 0x16, 255}}; // infiltração
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
    float t = Clamp(p.mistura, 0.0f, 1.0f);
    return (Color){canal(Lerp(c.base.r, c.secundaria.r, t) * p.brilho),
                   canal(Lerp(c.base.g, c.secundaria.g, t) * p.brilho),
                   canal(Lerp(c.base.b, c.secundaria.b, t) * p.brilho), 255};
}

// ------------------------------------------------------------------ geração (só no carregamento)
static Image gera_material(TipoMaterial m) {
    int largura = m == MATERIAL_CONCRETO ? TEXTURA_CONCRETO_PX : TEXTURA_LADO_PX, altura = TEXTURA_LADO_PX;
    Image img = GenImageColor(largura, altura, BLANK);
    Color *px = (Color *)img.data;
    CoresMaterial c = cores(m);
    // padrão MAP: cada pixel (u, v) → a cor do material
    for (int y = 0; y < altura; y++)
        for (int x = 0; x < largura; x++) {
            float u = (x + 0.5f) / (float)largura, v = (y + 0.5f) / (float)altura;
            px[y * largura + x] = cor_do_pixel(c, materiais_pixel(m, u, v, TEXTURA_SUJEIRA, TEXTURA_SEMENTE));
        }
    return img;
}

// Placa em estêncil: o texto (dialogo.c) com a fonte do jogo, centralizado, com a tinta gasta.
static Image gera_estencil(const Recursos *rec, const char *texto, unsigned semente) {
    char limpo[64];
    translitera(texto, limpo, sizeof(limpo), rec->tem_fonte ? &rec->fonte : NULL);
    float tamanho = 96.0f, espaco = tamanho * 0.04f;
    Image fino = ImageTextEx(rec->fonte, limpo, tamanho, espaco, WHITE);
    // traço grosso de estêncil: o texto carimbado 4 vezes, deslocado 3 px
    Image letras = GenImageColor(fino.width + 3, fino.height + 3, BLANK);
    Rectangle origem = {0, 0, (float)fino.width, (float)fino.height};
    for (int k = 0; k < 4; k++) // carregamento: os 4 carimbos (não é map/filter/fold)
        ImageDraw(&letras, fino, origem,
                  (Rectangle){(k & 1) * 3.0f, (k >> 1) * 3.0f, origem.width, origem.height}, WHITE);
    UnloadImage(fino);
    Image placa = GenImageColor(DECALQUE_PX * 2, DECALQUE_PX / 2, BLANK);
    float escala = fminf(placa.width * 0.92f / letras.width, placa.height * 0.8f / letras.height);
    Rectangle destino = {(placa.width - letras.width * escala) * 0.5f,
                         (placa.height - letras.height * escala) * 0.5f, letras.width * escala,
                         letras.height * escala};
    ImageDraw(&placa, letras, (Rectangle){0, 0, (float)letras.width, (float)letras.height}, destino, WHITE);
    UnloadImage(letras);
    ImageFormat(&placa, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *px = (Color *)placa.data;
    // padrão MAP: o alfa de cada pixel multiplicado pelo desgaste da tinta
    for (int i = 0; i < placa.width * placa.height; i++) {
        float u = (i % placa.width + 0.5f) / placa.width, v = (i / placa.width + 0.5f) / placa.height;
        px[i].a = canal(px[i].a * materiais_desgaste(u, v, semente));
    }
    return placa;
}

// Mancha (umidade, arrasto): cor fixa, alfa da função pura.
static Image gera_mancha(Decalque d, Color cor, unsigned semente) {
    int largura = DECALQUE_PX, altura = d == DECALQUE_ARRASTO ? DECALQUE_PX / 2 : DECALQUE_PX;
    Image img = GenImageColor(largura, altura, BLANK);
    Color *px = (Color *)img.data;
    // padrão MAP: cada pixel → a cor com o alfa da mancha
    for (int i = 0; i < largura * altura; i++) {
        float u = (i % largura + 0.5f) / largura, v = (i / largura + 0.5f) / altura;
        px[i] = (Color){cor.r, cor.g, cor.b, canal(255.0f * materiais_mancha(d, u, v, semente))};
    }
    return img;
}

static bool potencia_de_2(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

static int potencia_abaixo(int n) {
    int p = 1;
    while (p * 2 <= n && p * 2 <= TEXTURA_ARQUIVO_MAX_PX)
        p *= 2;
    return p;
}

// Manda a imagem para a GPU uma vez (com mipmaps, para não cintilar de longe) e libera a imagem.
// Potência de 2 sempre: repetir textura e gerar mipmaps exige isso no OpenGL 2.1/ES.
static Texture2D sobe(Image img, bool repete) {
    if (!potencia_de_2(img.width) || !potencia_de_2(img.height) || img.width > TEXTURA_ARQUIVO_MAX_PX ||
        img.height > TEXTURA_ARQUIVO_MAX_PX)
        ImageResize(&img, potencia_abaixo(img.width), potencia_abaixo(img.height));
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    ImageMipmaps(&img);
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(t, repete ? TEXTURE_WRAP_REPEAT : TEXTURE_WRAP_CLAMP);
    return t;
}

// [REQ:RNF4] asset ausente: sem assets/texturas/<nome>.png, a textura é gerada em código (nunca falha)
static void carrega_material(Recursos *rec, TipoMaterial m) {
    char relativo[64], caminho[512];
    snprintf(relativo, sizeof(relativo), "texturas/%s.png", NOMES[m]);
    Image img = {0};
    if (ui_caminho_asset(relativo, caminho, sizeof(caminho))) img = LoadImage(caminho);
    rec->material_de_arquivo[m] = img.data != NULL;
    if (img.data != NULL)
        TraceLog(LOG_INFO, "PRECURSOR: textura %s carregada de %s", NOMES[m], caminho);
    else
        img = gera_material(m);
    rec->material[m] = sobe(img, true);
}

void texturas_carrega(Recursos *rec) {
    int procedurais = 0;
    for (int m = 0; m < MATERIAL_TOTAL; m++) { // carregamento: uma vez por material (não é map/filter/fold)
        carrega_material(rec, (TipoMaterial)m);
        procedurais += !rec->material_de_arquivo[m];
    }
    static const TextoId PLACAS[DECALQUES_ESTENCIL] = {TXT_PLACA_POSTO, TXT_PLACA_RESTRITA,
                                                       TXT_PLACA_ARQUIVO};
    for (int d = 0; d < DECALQUES_ESTENCIL; d++) // carregamento: uma placa por texto
        rec->decalque[d] =
            sobe(gera_estencil(rec, dialogo_texto(PLACAS[d]), TEXTURA_SEMENTE + (unsigned)d), false);
    rec->decalque[DECALQUE_UMIDADE] =
        sobe(gera_mancha(DECALQUE_UMIDADE, (Color){0x10, 0x13, 0x0d, 255}, TEXTURA_SEMENTE), false);
    rec->decalque[DECALQUE_ARRASTO] =
        sobe(gera_mancha(DECALQUE_ARRASTO, (Color){0x0c, 0x08, 0x06, 255}, TEXTURA_SEMENTE), false);
    rec->halo = sobe(GenImageGradientRadial(64, 64, 0.0f, WHITE, BLANK), false);
    TraceLog(LOG_INFO,
             "PRECURSOR: texturas do mapa prontas (%d de %d procedurais; override em assets/texturas/)",
             procedurais, MATERIAL_TOTAL);
}

static void libera(Texture2D *t) {
    if (t->id != 0) UnloadTexture(*t);
    *t = (Texture2D){0};
}

void texturas_descarrega(Recursos *rec) {
    for (int m = 0; m < MATERIAL_TOTAL; m++) // liberação: uma vez por textura (não é map/filter/fold)
        libera(&rec->material[m]);
    for (int d = 0; d < DECALQUE_TOTAL; d++)
        libera(&rec->decalque[d]);
    libera(&rec->halo);
}

