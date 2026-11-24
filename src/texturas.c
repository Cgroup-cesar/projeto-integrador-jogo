// texturas.c — texturas do mapa: geradas UMA vez ao abrir o jogo (o desenho de cada material vem das funções
// puras de materiais.c), trocadas por assets/texturas/<material>.png quando o arquivo existe, e liberadas ao
// sair. Também desenha com elas: caixa (6 faces, UV em metros) e quadro (decalque).
// Nada aqui roda por quadro além do desenho: nenhum GenImage, LoadTexture nem upload depois do carregamento.
#include "texturas.h"
#include "mapas.h"
#include "posfx.h"
#include "dialogo.h"
#include "rlgl.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>

// ------------------------------------------------------------------ cores e nomes
static const char *const NOMES[MATERIAL_TOTAL] = {"concreto", "oliva", "piso", "teto", "metal", "porta"};

static unsigned char canal(float valor) {
    return (unsigned char)Clamp(valor, 0.0f, 255.0f);
}

// Cola o recorte `origem` em `destino`, redimensionado para `alvo` e misturado pelo alfa (como o ImageDraw).
// Feito à mão porque o nome da função da raylib mudou (ImageDraw na 5.5 e na 6.0 do instalador do Windows,
// ImageDrawImagePro na 6.2) e o número da versão não diz qual existe. Só roda ao gerar as texturas.
static void desenha_imagem(Image *destino, Image origem_img, Rectangle origem, Rectangle alvo, Color tinta) {
    int w = (int)alvo.width, h = (int)alvo.height;
    if (w <= 0 || h <= 0) return;
    Image pedaco = ImageFromImage(origem_img, origem);
    ImageResize(&pedaco, w, h);
    for (int y = 0; y < h; y++) // carregamento: cópia pixel a pixel (não é map/filter/fold)
        for (int x = 0; x < w; x++) {
            int dx = (int)alvo.x + x, dy = (int)alvo.y + y;
            if (dx < 0 || dy < 0 || dx >= destino->width || dy >= destino->height) continue;
            Color fundo = GetImageColor(*destino, dx, dy);
            ImageDrawPixel(destino, dx, dy, ColorAlphaBlend(fundo, GetImageColor(pedaco, x, y), tinta));
        }
    UnloadImage(pedaco);
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
        desenha_imagem(&letras, fino, origem,
                       (Rectangle){(k & 1) * 3.0f, (k >> 1) * 3.0f, origem.width, origem.height}, WHITE);
    UnloadImage(fino);
    Image placa = GenImageColor(DECALQUE_PX * 2, DECALQUE_PX / 2, BLANK);
    float escala = fminf(placa.width * 0.92f / letras.width, placa.height * 0.8f / letras.height);
    Rectangle destino = {(placa.width - letras.width * escala) * 0.5f,
                         (placa.height - letras.height * escala) * 0.5f, letras.width * escala,
                         letras.height * escala};
    desenha_imagem(&placa, letras, (Rectangle){0, 0, (float)letras.width, (float)letras.height}, destino,
                   WHITE);
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

// assets/texturas/<nome><sufixo>.png, se existir (sem ele, uma imagem vazia).
static Image do_arquivo(TipoMaterial m, const char *sufixo) {
    char relativo[64], caminho[512];
    snprintf(relativo, sizeof(relativo), "texturas/%s%s.png", NOMES[m], sufixo);
    Image img = {0};
    if (ui_caminho_asset(relativo, caminho, sizeof(caminho))) img = LoadImage(caminho);
    if (img.data != NULL)
        TraceLog(LOG_INFO, "PRECURSOR: textura %s%s carregada de %s", NOMES[m], sufixo, caminho);
    return img;
}

// [REQ:RNF4] asset ausente: sem assets/texturas/<nome>.png, os mapas são gerados em código (nunca falha).
// Com o arquivo, ele é a cor; a normal e o ORM vêm de <nome>_normal.png e <nome>_orm.png, ou ficam lisos.
static void carrega_material(Recursos *rec, TipoMaterial m) {
    Image cor = do_arquivo(m, "");
    rec->material_de_arquivo[m] = cor.data != NULL;
    MapasMaterial mapas;
    if (cor.data != NULL) {
        mapas = mapas_lisos(0.7f);
        mapas.cor = cor;
        Image normal = do_arquivo(m, "_normal"), orm = do_arquivo(m, "_orm");
        if (normal.data != NULL) UnloadImage(mapas.normal), mapas.normal = normal;
        if (orm.data != NULL) UnloadImage(mapas.orm), mapas.orm = orm;
    } else {
        int largura = m == MATERIAL_CONCRETO ? TEXTURA_CONCRETO_PX : TEXTURA_LADO_PX;
        mapas = mapas_gera(m, largura, TEXTURA_LADO_PX);
    }
    rec->material[m] = sobe(mapas.cor, true);
    rec->material_normal[m] = sobe(mapas.normal, true);
    rec->material_orm[m] = sobe(mapas.orm, true);
}

void texturas_carrega(Recursos *rec) {
    int procedurais = 0;
    for (int m = 0; m < MATERIAL_TOTAL; m++) { // carregamento: uma vez por material (não é map/filter/fold)
        carrega_material(rec, (TipoMaterial)m);
        procedurais += !rec->material_de_arquivo[m];
    }
    static const TextoId PLACAS[DECALQUES_ESTENCIL] = {TXT_PLACA_POSTO, TXT_PLACA_RESTRITA, TXT_PLACA_ARQUIVO,
                                                       TXT_PLACA_CAIXA_1, TXT_PLACA_CAIXA_2};
    for (int d = 0; d < DECALQUES_ESTENCIL; d++) // carregamento: uma placa por texto
        rec->decalque[d] =
            sobe(gera_estencil(rec, dialogo_texto(PLACAS[d]), TEXTURA_SEMENTE + (unsigned)d), false);
    rec->decalque[DECALQUE_UMIDADE] =
        sobe(gera_mancha(DECALQUE_UMIDADE, (Color){0x10, 0x13, 0x0d, 255}, TEXTURA_SEMENTE), false);
    rec->decalque[DECALQUE_ARRASTO] =
        sobe(gera_mancha(DECALQUE_ARRASTO, (Color){0x0c, 0x08, 0x06, 255}, TEXTURA_SEMENTE), false);
    rec->decalque[DECALQUE_POEIRA] =
        sobe(gera_mancha(DECALQUE_POEIRA, (Color){0x14, 0x12, 0x0e, 255}, TEXTURA_SEMENTE + 9u), false);
    for (int d = DECALQUE_MOLHADO; d <= DECALQUE_POCA; d++) // carregamento: as manchas de água
        rec->decalque[d] =
            sobe(gera_mancha((Decalque)d, (Color){0x05, 0x07, 0x08, 255}, TEXTURA_SEMENTE + 7u), false);
    char caminho[512]; // o sangue só existe como arquivo: sem ele, a textura fica vazia e não é desenhada
    if (ui_caminho_asset("texturas/sangue.png", caminho, sizeof(caminho))) {
        Image sangue = LoadImage(caminho);
        if (sangue.data != NULL) rec->decalque[DECALQUE_SANGUE] = sobe(sangue, false);
    }
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
    for (int m = 0; m < MATERIAL_TOTAL; m++) { // liberação: os três mapas de cada material
        libera(&rec->material[m]);
        libera(&rec->material_normal[m]);
        libera(&rec->material_orm[m]);
    }
    for (int d = 0; d < DECALQUE_TOTAL; d++)
        libera(&rec->decalque[d]);
    libera(&rec->halo);
}

// ------------------------------------------------------------------ desenho
ModoUV texturas_modo(TipoMaterial m, int variacao) {
    Vector2 desloca = {materiais_hash(variacao, 0, TEXTURA_SEMENTE),
                       materiais_hash(variacao, 1, TEXTURA_SEMENTE)};
    switch (m) {
    case MATERIAL_CONCRETO:
        return (ModoUV){{TEXTURA_METROS_CONCRETO, PE_DIREITO}, {desloca.x, 0.0f}, false}; // v preso ao teto
    case MATERIAL_OLIVA:
        return (ModoUV){{PE_DIREITO, PE_DIREITO}, {desloca.x, 0.0f}, false};
    case MATERIAL_PISO:
        return (ModoUV){{TEXTURA_METROS_PISO, TEXTURA_METROS_PISO}, {0.0f, 0.0f}, false};
    case MATERIAL_TETO:
        return (ModoUV){{TEXTURA_METROS_TETO, TEXTURA_METROS_TETO}, {0.0f, 0.0f}, false};
    case MATERIAL_METAL:
        return (ModoUV){{TEXTURA_METROS_METAL, TEXTURA_METROS_METAL}, desloca, false};
    default:
        return (ModoUV){{1.0f, 1.0f}, {0.0f, 0.0f}, true}; // porta: as almofadas seguem a folha
    }
}

// Os 4 cantos de cada face (sinal de x, y, z a partir do centro), na ordem que a raylib usa para cubos
// (anti-horária vista de fora: a face de trás é descartada).
static const signed char FACES[6][4][3] = {
    {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}},     // z+
    {{-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}, {1, -1, -1}}, // z−
    {{-1, 1, -1}, {-1, 1, 1}, {1, 1, 1}, {1, 1, -1}},     // y+
    {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}}, // y−
    {{1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, -1, 1}},     // x+
    {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}}, // x−
};
static const int EIXO_DA_FACE[6] = {2, 2, 1, 1, 0, 0};

