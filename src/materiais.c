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
// Uma linha fina e torta (rachadura): o ruído com o domínio entortado, onde ele passa perto de 0,5.
// `fx`, `fy`: quantas vezes a rede de rachaduras repete na textura (a forma dela).
static float rachadura(float u, float v, int fx, int fy, unsigned s) {
    // dois entortamentos: um largo (o caminho) e um fino (o serrilhado da borda quebrada)
    float wu = u + 0.03f * (materiais_fbm(u, v, fx * 2, fy * 2, 2, s + 1u) - 0.5f) +
               0.006f * (materiais_fbm(u, v, fx * 24, fy * 24, 2, s + 3u) - 0.5f);
    float wv = v + 0.03f * (materiais_fbm(u, v, fx * 2, fy * 2, 2, s + 2u) - 0.5f) +
               0.006f * (materiais_fbm(u, v, fx * 24, fy * 24, 2, s + 4u) - 0.5f);
    float d = fabsf(materiais_fbm(wu, wv, fx, fy, 5, s) - 0.5f);
    float largura = 0.0035f * (0.2f + 1.3f * materiais_fbm(u, v, fx * 3, fy * 3, 2, s + 5u)); // afina e some
    return (1.0f - passo(largura * 0.35f, largura, d)) * passo(0.15f, 0.35f, largura / 0.0035f);
}

// Escorridos de sujeira descendo do teto: colunas finas de largura e comprimento sorteados, mais fortes em
// cima e falhando no caminho (como nas paredes de concreto abandonadas).
static float escorridos(float u, float v, int colunas, unsigned s) {
    // a coluna balança um pouco ao descer (a água não escorre em linha reta)
    float bu = u + (0.6f / (float)colunas) * (materiais_fbm(u, v, 8, 6, 3, s + 3u) - 0.5f);
    float coluna = materiais_fbm(bu, 0.0f, colunas, 1, 2, s);
    int grupos = colunas / 6; // colunas vizinhas descem mais ou menos juntas
    float comprimento = 0.2f + 0.75f * materiais_ruido(u * (float)grupos, 0.0f, grupos, 1, s + 1u);
    float quebra = passo(0.25f, 0.75f, materiais_fbm(bu, v, colunas / 2, 12, 3, s + 2u));
    float desce = 1.0f - passo(comprimento * 0.5f, comprimento, v); // some aos poucos, não de uma vez
    return passo(0.55f, 0.75f, coluna) * desce * quebra;
}

// Concreto de forma (6 m × 3 m): tons claros e escuros grandes (cura desigual), grão e poros, juntas a cada
// 0,6 m, furos de tirante, rachaduras em algumas regiões, escorridos que descem do teto e a umidade que
// sobe do chão com a linha da maré. A mistura leva à cor da umidade.
static PixelMaterial concreto(float u, float v, float sujeira, unsigned s) {
    float brilho = 0.55f + 0.9f * materiais_fbm(u, v, 6, 3, 6, s);
    brilho *= 0.8f + 0.4f * materiais_fbm(u, v, 48, 24, 3, s + 1u); // aspereza: o concreto não é liso
    brilho *= 0.92f + 0.16f * materiais_hash((int)(u * 2048.0f), (int)(v * 1024.0f), s + 6u);
    if (materiais_hash((int)(u * 1400.0f), (int)(v * 700.0f), s + 2u) > 0.993f) brilho *= 0.6f; // poro
    float faixa = v * 5.0f;
    brilho *= 1.0f - 0.25f * (1.0f - passo(0.0f, 0.02f, fabsf(faixa - floorf(faixa + 0.5f))));
    float du = fracao(u * 10.0f) - 0.5f, dv = fracao(faixa) - 0.5f;
    brilho *= 1.0f - 0.45f * (1.0f - passo(0.018f, 0.03f, sqrtf(du * du + dv * dv)));
    float regiao = passo(0.5f, 0.6f, materiais_fbm(u, v, 4, 2, 3, s + 3u));
    brilho *= 1.0f - 0.7f * sujeira * rachadura(u, v, 6, 3, s + 4u) * regiao;
    float escorre = escorridos(u, v, 96, s + 7u);
    float nivel = v + 0.2f * (materiais_fbm(u, v, 10, 4, 3, s + 5u) - 0.5f);
    float umido = passo(0.72f, 0.95f, nivel);
    float mare = fmaxf(0.0f, passo(0.68f, 0.71f, nivel) - passo(0.71f, 0.74f, nivel));
    brilho *= (1.0f - sujeira * (0.55f * escorre + 0.4f * umido)) * (1.0f - 0.25f * sujeira * mare);
    return (PixelMaterial){brilho, sujeira * fmaxf(0.85f * escorre, 0.85f * umido), 0.0f};
}

