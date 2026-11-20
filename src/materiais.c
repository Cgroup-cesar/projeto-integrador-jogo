// materiais.c — texturas procedurais como funções puras: ruído, máscaras de cada material e UV de mundo.
// Não inclui raylib.h (só raymath.h): testado sem janela em tests/test_regras.c.
#include "materiais.h"
#include <math.h>

// ================================================================== ruído
// [REQ:PURA-materiais] ruído determinístico e contínuo nas bordas: a textura repete sem emenda
float materiais_hash(int x, int y, unsigned semente) {
    unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u + semente * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xffffffu) / 16777216.0f;
}

static int modulo(int a, int p) {
    int r = a % p;
    return r < 0 ? r + p : r;
}

// smoothstep: 0 abaixo de a, 1 acima de b, curva suave no meio
static float passo(float a, float b, float x) {
    float t = Clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float materiais_ruido(float x, float y, int periodo_x, int periodo_y, unsigned semente) {
    int px = periodo_x < 1 ? 1 : periodo_x, py = periodo_y < 1 ? 1 : periodo_y;
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float sx = passo(0.0f, 1.0f, x - (float)x0), sy = passo(0.0f, 1.0f, y - (float)y0);
    int xa = modulo(x0, px), xb = modulo(x0 + 1, px), ya = modulo(y0, py), yb = modulo(y0 + 1, py);
    float cima = Lerp(materiais_hash(xa, ya, semente), materiais_hash(xb, ya, semente), sx);
    float baixo = Lerp(materiais_hash(xa, yb, semente), materiais_hash(xb, yb, semente), sx);
    return Lerp(cima, baixo, sy);
}

float materiais_fbm(float u, float v, int freq_x, int freq_y, int oitavas, unsigned semente) {
    float soma = 0.0f, peso = 0.5f, total = 0.0f;
    // padrão FOLD: cada oitava (o dobro da frequência, metade do peso) acumulada na soma
    for (int o = 0; o < oitavas; o++) {
        int fx = freq_x << o, fy = freq_y << o;
        soma += peso * materiais_ruido(u * (float)fx, v * (float)fy, fx, fy, semente + (unsigned)o * 101u);
        total += peso;
        peso *= 0.5f;
    }
    return total > 0.0f ? soma / total : 0.0f;
}

