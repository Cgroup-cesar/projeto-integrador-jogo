// console_modelo.c — o terminal de época (um Ericsson militar) em cima da mesa do console. O modelo olha
// para +X; aqui ele gira para olhar para +Z (para quem está no console, de costas para a porta e de frente
// para o vidro) e é escalado pela largura, com a base apoiada no tampo da mesa.
#include "console_modelo.h"
#include "ui.h"
#include <math.h>

void console_modelo_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_terminal = false;
    if (!ui_caminho_asset("modelos/terminal.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/terminal.glb; o console fica com a tela simples");
        return;
    }
    rec->modelo_terminal = LoadModel(caminho);
    rec->tem_terminal = IsModelValid(rec->modelo_terminal) && rec->modelo_terminal.meshCount > 0;
    if (!rec->tem_terminal) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; o console fica com a tela simples",
                 caminho);
        return;
    }
    for (int i = 0; i < rec->modelo_terminal.materialCount && rec->luz_ok; i++) // padrão MAP: luz do jogo
        rec->modelo_terminal.materials[i].shader = rec->luz;
}

void console_modelo_descarrega(Recursos *rec) {
    if (rec->tem_terminal) UnloadModel(rec->modelo_terminal);
}

bool console_modelo_desenha(const Estado *e) {
    if (!e->rec.tem_terminal) return false;
    const Model *m = &e->rec.modelo_terminal;
    BoundingBox caixa = GetModelBoundingBox(*m);
    float largura = fmaxf(0.01f, caixa.max.z - caixa.min.z); // de lado a lado (o modelo olha para +X)
    float escala = CONSOLE_TERMINAL_LARGURA_M / largura;
    Vector3 centro = Vector3Scale(Vector3Add(caixa.min, caixa.max), 0.5f);
    Vector3 con = e->nivel.console;
    // girar -90° em Y leva (x, y, z) do modelo para (-z, y, x): o +X dele vira o +Z do mundo, e o centro
    // do modelo cai no meio da mesa
    Vector3 pos = {con.x + centro.z * escala, CONSOLE_TAMPO_Y - caixa.min.y * escala,
                   con.z + CONSOLE_TERMINAL_Z - centro.x * escala};
    DrawModelEx(*m, pos, (Vector3){0, 1, 0}, -90.0f, (Vector3){escala, escala, escala}, WHITE);
    return true;
}