// Tijolo (12 fiadas na textura, meia-vez desencontradas): 1 no tijolo, 0 na argamassa.
static float tijolo(float u, float v, unsigned s) {
    float fiada = v * 24.0f;
    float deslocado = u * 12.0f + (((int)floorf(fiada) & 1) ? 0.5f : 0.0f);
    float bu = fracao(deslocado), bv = fracao(fiada);
    float borda = fminf(fminf(bu, 1.0f - bu) * 2.0f, fminf(bv, 1.0f - bv));
    float gasta = 0.05f * (materiais_fbm(u, v, 48, 48, 2, s) - 0.5f); // a argamassa esfarelou
    return passo(0.03f, 0.08f, borda + gasta);
}

// Tinta oliva com pinceladas, descascando em placas de contorno torto: a borda da tinta levanta (mais clara)
// e faz sombra no reboco logo ao lado; nas placas mais fundas aparece o tijolo. Escorridos passam por
// cima da tinta e o pé da parede fica encardido. A mistura (tinta que caiu) mostra o reboco por baixo.
static PixelMaterial oliva(float u, float v, float sujeira, unsigned s) {
    float brilho = 0.82f + 0.3f * materiais_fbm(u, v, 8, 8, 5, s + 11u);
    brilho *= 0.97f + 0.06f * materiais_ruido(u * 140.0f, v * 18.0f, 140, 18, s + 13u); // pinceladas
    brilho *= 0.9f + 0.2f * materiais_fbm(u, v, 64, 64, 2, s + 14u);
    float wu = u + 0.05f * (materiais_fbm(u, v, 12, 12, 2, s + 15u) - 0.5f);
    float p = materiais_fbm(wu, v, 6, 6, 5, s + 17u), limiar = 0.68f - 0.07f * sujeira;
    float descascado = passo(limiar, limiar + 0.008f, p);
    float lasca = fmaxf(0.0f, passo(limiar - 0.02f, limiar, p) - descascado);
    float sombra = descascado * (1.0f - passo(limiar, limiar + 0.045f, p));
    float fundo = passo(limiar + 0.07f, limiar + 0.1f, p) *
                  passo(0.4f, 0.6f, materiais_fbm(u, v, 16, 16, 2, s + 25u));
    float tb = tijolo(u, v, s + 27u), tij = 0.85f * fundo * tb;
    float reboco = 0.75f + 0.5f * materiais_fbm(u, v, 40, 40, 3, s + 29u); // reboco áspero e manchado
    brilho *= 1.0f + (reboco - 1.0f) * descascado;
    brilho *= (1.0f + 0.08f * lasca) * (1.0f - 0.25f * sombra) * (1.0f - 0.55f * fundo * (1.0f - tb));
    brilho *= 1.0f - 0.35f * tij * materiais_fbm(u, v, 24, 24, 3, s + 31u); // tijolo sujo, cada um de um tom
    brilho *= 1.0f - 0.55f * sujeira * escorridos(u, v, 64, s + 19u);
    float pe = passo(0.8f, 1.0f, v + 0.08f * (materiais_fbm(u, v, 16, 4, 3, s + 21u) - 0.5f));
    brilho *= 1.0f - 0.45f * sujeira * pe;
    brilho *= 1.0f - 0.6f * sujeira * rachadura(u, v, 4, 4, s + 23u) * passo(0.55f, 0.65f, p);
    return (PixelMaterial){brilho, Clamp(descascado + 0.6f * sujeira * pe, 0.0f, 1.0f), tij};
}

