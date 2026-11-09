// ui.c — fonte com fallback, texto transliterado, menu, HUD, legendas e tela de fim.
#include "ui.h"
#include "audio_sintese.h"
#include <stdio.h>
#include <string.h>

// ------------------------------------------------------------------ fonte
static const char *const FONTES_CANDIDATAS[] = {
    "fontes/VT323-Regular.ttf",
    "fontes/IBMPlexMono-Regular.ttf",
};

// Símbolos além do Latin-1 que o jogo usa (setas, conectivos lógicos, quadradinhos do HUD).
static const int SIMBOLOS_EXTRAS[] = {0x2190, 0x2191, 0x2192, 0x2193, 0x2194, 0x2227, 0x2228, 0x25A0, 0x25A1, 0x2014, 0x2026, 0x2713, 0x2714};

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
    {0x2026, "..."}, {0x2190, "<-"}, {0x2191, "^"}, {0x2193, "v"}, {0x2192, "->"}, {0x2194, "<->"}, {0x2227, "^"}, {0x2228, "v"}, {0x25A0, "#"},
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

float ui_texto_quebrado(const Estado *e, const char *texto, Vector2 pos, float largura, float tamanho, Color cor) {
    char limpo[1024], linha[512] = "";
    translitera(texto, limpo, sizeof(limpo), e->rec.tem_fonte ? &e->rec.fonte : NULL);
    float y = pos.y, esp = espacamento(e, tamanho);
    size_t n = 0;
    const char *p = limpo;
    while (*p != '\0') {
        const char *fim_palavra = p;
        while (*fim_palavra != '\0' && *fim_palavra != ' ') fim_palavra++;
        size_t tam_palavra = (size_t)(fim_palavra - p);
        char teste[512];
        snprintf(teste, sizeof(teste), "%.*s%s%.*s", (int)n, linha, n > 0 ? " " : "", (int)tam_palavra, p);
        if (n > 0 && MeasureTextEx(e->rec.fonte, teste, tamanho, esp).x > largura) {
            DrawTextEx(e->rec.fonte, linha, (Vector2){pos.x, y}, tamanho, esp, cor);
            y += tamanho * 1.25f;
            snprintf(linha, sizeof(linha), "%.*s", (int)tam_palavra, p);
        } else {
            snprintf(linha, sizeof(linha), "%s", teste);
        }
        n = strlen(linha);
        p = (*fim_palavra == ' ') ? fim_palavra + 1 : fim_palavra;
    }
    if (n > 0) DrawTextEx(e->rec.fonte, linha, (Vector2){pos.x, y}, tamanho, esp, cor);
    return y + tamanho * 1.25f - pos.y;
}

void ui_painel(Rectangle r, Color borda) {
    DrawRectangleRec(r, (Color){6, 10, 8, 215});
    DrawRectangleLinesEx(r, 1.5f, Fade(borda, 0.65f));
}

void ui_opcao(const Estado *e, const char *texto, Vector2 pos, float largura, float tamanho, bool ativa, Color cor) {
    float s = ui_escala();
    Rectangle r = {pos.x - 10 * s, pos.y - 4 * s, largura, tamanho + 8 * s};
    if (ativa) {
        DrawRectangleRounded(r, 0.25f, 6, Fade(cor, 0.16f));
        DrawRectangle((int)r.x, (int)r.y, (int)(3 * s), (int)r.height, cor);
    }
    ui_texto(e, texto, (Vector2){pos.x + 6 * s, pos.y}, tamanho, ativa ? cor : Fade(cor, 0.55f));
}

float ui_quadrados_vidro(int vidas, Vector2 pos, float lado, Color cor) {
    for (int i = 0; i < VIDAS_VIDRO; i++) {
        Rectangle r = {pos.x + i * lado * 1.6f, pos.y, lado, lado};
        if (i < vidas) DrawRectangleRec(r, cor);
        else DrawRectangleLinesEx(r, lado * 0.12f, cor);
    }
    return VIDAS_VIDRO * lado * 1.6f;
}

