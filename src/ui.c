// ui.c — fonte com fallback, texto transliterado, menu, HUD, legendas e tela de fim.
#include "ui.h"
#include <stdio.h>
#include <string.h>

// ------------------------------------------------------------------ fonte
static const char *const FONTES_CANDIDATAS[] = {
    "fontes/VT323-Regular.ttf",
    "fontes/IBMPlexMono-Regular.ttf",
};

// Símbolos além do Latin-1 que o jogo usa (setas, conectivos lógicos, quadradinhos do HUD).
static const int SIMBOLOS_EXTRAS[] = {0x2192, 0x2194, 0x2227, 0x2228, 0x25A0, 0x25A1, 0x2014, 0x2026, 0x2713, 0x2714};

bool ui_caminho_asset(const char *relativo, char *saida, size_t tamanho) {
    snprintf(saida, tamanho, "assets/%s", relativo);
    if (FileExists(saida)) return true;
    snprintf(saida, tamanho, "%s../assets/%s", GetApplicationDirectory(), relativo);
    if (FileExists(saida)) return true;
    snprintf(saida, tamanho, "%sassets/%s", GetApplicationDirectory(), relativo);
    return FileExists(saida);
}

void ui_carrega_fonte(Recursos *rec) {
    int codigos[224 + sizeof(SIMBOLOS_EXTRAS) / sizeof(SIMBOLOS_EXTRAS[0])];
    int n = 0;
    for (int c = 32; c < 256; c++) codigos[n++] = c; // Latin-1 inteiro
    for (size_t i = 0; i < sizeof(SIMBOLOS_EXTRAS) / sizeof(SIMBOLOS_EXTRAS[0]); i++) codigos[n++] = SIMBOLOS_EXTRAS[i];

    rec->tem_fonte = false;
    rec->fonte = GetFontDefault();
    char caminho[512];
    for (size_t i = 0; i < sizeof(FONTES_CANDIDATAS) / sizeof(FONTES_CANDIDATAS[0]); i++) {
        if (!ui_caminho_asset(FONTES_CANDIDATAS[i], caminho, sizeof(caminho))) continue;
        Font f = LoadFontEx(caminho, 48, codigos, n);
        if (IsFontValid(f) && f.texture.id != 0) {
            SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
            rec->fonte = f;
            rec->tem_fonte = true;
            TraceLog(LOG_INFO, "PRECURSOR: fonte carregada: %s", caminho);
            return;
        }
    }
    TraceLog(LOG_INFO, "PRECURSOR: sem fonte em assets/fontes/; usando a padrão com transliteração (tools/baixa_fonte)");
}

void ui_descarrega_fonte(Recursos *rec) {
    if (rec->tem_fonte) UnloadFont(rec->fonte);
}

// ------------------------------------------------------------------ transliteração
typedef struct {
    int codigo;
    const char *ascii;
} Troca;

static const Troca TROCAS[] = {
    {0xE1, "a"}, {0xE0, "a"}, {0xE2, "a"}, {0xE3, "a"}, {0xE4, "a"}, {0xC1, "A"}, {0xC0, "A"}, {0xC2, "A"},
    {0xC3, "A"}, {0xC4, "A"}, {0xE9, "e"}, {0xE8, "e"}, {0xEA, "e"}, {0xEB, "e"}, {0xC9, "E"}, {0xC8, "E"},
    {0xCA, "E"}, {0xCB, "E"}, {0xED, "i"}, {0xEC, "i"}, {0xEE, "i"}, {0xEF, "i"}, {0xCD, "I"}, {0xCC, "I"},
    {0xCE, "I"}, {0xF3, "o"}, {0xF2, "o"}, {0xF4, "o"}, {0xF5, "o"}, {0xF6, "o"}, {0xD3, "O"}, {0xD2, "O"},
    {0xD4, "O"}, {0xD5, "O"}, {0xD6, "O"}, {0xFA, "u"}, {0xF9, "u"}, {0xFB, "u"}, {0xFC, "u"}, {0xDA, "U"},
    {0xD9, "U"}, {0xDB, "U"}, {0xDC, "U"}, {0xE7, "c"}, {0xC7, "C"}, {0xF1, "n"}, {0xD1, "N"}, {0xBA, "o"},
    {0xAA, "a"}, {0xB7, "|"}, {0xAC, "~"}, {0xAB, "\""}, {0xBB, "\""}, {0xB0, "o"}, {0x2014, "-"}, {0x2013, "-"},
    {0x2026, "..."}, {0x2192, "->"}, {0x2194, "<->"}, {0x2227, "^"}, {0x2228, "v"}, {0x25A0, "#"},
    {0x25A1, "o"}, {0x201C, "\""}, {0x201D, "\""}, {0x2018, "'"}, {0x2019, "'"}, {0x2713, "*"}, {0x2714, "*"},
};

