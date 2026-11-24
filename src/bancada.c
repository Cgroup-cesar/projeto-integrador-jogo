// bancada.c — ferramentas de bancada (ver bancada.h): ponto de vista fixo para as capturas e medição de FPS.
#include "bancada.h"
#include "jogador.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_AMOSTRAS 20000 // quadros guardados na medição (5 min a 60 FPS)

static struct {
    bool olhar, lanterna, energia;
    float x, z, yaw, pitch;
    float medir_s; // 0 = não mede
    float tempo;
    int quadros, n;
    float amostras[MAX_AMOSTRAS]; // tempo de cada quadro, em segundos
} banca;

bool bancada_le_argumento(int argc, char **argv, int *i) {
    const char *a = argv[*i];
    if (strcmp(a, "--olhar") == 0 && *i + 1 < argc) {
        banca.olhar = sscanf(argv[++*i], "%f,%f,%f,%f", &banca.x, &banca.z, &banca.yaw, &banca.pitch) == 4;
        return true;
    }
    if (strcmp(a, "--medir") == 0 && *i + 1 < argc) {
        banca.medir_s = (float)atof(argv[++*i]);
        return true;
    }
    if (strcmp(a, "--lanterna") == 0) return banca.lanterna = true;
    if (strcmp(a, "--energia") == 0) return banca.energia = true;
    return false;
}

void bancada_aplica(Estado *e) {
    if (banca.olhar) {
        jogador_teleporta(e, (Vector3){banca.x, 0.0f, banca.z}, banca.yaw);
        e->jog.pitch = banca.pitch;
    }
    if (banca.lanterna) e->lanterna.ligada = true;
    if (banca.energia) e->painel.resolvido = true;
    if (banca.medir_s > 0.0f) { // medir o custo de verdade: sem trava de 60 FPS nem vsync
        SetTargetFPS(0);
        ClearWindowState(FLAG_VSYNC_HINT);
    }
}

static int compara(const void *a, const void *b) {
    float x = *(const float *)a, y = *(const float *)b;
    return (x < y) - (x > y); // do mais lento para o mais rápido
}

bool bancada_quadro(float tempo_real) {
    if (banca.medir_s <= 0.0f) return false;
    if (++banca.quadros <= 60) return false; // os primeiros quadros (shaders, envio de texturas) não contam
    banca.tempo += tempo_real;
    if (banca.n < MAX_AMOSTRAS) banca.amostras[banca.n++] = tempo_real;
    if (banca.tempo < banca.medir_s || banca.n == 0) return false;
    double soma = 0.0;
    for (int k = 0; k < banca.n; k++) // padrão FOLD: soma dos tempos
        soma += banca.amostras[k];
    qsort(banca.amostras, (size_t)banca.n, sizeof(float), compara);
    int um_porcento = banca.n / 100 > 0 ? banca.n / 100 : 1;
    double pior = 0.0;
    for (int k = 0; k < um_porcento; k++) // padrão FOLD: média do 1% mais lento
        pior += banca.amostras[k];
    double media_ms = soma / banca.n * 1000.0, pior_ms = pior / um_porcento * 1000.0;
    printf("MEDIDA: %d quadros, %.2f ms em média (%.1f FPS), pior 1%%: %.2f ms (%.1f FPS)\n", banca.n, media_ms,
           1000.0 / media_ms, pior_ms, 1000.0 / pior_ms);
    return true;
}