// ------------------------------------------------------------------ legendas
void ui_fala(Estado *e, BlocoFalas bloco) {
    int n = 0;
    const Legenda *falas = dialogo_falas(bloco, &n);
    bool estava_vazia = e->legendas.n == 0;
    for (int i = 0; i < n; i++) {
        if (e->legendas.n >= MAX_LEGENDAS) {
            TraceLog(LOG_WARNING, "PRECURSOR: fila de legendas cheia; fala descartada");
            break;
        }
        e->legendas.fila[e->legendas.n++] = falas[i];
    }
    if (estava_vazia && e->legendas.n > 0) {
        e->legendas.tempo = 0.0f;
        audio_toca(e, SOM_BIP_INTERFONE, 0.6f, 1.0f);
    }
}

bool ui_legendas_vazias(const Estado *e) {
    return e->legendas.n == 0;
}

void ui_legenda_pula(Estado *e) {
    FilaLegendas *f = &e->legendas;
    if (f->n == 0) return;
    for (int i = 1; i < f->n; i++) f->fila[i - 1] = f->fila[i];
    f->n--;
    f->tempo = 0.0f;
    if (f->n > 0) audio_toca(e, SOM_BIP_INTERFONE, 0.6f, 1.0f);
}

static void atualiza_legendas(Estado *e, float dt) {
    FilaLegendas *f = &e->legendas;
    if (f->n == 0) return;
    f->tempo += dt;
    if (f->tempo >= f->fila[0].duracao) ui_legenda_pula(e);
}

// ------------------------------------------------------------------ menu e fim (lógica)
static void atualiza_menu(Estado *e) {
    int opcoes = e->modo_feira ? 1 : 2;
    if (e->in.cima) e->menu.cursor = (e->menu.cursor + opcoes - 1) % opcoes;
    if (e->in.baixo) e->menu.cursor = (e->menu.cursor + 1) % opcoes;
    if (!e->in.confirmar) return;
    if (e->menu.cursor == 0) e->evento = EV_INICIAR;
    else e->sair = true;
}

static void atualiza_tela_final(Estado *e, float dt) {
    if (!e->fim.tela_final) return;
    e->fim.tempo_tela += dt;
    if (e->in.confirmar || e->fim.tempo_tela >= FIM_VOLTA_MENU_S) e->evento = EV_VOLTAR_MENU;
}

void ui_atualiza(Estado *e, float dt) {
    atualiza_legendas(e, dt);
    if (e->fase == FASE_MENU) atualiza_menu(e);
    if (e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM) atualiza_tela_final(e, dt);
}

// ------------------------------------------------------------------ desenho: menu, intro, pausa
static void desenha_opcao(const Estado *e, const char *texto, float y, bool ativa) {
    float s = ui_escala();
    Vector2 m = ui_mede(e, texto, 30 * s);
    float x = (GetScreenWidth() - m.x) * 0.5f;
    if (ativa) {
        // Destaque do item ativo: fundo mais claro, cantos arredondados e barra na lateral.
        Rectangle r = {x - 22 * s, y - 6 * s, m.x + 44 * s, m.y + 12 * s};
        DrawRectangleRounded(r, 0.25f, 6, Fade(COR_FOSFORO, 0.12f));
        DrawRectangle((int)r.x, (int)r.y, (int)(3 * s), (int)r.height, COR_FOSFORO);
    }
    ui_texto(e, texto, (Vector2){x, y}, 30 * s, ativa ? COR_FOSFORO : Fade(COR_FOSFORO, 0.5f));
}

static void desenha_menu(const Estado *e) {
    float s = ui_escala();
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangleGradientV(0, 0, w, h, Fade(BLACK, 0.35f), Fade(BLACK, 0.85f));
    ui_texto_centro(e, TITULO_JOGO, 190 * s, 76 * s, COR_FOSFORO);
#if TITULO_PROVISORIO
    ui_texto_centro(e, dialogo_texto(TXT_MENU_AVISO_PROVISORIO), 278 * s, 22 * s, COR_AMBAR);
#endif
    desenha_opcao(e, dialogo_texto(TXT_MENU_INICIAR), 380 * s, e->menu.cursor == 0);
    if (!e->modo_feira) desenha_opcao(e, dialogo_texto(TXT_MENU_SAIR), 430 * s, e->menu.cursor == 1);
    ui_texto_centro(e, dialogo_texto(TXT_MENU_DICA), 560 * s, 18 * s, Fade(COR_VIDRO, 0.8f));
    ui_texto_centro(e, dialogo_texto(TXT_MENU_CONTROLES), 650 * s, 18 * s, Fade(COR_FOSFORO, 0.55f));
}

