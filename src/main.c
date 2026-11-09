// main.c — game loop imperativo: entrada → atualiza → desenha.
// Aqui ficam só a orquestração (o "como"): quem decide (o "o quê") são as regras puras.
#include "estado.h"
#include "posfx.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

static Estado estado; // o estado inteiro do jogo (estático: nada de malloc por quadro)
static const char *arquivo_captura = NULL; // --captura <arquivo.png>: salva um quadro e segue

// ------------------------------------------------------------------ entrada
static void le_entrada(Entrada *in) {
    memset(in, 0, sizeof(*in));
    in->mover.x = (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
    in->mover.y = (float)(IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
    in->mouse = GetMouseDelta();
    in->correr = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    in->interagir = IsKeyPressed(KEY_E);
    in->voltar = IsKeyPressed(KEY_ESCAPE);
    in->confirmar = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    in->cima = IsKeyPressed(KEY_UP);
    in->baixo = IsKeyPressed(KEY_DOWN);
    in->esquerda = IsKeyPressed(KEY_LEFT);
    in->direita = IsKeyPressed(KEY_RIGHT);
    in->tecla1 = IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1);
    in->tecla2 = IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2);
    in->tecla3 = IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3);
#if TECLAS_DEBUG
    in->f1 = IsKeyPressed(KEY_F1);
    in->f2 = IsKeyPressed(KEY_F2);
    in->f3 = IsKeyPressed(KEY_F3);
    in->f5 = IsKeyPressed(KEY_F5);
    in->f9 = IsKeyPressed(KEY_F9);
    in->f12 = IsKeyPressed(KEY_F12);
#endif
    in->alguma = GetKeyPressed() != 0 || in->mover.x != 0 || in->mover.y != 0 || in->mouse.x != 0 ||
                 in->mouse.y != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ------------------------------------------------------------------ atualiza e desenha
static void atualiza(Estado *e, float dt) {
    e->tempo_total += dt;
    posfx_atualiza(e, dt);
    ui_atualiza(e, dt);
    if (e->in.voltar) e->sair = true;
}

static void desenha(const Estado *e) {
    posfx_inicia_cena(e);
    ClearBackground(COR_SOMBRA);
    posfx_termina_cena(e);

    BeginDrawing();
    ClearBackground(BLACK);
    posfx_desenha(e);
    ui_desenha(e);
    EndDrawing();
}

// Lê o quadro que acabou de ser desenhado e salva em PNG.
static void salva_captura(const char *caminho) {
    Image quadro = LoadImageFromScreen();
    if (ExportImage(quadro, caminho)) TraceLog(LOG_INFO, "PRECURSOR: captura salva em %s", caminho);
    UnloadImage(quadro);
}

// ------------------------------------------------------------------ início e fim
static void le_argumentos(Estado *e, int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--smoke") == 0) e->modo_smoke = true;
        if (strcmp(argv[i], "--feira") == 0) e->modo_feira = true;
        if (strcmp(argv[i], "--captura") == 0 && i + 1 < argc) arquivo_captura = argv[++i];
    }
}

static void abre_janela(void) {
    char titulo[128];
    snprintf(titulo, sizeof(titulo), "%s%s", TITULO_JOGO, TITULO_PROVISORIO ? " [NOME PROVISÓRIO]" : "");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_JANELA, ALTURA_JANELA, titulo);
    SetWindowMinSize(640, 360);
    SetExitKey(KEY_NULL); // Esc é do jogo (sair do terminal, pausar), não fecha a janela
}

int main(int argc, char **argv) {
    le_argumentos(&estado, argc, argv);
    abre_janela();
    // [REQ:RF1] game loop a 60 FPS; o dt de GetFrameTime() é limitado a DT_MAXIMO
    SetTargetFPS(FPS_ALVO);
    estado.vhs_ligado = VHS_LIGADO_AO_INICIAR;
    posfx_inicia(&estado.rec);
    ui_carrega_fonte(&estado.rec);

    while (!WindowShouldClose() && !estado.sair) {
        float dt = GetFrameTime();
        if (dt > DT_MAXIMO) dt = DT_MAXIMO;
        le_entrada(&estado.in);
        atualiza(&estado, dt);
        desenha(&estado);
        if (arquivo_captura != NULL && estado.tempo_total > 0.8f) {
            salva_captura(arquivo_captura);
            arquivo_captura = NULL;
        }
        if (estado.modo_smoke && estado.tempo_total > 1.0f) estado.sair = true;
    }

    ui_descarrega_fonte(&estado.rec);
    posfx_finaliza(&estado.rec);
    CloseWindow();
    return 0;
}
