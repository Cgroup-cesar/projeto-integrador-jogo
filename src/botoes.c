// botoes.c — os ícones dos botões do controle. Carrega uma vez; no desenho, só procura o rótulo na tabela.
// Ícones: Zacksly, "Xbox Series Button Icons & Controls" (CC BY), recortados para fundo transparente.
#include "botoes.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

// O rótulo que a barra usa (teclas.c) e o arquivo em assets/botoes/. A ordem é a de rec->botao[].
typedef struct {
    const char *rotulo, *arquivo;
    float escala; // os analógicos têm as setinhas em volta: crescem para o círculo ficar do tamanho dos outros
} IconeBotao;

static const IconeBotao ICONES[BOTOES_TOTAL] = {
    {"A", "a", 1.0f},         {"B", "b", 1.0f},           {"X", "x", 1.0f},
    {"Y", "y", 1.0f},         {"LB", "lb", 1.0f},         {"L", "l", 1.4f},
    {"R", "r", 1.4f},         {"VIEW", "view", 1.0f},     {"START", "menu", 1.0f},
    {"SETAS", "dpad", 1.0f},  {"SETAS_V", "dpad_v", 1.0f}, {"SETAS_H", "dpad_h", 1.0f},
};

void botoes_carrega(Recursos *rec) {
    for (int i = 0; i < BOTOES_TOTAL; i++) { // carregamento: uma vez por ícone (não é map/filter/fold)
        char relativo[64], caminho[512];
        snprintf(relativo, sizeof(relativo), "botoes/%s.png", ICONES[i].arquivo);
        rec->tem_botao[i] = ui_caminho_asset(relativo, caminho, sizeof(caminho));
        if (!rec->tem_botao[i]) {
            TraceLog(LOG_INFO, "BOTOES: sem %s, o botão %s fica em texto", relativo, ICONES[i].rotulo);
            continue;
        }
        rec->botao[i] = LoadTexture(caminho);
        rec->tem_botao[i] = rec->botao[i].id != 0;
        if (!rec->tem_botao[i]) continue;
        GenTextureMipmaps(&rec->botao[i]); // a barra desenha bem menor que o arquivo
        SetTextureFilter(rec->botao[i], TEXTURE_FILTER_TRILINEAR);
    }
}

void botoes_descarrega(Recursos *rec) {
    for (int i = 0; i < BOTOES_TOTAL; i++) // padrão MAP: libera cada ícone carregado
        if (rec->tem_botao[i]) UnloadTexture(rec->botao[i]);
}

float botoes_desenha(const Estado *e, const char *rotulo, Vector2 pos, float altura, Color cor) {
    for (int i = 0; i < BOTOES_TOTAL; i++) { // padrão FOLD: o ícone deste rótulo, se houver
        if (strcmp(ICONES[i].rotulo, rotulo) != 0) continue;
        if (!e->rec.tem_botao[i]) return 0;
        Texture2D t = e->rec.botao[i];
        float h = altura * ICONES[i].escala, largura = h * (float)t.width / (float)t.height;
        DrawTexturePro(t, (Rectangle){0, 0, (float)t.width, (float)t.height},
                       (Rectangle){pos.x, pos.y - (h - altura) * 0.5f, largura, h}, (Vector2){0, 0}, 0, cor);
        return largura;
    }
    return 0;
}