// UV local: cada face vai de 0 a 1, com v = 0 em cima (a imagem não fica de cabeça para baixo).
static Vector2 uv_local(Vector3 p, int eixo, Vector3 min, Vector3 tam) {
    Vector2 m = eixo == 0   ? (Vector2){tam.z, tam.y}
                : eixo == 1 ? (Vector2){tam.x, tam.z}
                            : (Vector2){tam.x, tam.y};
    m.x = m.x > 0.0f ? m.x : 1.0f;
    m.y = m.y > 0.0f ? m.y : 1.0f;
    Vector2 d = eixo == 0   ? (Vector2){-min.z / m.x, 0.0f}
                : eixo == 1 ? (Vector2){-min.x / m.x, -min.z / m.y}
                            : (Vector2){-min.x / m.x, 0.0f};
    return materiais_uv(p, eixo, m, d, min.y + tam.y);
}

void texturas_caixa(Texture2D textura, Vector3 centro, Vector3 tamanho, ModoUV uv, Color tinta) {
    if (textura.id == 0) {
        DrawCubeV(centro, tamanho, tinta);
        return;
    }
    Vector3 meio = Vector3Scale(tamanho, 0.5f), min = Vector3Subtract(centro, meio);
    rlSetTexture(textura.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tinta.r, tinta.g, tinta.b, tinta.a);
    // laço de desenho: as 6 faces e os 4 cantos de cada uma (não é map/filter/fold)
    for (int f = 0; f < 6; f++) {
        int eixo = EIXO_DA_FACE[f];
        rlNormal3f(eixo == 0 ? FACES[f][0][0] : 0.0f, eixo == 1 ? FACES[f][0][1] : 0.0f,
                   eixo == 2 ? FACES[f][0][2] : 0.0f);
        for (int k = 0; k < 4; k++) {
            Vector3 p = {centro.x + FACES[f][k][0] * meio.x, centro.y + FACES[f][k][1] * meio.y,
                         centro.z + FACES[f][k][2] * meio.z};
            Vector2 st = uv.local ? uv_local(p, eixo, min, tamanho)
                                  : materiais_uv(p, eixo, uv.metros, uv.desloca, PE_DIREITO);
            rlTexCoord2f(st.x, st.y);
            rlVertex3f(p.x, p.y, p.z);
        }
    }
    rlEnd();
    rlSetTexture(0);
}