// Distância (0..0,5) do ponto à borda da placa numa grade de `n` × `n` placas.
static float borda_da_placa(float u, float v, int n) {
    float gx = fracao(u * (float)n), gy = fracao(v * (float)n);
    return fminf(fminf(gx, 1.0f - gx), fminf(gy, 1.0f - gy));
}

// Granilite (2 m, 4 × 4 placas de 0,5 m): rejunte escuro, tom por placa, grãos claros e escuros, placas
// trincadas, quinas lascadas e poeira acumulada (a mistura leva à cor da sujeira).
static PixelMaterial piso(float u, float v, float sujeira, unsigned s) {
    float rejunte = 1.0f - passo(0.012f, 0.026f, borda_da_placa(u, v, 4));
    int px = (int)(u * 4.0f), pz = (int)(v * 4.0f);
    float brilho = 0.84f + 0.3f * materiais_hash(px, pz, s + 3u);
    brilho *= 0.9f + 0.2f * materiais_fbm(u, v, 16, 16, 3, s + 4u);
    float grao = materiais_hash((int)(u * 520.0f), (int)(v * 520.0f), s + 5u);
    brilho *= grao > 0.93f ? 1.3f : grao < 0.05f ? 0.72f : 1.0f;
    bool trincada = materiais_hash(px, pz, s + 6u) < 0.35f;
    brilho *= 1.0f - (trincada ? 0.75f * rachadura(u, v, 4, 4, s + 7u) : 0.0f);
    float gx = fracao(u * 4.0f), gz = fracao(v * 4.0f); // a quina lascada de algumas placas
    float qx = fminf(gx, 1.0f - gx), qz = fminf(gz, 1.0f - gz), quina = sqrtf(qx * qx + qz * qz);
    float lascada = materiais_hash(px, pz, s + 8u) < 0.3f ? 1.0f - passo(0.05f, 0.07f, quina) : 0.0f;
    float poeira = passo(0.5f, 0.8f, materiais_fbm(u, v, 3, 3, 5, s + 9u)) * sujeira;
    brilho *= (1.0f - 0.3f * poeira) * (1.0f - 0.6f * rejunte) * (1.0f - 0.5f * lascada);
    return (PixelMaterial){brilho, Clamp(poeira + 0.7f * lascada, 0.0f, 1.0f), 0.0f};
}
// Forro (2 m, 3 × 3 placas): juntas, tom por placa e manchas de infiltração com anel mais escuro.
static PixelMaterial teto(float u, float v, float sujeira, unsigned s) {
    float junta = 1.0f - passo(0.01f, 0.025f, borda_da_placa(u, v, 3));
    float brilho = 0.9f + 0.15f * materiais_hash((int)(u * 3.0f), (int)(v * 3.0f), s + 19u);
    float f = materiais_fbm(u, v, 3, 3, 4, s + 21u), limiar = 0.64f - 0.06f * sujeira;
    float mancha = passo(limiar, limiar + 0.05f, f);
    float anel = fmaxf(0.0f, passo(limiar - 0.015f, limiar, f) - passo(limiar, limiar + 0.015f, f));
    brilho *= (1.0f - 0.55f * junta) * (1.0f - 0.3f * anel * sujeira);
    return (PixelMaterial){brilho, sujeira * fminf(1.0f, 0.6f * mancha + anel), 0.0f};
}

// Metal pintado (1 m), cinza neutro (a cor vem do objeto): escovado vertical, riscos claros e ferrugem.
static PixelMaterial metal(float u, float v, float sujeira, unsigned s) {
    float brilho = 0.82f + 0.12f * materiais_ruido(u * 8.0f, v * 96.0f, 8, 96, s + 31u) +
                   0.12f * materiais_fbm(u, v, 6, 6, 3, s + 33u);
    brilho *= 1.0f + 0.35f * passo(0.97f, 1.0f, materiais_ruido(u * 96.0f, v * 6.0f, 96, 6, s + 35u));
    float ferrugem = passo(0.68f - 0.08f * sujeira, 0.76f, materiais_fbm(u, v, 8, 8, 4, s + 37u)) * sujeira;
    return (PixelMaterial){brilho, ferrugem, 0.0f};
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
        return (PixelMaterial){1.0f, 0.0f, 0.0f};
    }
}

