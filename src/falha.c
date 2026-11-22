// falha.c — o chuvisco de TV e a falha das opções escolhidas (tela de início, pausa e perguntas da intro).
// A escolha muda e->menu.troca_t; daqui só se lê o estado e o relógio.
#include "falha.h"
#include "ui.h"
#include <math.h>

float falha_sorteio(int n) {
    unsigned h = (unsigned)n * 2654435761u;
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return (h & 0xFFFFu) / 65535.0f;
}

// Uma textura de ruído branco gerada uma vez só (sem malloc por quadro); a GPU a libera no CloseWindow.
static Texture2D textura_ruido(void) {
    static Texture2D t = {0};
    if (t.id == 0) {
        Image img = GenImageWhiteNoise(256, 256, 0.5f);
        t = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(t, TEXTURE_FILTER_POINT);
        SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    }
    return t;
}

// A cada quadro do chuvisco a textura é desenhada com um deslocamento sorteado: o grão "ferve" como TV fora
// do ar. Nos surtos, ele engrossa e uma linha fina corta a tela, como no tracking perdido.
void falha_chuvisco(const Estado *e, float alfa, float pico, bool faixas) {
    if (alfa <= 0.0f) return;
    float s = ui_escala(), w = GetScreenWidth(), h = GetScreenHeight(), grao = MENU_ESTATICA_GRAO * s;
    int q = (int)(e->tempo_total * MENU_ESTATICA_QUADROS);
    float surto = falha_sorteio(q / 6) > 0.8f ? pico * falha_sorteio(q) : 0.0f;
    float forca = alfa * (0.85f + 0.15f * falha_sorteio(q + 3)) + surto;
    Rectangle fonte = {falha_sorteio(q) * 256.0f, falha_sorteio(q + 1) * 256.0f, w / grao, h / grao};
    BeginBlendMode(BLEND_ADDITIVE); // aditivo: o grão só clareia, então o fundo nunca fica tapado de preto
    DrawTexturePro(textura_ruido(), fonte, (Rectangle){0, 0, w, h}, (Vector2){0, 0}, 0, Fade(WHITE, forca));
    for (int i = 0; faixas && i < 2; i++) { // padrão MAP: duas faixas claras descendo, cada uma num ritmo
        float y = fmodf(e->tempo_total * (40.0f + 55.0f * i) * s + i * h * 0.5f, h + 120 * s) - 60 * s;
        float alto = (30 + 30 * i) * s;
        DrawRectangleGradientV(0, (int)y, (int)w, (int)(alto * 0.5f), Fade(WHITE, 0.0f), Fade(WHITE, 0.05f));
        DrawRectangleGradientV(0, (int)(y + alto * 0.5f), (int)w, (int)(alto * 0.5f), Fade(WHITE, 0.05f),
                               Fade(WHITE, 0.0f));
    }
    if (surto > 0.0f && faixas)
        DrawRectangle(0, (int)(falha_sorteio(q + 9) * h), (int)w, (int)(2 * s), Fade(WHITE, 0.25f));
    EndBlendMode();
}

// Um balanço lento sempre, uma falha forte logo depois da troca (MENU_FALHA_TROCA_S) e, de vez em quando,
// um soluço curto como fita mastigada. Tudo multiplicado pela intensidade.
float falha_deslocamento(const Estado *e, float intensidade, float *forca) {
    float s = ui_escala(), desde = e->tempo_total - e->menu.troca_t;
    int q = (int)(e->tempo_total * 30.0f);
    float troca = (desde >= 0.0f && desde < MENU_FALHA_TROCA_S) ? 1.0f - desde / MENU_FALHA_TROCA_S : 0;
    float soluco = fmodf(e->tempo_total / 2.7f, 1.0f) < 0.04f ? 0.6f : 0.0f;
    *forca = fmaxf(troca, soluco) * intensidade;
    float balanco = sinf(e->tempo_total * 2.4f) * MENU_BALANCO_PX * s * intensidade;
    return balanco + *forca * (falha_sorteio(q) - 0.5f) * 22.0f * s;
}

void falha_texto(const Estado *e, const char *texto, Vector2 p, float tamanho, Color cor, float forca,
                 Rectangle faixa) {
    float s = ui_escala();
    if (forca > 0.0f) {
        int q = (int)(e->tempo_total * 30.0f);
        Color vermelho = Fade((Color){255, 30, 50, 255}, forca);
        Color ciano = Fade((Color){40, 220, 255, 255}, forca);
        BeginBlendMode(BLEND_ADDITIVE);
        ui_texto(e, texto, (Vector2){p.x + 6 * forca * s, p.y}, tamanho, vermelho);
        ui_texto(e, texto, (Vector2){p.x - 5 * forca * s, p.y}, tamanho, ciano);
        EndBlendMode();
        float corte = faixa.y + faixa.height * (0.2f + 0.5f * falha_sorteio(q + 5)); // uma fatia escorrega
        BeginScissorMode((int)faixa.x, (int)corte, (int)faixa.width, (int)(8 * s));
        float escorrega = (falha_sorteio(q + 11) - 0.5f) * 30 * forca * s;
        ui_texto(e, texto, (Vector2){p.x + escorrega, p.y}, tamanho, cor);
        EndScissorMode();
    }
    ui_texto(e, texto, p, tamanho, cor);
}