static void copia_letras(const char *texto, int letras, char *saida, size_t tamanho) {
    size_t j = 0;
    int n = 0;
    for (const char *p = texto; *p && j + 1 < tamanho; p++) {
        if (((unsigned char)*p & 0xC0) != 0x80 && n++ >= letras) break;
        saida[j++] = *p;
    }
    saida[j] = '\0';
}

static void desenha_intro(const Estado *e) {
    float s = ui_escala();
    if (e->intro.interfone) return; // a sala aparece; as falas vêm pelas legendas
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
    float y = 230 * s;
    for (int i = 0; i <= e->intro.linha && i < INTRO_LINHAS; i++) {
        char parcial[256];
        const char *linha = dialogo_intro(i);
        if (i == e->intro.linha) copia_letras(linha, (int)e->intro.letras, parcial, sizeof(parcial));
        else snprintf(parcial, sizeof(parcial), "%s", linha);
        y += ui_texto_quebrado(e, parcial, (Vector2){150 * s, y}, GetScreenWidth() - 300 * s, 24 * s, COR_FOSFORO) + 14 * s;
    }
    ui_texto(e, dialogo_texto(TXT_INTRO_PULAR), (Vector2){GetScreenWidth() - 150 * s, GetScreenHeight() - 40 * s}, 16 * s, Fade(COR_FOSFORO, 0.4f));
}

static void desenha_pausa(const Estado *e) {
    float s = ui_escala();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));
    ui_texto_centro(e, dialogo_texto(TXT_PAUSA), 240 * s, 52 * s, COR_FOSFORO);
    desenha_opcao(e, dialogo_texto(TXT_PAUSA_CONTINUAR), 350 * s, e->menu.cursor == 0);
    desenha_opcao(e, dialogo_texto(TXT_PAUSA_MENU), 400 * s, e->menu.cursor == 1);
}

// ------------------------------------------------------------------ desenho: HUD
// Integridade do vidro (vidas): quadrados cheios e vazios, desenhados (não dependem da fonte).
static void desenha_vidas(const Estado *e, Vector2 pos) {
    float s = ui_escala();
    Color cor = e->vidas <= 1 ? COR_ALARME : COR_VIDRO;
    ui_texto(e, dialogo_texto(TXT_HUD_VIDRO), pos, 20 * s, cor);
    float x = pos.x + ui_mede(e, dialogo_texto(TXT_HUD_VIDRO), 20 * s).x + 12 * s;
    ui_quadrados_vidro(e->vidas, (Vector2){x, pos.y + 2 * s}, 14 * s, cor);
}

static void desenha_vidro_e_placar(const Estado *e) {
    float s = ui_escala();
    char texto[64];
    float x = GetScreenWidth() - 300 * s;
    if (!regras_fase_eh_saida(e->fase)) desenha_vidas(e, (Vector2){x, 20 * s});
    snprintf(texto, sizeof(texto), "%s %03d", dialogo_texto(TXT_HUD_PLACAR), e->placar);
    ui_texto(e, texto, (Vector2){x, 46 * s}, 20 * s, COR_FOSFORO);
}

static void desenha_folego(const Estado *e) {
    if (!regras_fase_eh_saida(e->fase) || e->jog.folego >= JOGADOR_FOLEGO_MAX - 0.01f) return;
    float s = ui_escala();
    float fracao = e->jog.folego / JOGADOR_FOLEGO_MAX;
    Rectangle r = {30 * s, GetScreenHeight() - 50 * s, 200 * s, 8 * s};
    ui_texto(e, dialogo_texto(TXT_HUD_FOLEGO), (Vector2){r.x, r.y - 24 * s}, 16 * s, Fade(COR_FOSFORO, 0.7f));
    DrawRectangleRec(r, Fade(BLACK, 0.6f));
    DrawRectangle((int)r.x, (int)r.y, (int)(r.width * fracao), (int)r.height, e->jog.exausto ? COR_ALARME : COR_FOSFORO);
}

