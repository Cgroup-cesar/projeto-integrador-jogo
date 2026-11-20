// test_menu.c — Teste standalone do menu inicial VHS/CRT
#include "menu_inicial.h"
#include <raylib.h>

int main(void) {
    const int larguraInicial = 1280;
    const int alturaInicial = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(larguraInicial, alturaInicial, "PRECURSOR - Menu VHS/CRT");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Esc é do jogo, não fecha a janela

    MenuInicial menu;
    menu_inicial_init(&menu, larguraInicial, alturaInicial);

    int largAnt = larguraInicial;
    int altAnt = alturaInicial;

    while (!WindowShouldClose()) {
        // Verifica redimensionamento
        int largAtual = GetScreenWidth();
        int altAtual = GetScreenHeight();
        if (largAtual != largAnt || altAtual != altAnt) {
            menu_inicial_resize(&menu, largAtual, altAtual);
            largAnt = largAtual;
            altAnt = altAtual;
        }

        // Atualiza
        float dt = GetFrameTime();
        menu_inicial_update(&menu, dt);

        // Verifica ações
        if (menu_inicial_deve_sair(&menu)) break;
        if (menu_inicial_deve_iniciar(&menu)) {
            TraceLog(LOG_INFO, "JOGO INICIADO!");
            break;
        }

        // Desenha
        BeginDrawing();
        ClearBackground(BLACK);
        menu_inicial_draw(&menu);
        EndDrawing();
    }

    menu_inicial_unload(&menu);
    CloseWindow();
    return 0;
}