void texturas_caixa_mat(const Estado *e, TipoMaterial m, Vector3 centro, Vector3 tamanho, ModoUV uv,
                        Color tinta) {
    posfx_material(e, m);
    texturas_caixa(e->rec.material[m], centro, tamanho, uv, tinta);
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
}

void texturas_quadro(Texture2D textura, Vector3 centro, Vector3 direita, Vector3 cima, Color tinta) {
    if (textura.id == 0) return;
    Vector3 d = Vector3Scale(direita, 0.5f), c = Vector3Scale(cima, 0.5f);
    Vector3 n = Vector3Normalize(Vector3CrossProduct(direita, cima));
    Vector3 cantos[4] = {Vector3Subtract(Vector3Subtract(centro, d), c),
                         Vector3Subtract(Vector3Add(centro, d), c), Vector3Add(Vector3Add(centro, d), c),
                         Vector3Add(Vector3Subtract(centro, d), c)};
    static const float UV[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    rlSetTexture(textura.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tinta.r, tinta.g, tinta.b, tinta.a);
    rlNormal3f(n.x, n.y, n.z);
    for (int k = 0; k < 4; k++) { // laço de desenho: os 4 cantos (não é map/filter/fold)
        rlTexCoord2f(UV[k][0], UV[k][1]);
        rlVertex3f(cantos[k].x, cantos[k].y, cantos[k].z);
    }
    rlEnd();
    rlSetTexture(0);
}