static const char *ascii_de(int codigo) {
    for (size_t i = 0; i < sizeof(TROCAS) / sizeof(TROCAS[0]); i++)
        if (TROCAS[i].codigo == codigo) return TROCAS[i].ascii;
    return "?";
}

static bool fonte_desenha(const Font *fonte, int codigo) {
    if (fonte == NULL) return false;
    int indice = GetGlyphIndex(*fonte, codigo);
    return fonte->glyphs[indice].value == codigo;
}

void translitera(const char *entrada, char *saida, size_t tamanho, const Font *fonte) {
    size_t j = 0;
    const char *p = entrada;
    while (*p != '\0' && j + 1 < tamanho) {
        int bytes = 1;
        int codigo = GetCodepointNext(p, &bytes);
        if (codigo < 128 || fonte_desenha(fonte, codigo)) {
            for (int b = 0; b < bytes && j + 1 < tamanho; b++) saida[j++] = p[b];
        } else {
            const char *troca = ascii_de(codigo);
            for (size_t k = 0; troca[k] != '\0' && j + 1 < tamanho; k++) saida[j++] = troca[k];
        }
        p += bytes;
    }
    saida[j] = '\0';
}

// ------------------------------------------------------------------ texto
float ui_escala(void) {
    return GetScreenHeight() / 720.0f;
}

static const char *prepara(const Estado *e, const char *texto, char *buffer, size_t tamanho) {
    translitera(texto, buffer, tamanho, e->rec.tem_fonte ? &e->rec.fonte : NULL);
    return buffer;
}

static float espacamento(const Estado *e, float tamanho) {
    return e->rec.tem_fonte ? tamanho * 0.02f : tamanho * 0.1f;
}

void ui_texto(const Estado *e, const char *texto, Vector2 pos, float tamanho, Color cor) {
    char buffer[1024];
    DrawTextEx(e->rec.fonte, prepara(e, texto, buffer, sizeof(buffer)), pos, tamanho, espacamento(e, tamanho), cor);
}

Vector2 ui_mede(const Estado *e, const char *texto, float tamanho) {
    char buffer[1024];
    return MeasureTextEx(e->rec.fonte, prepara(e, texto, buffer, sizeof(buffer)), tamanho, espacamento(e, tamanho));
}

void ui_texto_centro(const Estado *e, const char *texto, float y, float tamanho, Color cor) {
    Vector2 m = ui_mede(e, texto, tamanho);
    ui_texto(e, texto, (Vector2){(GetScreenWidth() - m.x) * 0.5f, y}, tamanho, cor);
}

// ------------------------------------------------------------------ telas
void ui_atualiza(Estado *e, float dt) {
    (void)e;
    (void)dt;
}

void ui_desenha(const Estado *e) {
    float s = ui_escala();
    ui_texto_centro(e, TITULO_JOGO, 250 * s, 72 * s, COR_FOSFORO);
#if TITULO_PROVISORIO
    ui_texto_centro(e, "[NOME PROVISÓRIO — título final a definir]", 335 * s, 22 * s, COR_AMBAR);
#endif
    ui_texto_centro(e, "M0 — fundação: F1 liga/desliga o VHS · Esc sai", 600 * s, 18 * s, COR_VIDRO);
}