// Janela que zera o alfa perto das bordas do decalque (sem quina visível).
static float janela(float u, float v) {
    return passo(0.0f, 0.1f, u) * passo(0.0f, 0.1f, 1.0f - u) * passo(0.0f, 0.1f, v) *
           passo(0.0f, 0.1f, 1.0f - v);
}

// Mancha de umidade: borrão irregular com borda mais escura e escorridos descendo.
static float umidade(float u, float v, unsigned s) {
    float du = u - 0.5f, dv = (v - 0.35f) * 1.3f;
    float borda = sqrtf(du * du + dv * dv) + 0.3f * (materiais_fbm(u, v, 4, 4, 4, s) - 0.5f);
    float corpo = 1.0f - passo(0.22f, 0.36f, borda);
    float anel = fmaxf(0.0f, passo(0.28f, 0.33f, borda) - passo(0.33f, 0.38f, borda));
    float coluna = passo(0.66f, 0.86f, materiais_ruido(u * 18.0f, 0.0f, 18, 1, s + 3u));
    float pinga = coluna * passo(0.3f, 0.45f, v) * (1.0f - passo(0.45f, 0.95f, v));
    return fminf(1.0f, 0.7f * corpo + 0.4f * anel + 0.6f * pinga * (1.0f - corpo));
}

// Marcas de arrasto: quatro trilhas paralelas que somem longe do duto, falhando no caminho.
static float arrasto(float u, float v, unsigned s) {
    static const float TRILHAS[4] = {0.25f, 0.4f, 0.6f, 0.75f};
    float alfa = 0.0f;
    // padrão FOLD: as trilhas somadas num alfa só
    for (int i = 0; i < 4; i++) {
        float centro = TRILHAS[i] + 0.03f * (materiais_ruido(u * 6.0f, (float)i, 6, 4, s + 5u) - 0.5f);
        alfa += 1.0f - passo(0.025f, 0.05f, fabsf(v - centro));
    }
    float falha = passo(0.3f, 0.5f, materiais_fbm(u, v, 8, 2, 3, s + 7u));
    return fminf(1.0f, alfa) * passo(0.0f, 0.9f, u) * falha;
}

// Chão molhado: um borrão de contorno torto (o ruído desloca o raio), com ilhas secas no meio e a borda
// mais fina onde a água já secou.
static float molhado(float u, float v, unsigned s) {
    float du = u - 0.5f, dv = v - 0.5f;
    float torto = materiais_fbm(u, v, 3, 3, 5, s + 11u) - 0.5f;
    float raio = sqrtf(du * du + dv * dv) + 0.55f * torto;
    float corpo = 1.0f - passo(0.12f, 0.3f, raio);
    float seco = passo(0.6f, 0.68f, materiais_fbm(u, v, 6, 6, 3, s + 19u)); // ilhas onde o chão é mais alto
    float fino = 0.7f + 0.3f * materiais_fbm(u, v, 16, 16, 2, s + 23u);
    return corpo * (1.0f - 0.85f * seco) * fino;
}

// Escorrido: ao longo de u (o comprimento), um filete que serpenteia, alarga devagar e empoça no fim.
static float escorrido(float u, float v, unsigned s) {
    float centro = 0.5f + 0.22f * (materiais_fbm(u, 0.0f, 3, 1, 4, s + 29u) - 0.5f);
    float largura = 0.04f + 0.1f * u * u + 0.05f * materiais_fbm(u, 0.5f, 8, 1, 2, s + 31u);
    float filete = 1.0f - passo(largura * 0.6f, largura, fabsf(v - centro));
    float du = (u - 0.84f) * 1.6f, dv = v - centro;
    float torto = 0.1f * (materiais_fbm(u, v, 6, 6, 3, s) - 0.5f);
    float poca_fim = 1.0f - passo(0.09f, 0.16f, sqrtf(du * du + dv * dv) + torto);
    return fmaxf(filete, poca_fim) * passo(0.02f, 0.12f, u);
}