static TextoId objetivo(const Estado *e) {
    if (regras_fase_eh_saida(e->fase)) {
        if (e->painel.resolvido) return TXT_OBJETIVO_VOLTAR;
        return e->fase == FASE_S1 ? TXT_OBJETIVO_S1 : TXT_OBJETIVO_S2;
    }
    return e->fase == FASE_RESPOSTA_FINAL ? TXT_OBJETIVO_RESPOSTA : TXT_OBJETIVO_SALA;
}

static void desenha_mira(const Estado *e) {
    float s = ui_escala();
    int cx = GetScreenWidth() / 2, cy = GetScreenHeight() / 2;
    if (e->jog.modo == MODO_ANDANDO) DrawCircle(cx, cy, 2.5f * s, Fade(COR_FOSFORO, 0.6f));
    TextoId dica = TXT_TOTAL;
    if (e->jog.modo == MODO_ARMARIO) dica = TXT_ARMARIO_SAIR;
    else if (e->jog.mira >= 0) {
        static const TextoId por_tipo[] = {TXT_MIRA_CONSOLE, TXT_MIRA_PAINEL, TXT_MIRA_PAINEL, TXT_MIRA_DIARIO, TXT_MIRA_ARMARIO, TXT_MIRA_PORTA};
        dica = por_tipo[e->interativos.itens[e->jog.mira].tipo];
    }
    if (dica != TXT_TOTAL) ui_texto_centro(e, dialogo_texto(dica), cy + 40 * s, 22 * s, COR_FOSFORO);
}

static void desenha_hud(const Estado *e) {
    float s = ui_escala();
    if (e->jog.modo == MODO_ANDANDO || e->jog.modo == MODO_ARMARIO)
        ui_texto(e, dialogo_texto(objetivo(e)), (Vector2){24 * s, 20 * s}, 18 * s, Fade(COR_FOSFORO, 0.7f));
    if (e->jog.modo != MODO_TERMINAL && e->jog.modo != MODO_PAINEL) desenha_vidro_e_placar(e); // o terminal já mostra
    desenha_folego(e);
    desenha_mira(e);
}

static void desenha_legenda(const Estado *e) {
    if (e->legendas.n == 0) return;
    float s = ui_escala();
    const Legenda *l = &e->legendas.fila[0];
    float largura = 900 * s;
    float x = (GetScreenWidth() - largura) * 0.5f, y = GetScreenHeight() - 150 * s;
    if (e->jog.modo == MODO_TERMINAL) { // o terminal ocupa a direita: a legenda vai para a esquerda
        largura = GetScreenWidth() - 680 * s;
        x = 24 * s;
    }
    const char *quem = l->falante == FALANTE_VARELA ? "CEL. VARELA (interfone)" : l->falante == FALANTE_ELIAS ? "PROF. ELIAS" : "";
    ui_painel((Rectangle){x - 16 * s, y - 34 * s, largura + 32 * s, 104 * s}, COR_AMBAR);
    ui_texto(e, quem, (Vector2){x, y - 26 * s}, 16 * s, COR_AMBAR);
    ui_texto_quebrado(e, l->texto, (Vector2){x, y}, largura, 22 * s, RAYWHITE);
}

static void desenha_diario(const Estado *e) {
    if (e->jog.modo != MODO_DIARIO) return;
    float s = ui_escala();
    Rectangle folha = {GetScreenWidth() * 0.5f - 330 * s, 60 * s, 660 * s, 440 * s}; // acima da legenda
    DrawRectangleRec(folha, (Color){0xd8, 0xd0, 0xb8, 245});
    DrawRectangleLinesEx(folha, 2, (Color){0x6a, 0x5a, 0x40, 255});
    ui_texto_quebrado(e, dialogo_texto(TXT_DIARIO), (Vector2){folha.x + 40 * s, folha.y + 50 * s}, folha.width - 80 * s, 22 * s, (Color){0x20, 0x1a, 0x14, 255});
    ui_texto(e, dialogo_texto(TXT_DIARIO_FECHAR), (Vector2){folha.x + 40 * s, folha.y + folha.height - 40 * s}, 16 * s, (Color){0x50, 0x44, 0x34, 255});
}

