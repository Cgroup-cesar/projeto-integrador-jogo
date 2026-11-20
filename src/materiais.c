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

static float fracao(float x) {
    return x - floorf(x);
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

// ================================================================== materiais
// Concreto de forma (6 m × 3 m): manchado, juntas a cada 0,6 m, furos de tirante, escorridos que descem
// do teto e umidade subindo do chão. A mistura leva à cor da umidade.
static PixelMaterial concreto(float u, float v, float sujeira, unsigned s) {
    float brilho = 0.8f + 0.4f * materiais_fbm(u, v, 12, 6, 4, s);
    float faixa = v * 5.0f;
    brilho *= 1.0f - 0.35f * (1.0f - passo(0.0f, 0.03f, fabsf(faixa - floorf(faixa + 0.5f))));
    float du = fracao(u * 10.0f) - 0.5f, dv = fracao(faixa) - 0.5f;
    brilho *= 1.0f - 0.4f * (1.0f - passo(0.02f, 0.035f, sqrtf(du * du + dv * dv)));
    float coluna = materiais_ruido(u * 48.0f, 0.0f, 48, 1, s + 7u);
    float quebra = 0.5f + 0.5f * materiais_fbm(u, v, 24, 3, 2, s + 9u);
    float escorre = passo(0.62f, 0.85f, coluna) * powf(1.0f - v * 0.85f, 1.5f) * quebra;
    float umido = passo(0.62f, 0.95f, v + 0.25f * (materiais_fbm(u, v, 16, 4, 3, s + 5u) - 0.5f));
    brilho *= 1.0f - sujeira * (0.4f * escorre + 0.35f * umido);
    return (PixelMaterial){brilho, sujeira * fmaxf(0.7f * escorre, 0.8f * umido)};
}

// Tinta oliva com pinceladas, descascando em placas de borda escura; encardida perto do chão.
// A mistura (tinta que caiu) mostra o concreto por baixo.
static PixelMaterial oliva(float u, float v, float sujeira, unsigned s) {
    float brilho = 0.88f + 0.22f * materiais_fbm(u, v, 24, 12, 3, s + 11u);
    brilho *= 0.95f + 0.1f * materiais_ruido(u * 6.0f, v * 160.0f, 6, 160, s + 13u);
    float p = materiais_fbm(u, v, 24, 24, 4, s + 17u), limiar = 0.74f - 0.08f * sujeira;
    float descascado = passo(limiar, limiar + 0.015f, p);
    float borda = fmaxf(0.0f, passo(limiar - 0.04f, limiar, p) - descascado);
    brilho *= (1.0f - 0.45f * borda) * (1.0f - 0.3f * sujeira * passo(0.85f, 1.0f, v));
    return (PixelMaterial){brilho, descascado};
}

// Distância (0..0,5) do ponto à borda da placa numa grade de `n` × `n` placas.
static float borda_da_placa(float u, float v, int n) {
    float gx = fracao(u * (float)n), gy = fracao(v * (float)n);
    return fminf(fminf(gx, 1.0f - gx), fminf(gy, 1.0f - gy));
}

// Granilite (2 m, 4 × 4 placas de 0,5 m): rejunte escuro, tom por placa, grãos claros e escuros e manchas
// de desgaste (a mistura leva à cor da mancha).
static PixelMaterial piso(float u, float v, float sujeira, unsigned s) {
    float rejunte = 1.0f - passo(0.015f, 0.03f, borda_da_placa(u, v, 4));
    float brilho = 0.88f + 0.24f * materiais_hash((int)(u * 4.0f), (int)(v * 4.0f), s + 3u);
    float grao = materiais_hash((int)(u * 320.0f), (int)(v * 320.0f), s + 5u);
    brilho *= grao > 0.94f ? 1.3f : grao < 0.04f ? 0.75f : 1.0f;
    float mancha = passo(0.58f, 0.75f, materiais_fbm(u, v, 4, 4, 3, s + 9u)) * sujeira;
    brilho *= (1.0f - 0.35f * mancha) * (1.0f - 0.6f * rejunte);
    return (PixelMaterial){brilho, mancha};
}

// Forro (2 m, 3 × 3 placas): juntas, tom por placa e manchas de infiltração com anel mais escuro.
static PixelMaterial teto(float u, float v, float sujeira, unsigned s) {
    float junta = 1.0f - passo(0.01f, 0.025f, borda_da_placa(u, v, 3));
    float brilho = 0.9f + 0.15f * materiais_hash((int)(u * 3.0f), (int)(v * 3.0f), s + 19u);
    float f = materiais_fbm(u, v, 3, 3, 4, s + 21u), limiar = 0.64f - 0.06f * sujeira;
    float mancha = passo(limiar, limiar + 0.05f, f);
    float anel = fmaxf(0.0f, passo(limiar - 0.015f, limiar, f) - passo(limiar, limiar + 0.015f, f));
    brilho *= (1.0f - 0.55f * junta) * (1.0f - 0.3f * anel * sujeira);
    return (PixelMaterial){brilho, sujeira * fminf(1.0f, 0.6f * mancha + anel)};
}

// Metal pintado (1 m), cinza neutro (a cor vem do objeto): escovado vertical, riscos claros e ferrugem.
static PixelMaterial metal(float u, float v, float sujeira, unsigned s) {
    float brilho = 0.82f + 0.12f * materiais_ruido(u * 8.0f, v * 96.0f, 8, 96, s + 31u) +
                   0.12f * materiais_fbm(u, v, 6, 6, 3, s + 33u);
    brilho *= 1.0f + 0.35f * passo(0.97f, 1.0f, materiais_ruido(u * 96.0f, v * 6.0f, 96, 6, s + 35u));
    float ferrugem = passo(0.68f - 0.08f * sujeira, 0.76f, materiais_fbm(u, v, 8, 8, 4, s + 37u)) * sujeira;
    return (PixelMaterial){brilho, ferrugem};
}

// Porta (UV local da face): duas almofadas com friso escuro, mão encardida perto da maçaneta, pé riscado.
static PixelMaterial porta(float u, float v, float sujeira, unsigned s) {
    PixelMaterial m = metal(u, v, sujeira, s + 40u);
    bool dentro_u = u > 0.12f && u < 0.88f;
    bool almofada = dentro_u && ((v > 0.08f && v < 0.45f) || (v > 0.55f && v < 0.92f));
    float bu = fminf(fabsf(u - 0.12f), fabsf(u - 0.88f));
    float bv = fminf(fminf(fabsf(v - 0.08f), fabsf(v - 0.45f)), fminf(fabsf(v - 0.55f), fabsf(v - 0.92f)));
    bool friso = (dentro_u && bv < 0.012f) || (bu < 0.012f && v > 0.08f && v < 0.92f);
    m.brilho *= friso ? 0.55f : almofada ? 0.92f : 1.0f;
    float mao = passo(0.7f, 0.78f, u) * (1.0f - passo(0.6f, 0.68f, fabsf(v - 0.5f) + 0.55f));
    m.brilho *= 1.0f - 0.3f * sujeira * mao;
    m.brilho *=
        1.0f - 0.25f * sujeira * passo(0.85f, 1.0f, v) * materiais_ruido(u * 40.0f, v * 4.0f, 40, 4, s);
    return m;
}

PixelMaterial materiais_pixel(TipoMaterial m, float u, float v, float sujeira, unsigned semente) {
    u = fracao(u);
    v = fracao(v);
    sujeira = Clamp(sujeira, 0.0f, 1.0f);
    switch (m) {
    case MATERIAL_CONCRETO:
        return concreto(u, v, sujeira, semente);
    case MATERIAL_OLIVA:
        return oliva(u, v, sujeira, semente);
    case MATERIAL_PISO:
        return piso(u, v, sujeira, semente);
    case MATERIAL_TETO:
        return teto(u, v, sujeira, semente);
    case MATERIAL_METAL:
        return metal(u, v, sujeira, semente);
    case MATERIAL_PORTA:
        return porta(u, v, sujeira, semente);
    default:
        return (PixelMaterial){1.0f, 0.0f};
    }
}