// Respingos: uma mancha pequena no meio e gotas soltas em volta (uma por célula, quando o sorteio deixa),
// cada uma do seu tamanho.
static float respingo(float u, float v, unsigned s) {
    const int n = 7;
    float gu = u * n, gv = v * n;
    int cx = (int)floorf(gu), cy = (int)floorf(gv);
    float pu = gu - cx - 0.2f - 0.6f * materiais_hash(cx, cy, s + 41u);
    float pv = gv - cy - 0.2f - 0.6f * materiais_hash(cx, cy, s + 43u);
    float raio = 0.06f + 0.22f * materiais_hash(cx, cy, s + 47u);
    float du = u - 0.5f, dv = v - 0.5f, longe = sqrtf(du * du + dv * dv);
    bool tem = materiais_hash(cx, cy, s + 53u) < 0.75f - longe * 1.2f; // mais gotas perto do meio
    float gota = tem ? 1.0f - passo(raio * 0.7f, raio, sqrtf(pu * pu + pv * pv)) : 0.0f;
    float mancha = 1.0f - passo(0.08f, 0.16f, longe + 0.2f * (materiais_fbm(u, v, 4, 4, 3, s + 59u) - 0.5f));
    return fmaxf(gota, mancha);
}

// Poça: água parada, de borda nítida e irregular; a borda é um pouco mais escura (a sujeira que ela junta).
static float poca(float u, float v, unsigned s) {
    float du = u - 0.5f, dv = v - 0.5f;
    float raio = sqrtf(du * du + dv * dv) + 0.2f * (materiais_fbm(u, v, 4, 4, 4, s + 13u) - 0.5f);
    float corpo = 1.0f - passo(0.33f, 0.355f, raio);
    float anel = passo(0.29f, 0.33f, raio) * (1.0f - passo(0.355f, 0.37f, raio));
    return fminf(1.0f, 0.8f * corpo + 0.2f * anel);
}

// Poeira no pé da parede: grossa encostada nela (v = 0), rala para o meio, em tufos, com grãos soltos.
static float poeira(float u, float v, unsigned s) {
    float tufos = 0.45f + 0.75f * materiais_fbm(u, v, 6, 2, 4, s + 61u);
    float encosta = 1.0f - passo(0.0f, 0.85f + 0.15f * materiais_fbm(u, 0.0f, 8, 1, 2, s + 63u), v);
    float grao = materiais_hash((int)(u * 256.0f), (int)(v * 256.0f), s + 67u) > 0.93f ? 0.5f : 0.0f;
    return fminf(1.0f, encosta * encosta * tufos + grao * (1.0f - v));
}

float materiais_mancha(Decalque d, float u, float v, unsigned semente) {
    float alfa = d == DECALQUE_UMIDADE   ? umidade(u, v, semente)
                 : d == DECALQUE_ARRASTO ? arrasto(u, v, semente)
                 : d == DECALQUE_MOLHADO   ? molhado(u, v, semente)
                 : d == DECALQUE_ESCORRIDO ? escorrido(u, v, semente)
                 : d == DECALQUE_RESPINGO  ? respingo(u, v, semente)
                 : d == DECALQUE_POCA      ? poca(u, v, semente)
                 : d == DECALQUE_POEIRA    ? poeira(u, v, semente)
                                           : 0.0f;
    return alfa * janela(u, v);
}

float materiais_desgaste(float u, float v, unsigned semente) {
    float manchas = 0.35f + 0.65f * passo(0.22f, 0.4f, materiais_fbm(fracao(u), fracao(v), 8, 4, 4, semente));
    float spray = materiais_hash((int)(u * 256.0f), (int)(v * 64.0f), semente + 1u) > 0.12f ? 1.0f : 0.4f;
    return manchas * spray;
}

// ================================================================== UV de mundo
// [REQ:PURA-materiais_uv] UV em metros: a mesma posição do mundo dá a mesma UV (nada estica, vizinhas
// emendam)
Vector2 materiais_uv(Vector3 ponto, int eixo, Vector2 metros, Vector2 desloca, float topo) {
    float mx = metros.x > 0.0f ? metros.x : 1.0f, my = metros.y > 0.0f ? metros.y : 1.0f;
    Vector2 uv = eixo == 0   ? (Vector2){ponto.z / mx, (topo - ponto.y) / my}
                 : eixo == 1 ? (Vector2){ponto.x / mx, ponto.z / my}
                             : (Vector2){ponto.x / mx, (topo - ponto.y) / my};
    return Vector2Add(uv, desloca);
}