// ------------------------------------------------------------------ desenho: tela de fim
static void desenha_tela_final(const Estado *e) {
    float s = ui_escala();
    bool vitoria = e->fase == FASE_FIM_BOM;
    char texto[96];
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){4, 6, 5, 240});
    ui_texto_centro(e, dialogo_texto(vitoria ? TXT_FIM_VITORIA : TXT_FIM_DERROTA), 90 * s, 64 * s, vitoria ? COR_FOSFORO : COR_ALARME);
    snprintf(texto, sizeof(texto), "%s  %d", dialogo_texto(TXT_FIM_PLACAR), e->placar);
    ui_texto_centro(e, texto, 190 * s, 30 * s, COR_FOSFORO);
    float y = 250 * s;
    const Fase fases[] = {FASE_R1, FASE_S1, FASE_R2, FASE_S2, FASE_R3, FASE_RESPOSTA_FINAL, vitoria ? FASE_FIM_BOM : FASE_FIM_RUIM};
    float meio = GetScreenWidth() * 0.5f;
    for (int i = 0; i < 7; i++) { // duas colunas: o nome alinhado à direita, os pontos à esquerda
        int pontos = regras_placar_da_fase(e->eventos.itens, e->eventos.n, fases[i]);
        const char *nome = dialogo_nome_fase(fases[i]);
        ui_texto(e, nome, (Vector2){meio - 24 * s - ui_mede(e, nome, 20 * s).x, y}, 20 * s, Fade(COR_FOSFORO, 0.75f));
        snprintf(texto, sizeof(texto), "%+d", pontos);
        ui_texto(e, texto, (Vector2){meio + 24 * s, y}, 20 * s, pontos < 0 ? COR_ALARME : COR_FOSFORO);
        y += 26 * s;
    }
    int seg = (int)e->tempo_partida;
    snprintf(texto, sizeof(texto), "%s %02d:%02d   ·   %s %d", dialogo_texto(TXT_FIM_TEMPO), seg / 60, seg % 60, dialogo_texto(TXT_FIM_ERROS), e->erros_totais);
    ui_texto_centro(e, texto, y + 16 * s, 22 * s, COR_VIDRO);
    snprintf(texto, sizeof(texto), "%s  (%d)", dialogo_texto(TXT_FIM_ENTER), (int)(FIM_VOLTA_MENU_S - e->fim.tempo_tela + 0.99f));
    ui_texto_centro(e, texto, GetScreenHeight() - 110 * s, 24 * s, COR_AMBAR);
    ui_texto_centro(e, dialogo_texto(TXT_FIM_RODAPE), GetScreenHeight() - 50 * s, 16 * s, Fade(COR_FOSFORO, 0.45f));
}

static void desenha_debug(const Estado *e) {
    if (!e->debug_criatura) return;
    char texto[160];
    snprintf(texto, sizeof(texto), "fase %s  t=%.1f  criatura %s%s  pos %.1f %.1f  ruidos %d  %d fps", regras_nome_fase(e->fase),
             e->tempo_fase, regras_nome_estado_criatura(e->cri.estado), e->cri.ativo ? "" : " (inativa)", e->jog.pos.x,
             e->jog.pos.z, e->ruidos.n, GetFPS());
    ui_texto(e, texto, (Vector2){10, GetScreenHeight() - 24.0f}, 16, YELLOW);
}

void ui_desenha(const Estado *e) {
    bool jogando = e->fase != FASE_MENU && e->fase != FASE_INTRO && e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM;
    if (e->fase == FASE_MENU) desenha_menu(e);
    if (e->fase == FASE_INTRO) desenha_intro(e);
    if (jogando && e->jumpscare_t <= 0.0f) desenha_hud(e);
    desenha_diario(e);
    desenha_legenda(e);
    if ((e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM) && e->fim.tela_final) desenha_tela_final(e);
    if (e->pausado) desenha_pausa(e);
    desenha_debug(e);
}
