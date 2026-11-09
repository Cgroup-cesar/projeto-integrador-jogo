// audio_sintese.c — todos os sons gerados em código (22050 Hz, mono, 16 bits).
// Se existir assets/audio/<nome>.ogg, ele substitui o som sintetizado. Sem placa de som, tudo vira silêncio.
#include "audio_sintese.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TAXA    22050
#define DOIS_PI 6.28318530718f

static const char *const NOMES[SOM_TOTAL] = {
    "zumbido_lampada", "voz_precursor", "verdadeiro",     "falso",        "bip_interfone", "maquina_escrever",
    "interruptor",     "passo",         "passo_criatura", "vidro_estalo", "estrondo",      "alarme",
    "respiracao",      "ambiente_sala", "ambiente_saida", "papel",        "energia",       "distorcao",
};

// ================================================================== ferramentas de síntese
typedef struct {
    float *amostras;
    int n;
} Buffer;

static unsigned semente_ruido = 1985u;

static float ruido(void) {
    semente_ruido = semente_ruido * 1664525u + 1013904223u;
    return ((semente_ruido >> 9) & 0xFFFF) / 32768.0f - 1.0f;
}

static float envelope(float t, float ataque, float duracao) {
    if (t < ataque) return t / ataque;
    float resto = (duracao - t) / fmaxf(0.0001f, duracao - ataque);
    return resto < 0.0f ? 0.0f : resto;
}

// Filtro passa-baixa de um polo: deixa o ruído "abafado" (passos, respiração, estrondo).
static float filtra(float *estado, float entrada, float suavidade) {
    *estado += (entrada - *estado) * suavidade;
    return *estado;
}

// ================================================================== cada som
static void zumbido(Buffer *b) { // zumbido de lâmpada: 60 Hz e harmônicos, com um chiado leve
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        b->amostras[i] = 0.25f * sinf(DOIS_PI * 60 * t) + 0.18f * sinf(DOIS_PI * 120 * t) +
                         0.08f * sinf(DOIS_PI * 180 * t) + 0.03f * ruido();
    }
}

static void voz(Buffer *b) { // 3 osciladores FM com formantes que variam: não soa como palavra nenhuma
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA, d = (float)b->n / TAXA;
        float base = 95.0f + 25.0f * sinf(DOIS_PI * 0.7f * t);
        float fm = sinf(DOIS_PI * base * t + 3.0f * sinf(DOIS_PI * base * 1.5f * t));
        float formante1 = sinf(DOIS_PI * (550 + 250 * sinf(DOIS_PI * 2.3f * t)) * t);
        float formante2 = sinf(DOIS_PI * (1250 + 400 * sinf(DOIS_PI * 1.1f * t)) * t);
        float silabas = 0.5f + 0.5f * sinf(DOIS_PI * 4.2f * t);
        b->amostras[i] =
            envelope(t, 0.08f, d) * (0.55f * fm + 0.25f * fm * formante1 + 0.15f * fm * formante2) * silabas;
    }
}

static void verdadeiro(Buffer *b) { // acorde grave consonante (tônica, quinta, oitava)
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float s = sinf(DOIS_PI * 110 * t) + 0.7f * sinf(DOIS_PI * 165 * t) + 0.5f * sinf(DOIS_PI * 220 * t);
        b->amostras[i] = 0.35f * envelope(t, 0.05f, 0.6f) * s;
    }
}

static void falso(Buffer *b) { // guincho dissonante subindo (trítono)
    float fase1 = 0, fase2 = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float f = 650.0f + 1300.0f * t / 0.5f;
        fase1 += DOIS_PI * f / TAXA;
        fase2 += DOIS_PI * f * 1.414f / TAXA;
        float serra = fmodf(fase1, DOIS_PI) / DOIS_PI * 2.0f - 1.0f;
        b->amostras[i] = 0.3f * envelope(t, 0.02f, 0.5f) * (serra + sinf(fase2) + 0.2f * ruido());
    }
}

static void bip(Buffer *b) { // bip do interfone com chiado de rádio
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        b->amostras[i] = (t < 0.12f ? 0.4f * sinf(DOIS_PI * 1050 * t) : 0.0f) +
                         0.12f * ruido() * envelope(t, 0.01f, 0.25f);
    }
}

static void clique(Buffer *b, float tom, float corpo) { // máquina de escrever e interruptor
    float lp = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float queda = expf(-t * 90.0f);
        b->amostras[i] = queda * (0.6f * filtra(&lp, ruido(), 0.5f) + corpo * sinf(DOIS_PI * tom * t));
    }
}

static void passo(Buffer *b, float grave, float queda) { // baque abafado; o da criatura é mais grave e lento
    float lp = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float e = expf(-t * queda);
        b->amostras[i] = e * (0.8f * filtra(&lp, ruido(), 0.08f) + 0.6f * sinf(DOIS_PI * grave * t));
    }
}

static void estalo(Buffer *b) { // estalo de vidro: ruído seco e parciais agudos que soam um pouco
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float seco = expf(-t * 60.0f) * ruido();
        float anel = expf(-t * 9.0f) * (sinf(DOIS_PI * 2300 * t) + 0.6f * sinf(DOIS_PI * 3700 * t) +
                                        0.4f * sinf(DOIS_PI * 5100 * t));
        b->amostras[i] = 0.6f * seco + 0.2f * anel;
    }
}

static void estrondo(Buffer *b) { // o susto: baque grave, ruído e um guincho por cima
    float lp = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float baque =
            expf(-t * 3.0f) * (sinf(DOIS_PI * (55 - 20 * t) * t) + filtra(&lp, ruido(), 0.05f) * 1.5f);
        float guincho = expf(-t * 2.5f) * sinf(DOIS_PI * (900 + 600 * sinf(DOIS_PI * 7 * t)) * t);
        b->amostras[i] = 0.58f * baque + 0.3f * guincho;
    }
}

static void alarme(Buffer *b) { // sirene de dois tons (laço de 1 s)
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float f = t < 0.5f ? 640.0f : 480.0f;
        b->amostras[i] =
            0.3f * (sinf(DOIS_PI * f * t) > 0 ? 1.0f : -1.0f) * 0.6f + 0.2f * sinf(DOIS_PI * f * 2 * t);
    }
}

static void respiracao(Buffer *b) { // inspira, expira (laço de 2 s), ruído abafado
    float lp = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float folego = powf(fabsf(sinf(DOIS_PI * 0.5f * t)), 2.0f);
        b->amostras[i] = 0.6f * folego * filtra(&lp, ruido(), 0.12f);
    }
}

static void ambiente(Buffer *b, float grave,
                     float metal) { // drone em laço; nas saídas, mais escuro e com metal
    float lp = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float drone =
            sinf(DOIS_PI * grave * t) + 0.5f * sinf(DOIS_PI * grave * 1.5f * t + sinf(DOIS_PI * 0.25f * t));
        float rumor = filtra(&lp, ruido(), 0.02f) * 2.0f;
        float rangido = metal * sinf(DOIS_PI * 420 * t + 4 * sinf(DOIS_PI * 0.5f * t)) *
                        powf(fmaxf(0.0f, sinf(DOIS_PI * 0.25f * t)), 12.0f);
        b->amostras[i] = 0.22f * drone + 0.25f * rumor + 0.12f * rangido;
    }
}

static void papel(Buffer *b) {
    float lp = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float rajadas = fmaxf(0.0f, sinf(DOIS_PI * 13 * t)) * envelope(t, 0.02f, 0.3f);
        b->amostras[i] = 0.5f * rajadas * (ruido() - filtra(&lp, ruido(), 0.3f));
    }
}

static void energia(Buffer *b) { // a energia volta: varredura subindo e o zumbido entrando
    float fase = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        fase += DOIS_PI * (70 + 330 * (t / 1.2f)) / TAXA;
        b->amostras[i] = 0.35f * envelope(t, 0.1f, 1.2f) * (sinf(fase) + 0.4f * sinf(DOIS_PI * 120 * t));
    }
}

static void distorcao(Buffer *b) {
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float f = 200 + 150 * sinf(DOIS_PI * 9 * t);
        b->amostras[i] = 0.4f * envelope(t, 0.05f, 1.0f) * sinf(DOIS_PI * f * t) * sinf(DOIS_PI * 31 * t);
    }
}

// ================================================================== montagem
static float duracao(IdSom som) {
    static const float d[SOM_TOTAL] = {1.0f,  1.6f, 0.6f, 0.5f, 0.25f, 0.05f, 0.06f, 0.14f, 0.3f,
                                       0.45f, 1.2f, 1.0f, 2.0f, 4.0f,  4.0f,  0.3f,  1.2f,  1.0f};
    return d[som];
}

// Exceção ao limite de ~40 linhas: é só um caso por som, sem lógica (cada som tem a sua função acima).
static void sintetiza(IdSom som, Buffer *b) {
    switch (som) {
    case SOM_ZUMBIDO_LAMPADA:
        zumbido(b);
        break;
    case SOM_VOZ_PRECURSOR:
        voz(b);
        break;
    case SOM_VERDADEIRO:
        verdadeiro(b);
        break;
    case SOM_FALSO:
        falso(b);
        break;
    case SOM_BIP_INTERFONE:
        bip(b);
        break;
    case SOM_MAQUINA_ESCREVER:
        clique(b, 3100.0f, 0.3f);
        break;
    case SOM_INTERRUPTOR:
        clique(b, 900.0f, 0.5f);
        break;
    case SOM_PASSO:
        passo(b, 85.0f, 30.0f);
        break;
    case SOM_PASSO_CRIATURA:
        passo(b, 48.0f, 12.0f);
        break;
    case SOM_VIDRO_ESTALO:
        estalo(b);
        break;
    case SOM_ESTRONDO:
        estrondo(b);
        break;
    case SOM_ALARME:
        alarme(b);
        break;
    case SOM_RESPIRACAO:
        respiracao(b);
        break;
    case SOM_AMBIENTE_SALA:
        ambiente(b, 55.0f, 0.0f);
        break;
    case SOM_AMBIENTE_SAIDA:
        ambiente(b, 41.0f, 1.0f);
        break;
    case SOM_PAPEL:
        papel(b);
        break;
    case SOM_ENERGIA:
        energia(b);
        break;
    case SOM_DISTORCAO:
        distorcao(b);
        break;
    default:
        break;
    }
}

// Converte as amostras (−1..1) em 16 bits numa Wave da raylib.
static Wave onda_sintetizada(IdSom som) {
    Buffer b = {0};
    b.n = (int)(duracao(som) * TAXA);
    b.amostras = calloc((size_t)b.n, sizeof(float));
    short *pcm = RL_MALLOC((size_t)b.n * sizeof(short));
    if (b.amostras == NULL || pcm == NULL) {
        free(b.amostras);
        RL_FREE(pcm);
        return (Wave){0};
    }
    sintetiza(som, &b);
    for (int i = 0; i < b.n; i++)
        pcm[i] = (short)(fmaxf(-1.0f, fminf(1.0f, b.amostras[i])) * 30000.0f);
    free(b.amostras);
    return (Wave){(unsigned int)b.n, TAXA, 16, 1, pcm};
}

static Sound som_sintetizado(IdSom som) {
    Wave onda = onda_sintetizada(som);
    if (onda.data == NULL) return (Sound){0};
    Sound s = LoadSoundFromWave(onda);
    UnloadWave(onda);
    return s;
}

void audio_exporta(const char *pasta) {
    for (int i = 0; i < SOM_TOTAL; i++) {
        char caminho[512];
        snprintf(caminho, sizeof(caminho), "%s/%s.wav", pasta, NOMES[i]);
        Wave onda = onda_sintetizada((IdSom)i);
        if (onda.data != NULL && ExportWave(onda, caminho)) printf("%s\n", caminho);
        UnloadWave(onda);
    }
}

void audio_inicia(Recursos *rec) {
    InitAudioDevice();
    rec->audio_ok = IsAudioDeviceReady();
    if (!rec->audio_ok) {
        TraceLog(LOG_WARNING, "PRECURSOR: sem dispositivo de áudio; o jogo segue em silêncio");
        return;
    }
    for (int i = 0; i < SOM_TOTAL; i++) {
        char relativo[64], caminho[512];
        snprintf(relativo, sizeof(relativo), "audio/%s.ogg", NOMES[i]);
        if (ui_caminho_asset(relativo, caminho, sizeof(caminho)))
            rec->sons[i] = LoadSound(caminho); // o .ogg substitui
        if (!IsSoundValid(rec->sons[i])) rec->sons[i] = som_sintetizado((IdSom)i);
    }
}

void audio_finaliza(Recursos *rec) {
    if (!rec->audio_ok) return;
    for (int i = 0; i < SOM_TOTAL; i++)
        if (IsSoundValid(rec->sons[i])) UnloadSound(rec->sons[i]);
    CloseAudioDevice();
}

// ================================================================== tocar
void audio_toca(const Estado *e, IdSom som, float volume, float tom) {
    if (!e->rec.audio_ok || som < 0 || som >= SOM_TOTAL) return;
    Sound s = e->rec.sons[som];
    SetSoundVolume(s, fmaxf(0.0f, fminf(1.0f, volume)));
    SetSoundPitch(s, tom);
    PlaySound(s);
}

void audio_para(const Estado *e, IdSom som) {
    if (e->rec.audio_ok && IsSoundPlaying(e->rec.sons[som])) StopSound(e->rec.sons[som]);
}

void audio_silencia(const Estado *e) {
    for (int i = 0; i < SOM_TOTAL; i++)
        audio_para(e, (IdSom)i);
}

// ================================================================== laços (ambiência por fase, zumbido,
// alarme, respiração)
static void mantem_laco(const Estado *e, IdSom som, bool ligado, float volume) {
    if (!ligado) {
        audio_para(e, som);
        return;
    }
    Sound s = e->rec.sons[som];
    SetSoundVolume(s, volume);
    if (!IsSoundPlaying(s)) {
        SetSoundPitch(s, 1.0f);
        PlaySound(s);
    }
}

void audio_atualiza(Estado *e, float dt) {
    (void)dt;
    if (!e->rec.audio_ok) return;
    bool silencio =
        e->silencio_t > 0.0f || (e->fase == FASE_FIM_BOM && e->fim.tempo > 2.8f && e->fim.tempo < 4.8f);
    bool saida = regras_fase_eh_saida(e->fase);
    bool sala = !saida && e->fase != FASE_FIM_RUIM && !(e->fase == FASE_INTRO && !e->intro.interfone);
    bool lampada_acesa = e->apagao_t <= 0.0f && (sala || (saida && e->painel.resolvido));
    mantem_laco(e, SOM_AMBIENTE_SALA, !silencio && sala, 0.35f);
    mantem_laco(e, SOM_AMBIENTE_SAIDA, !silencio && (saida || e->fase == FASE_FIM_RUIM), 0.5f);
    mantem_laco(e, SOM_ZUMBIDO_LAMPADA, !silencio && lampada_acesa, e->falha_luz_t > 0.0f ? 0.3f : 0.12f);
    mantem_laco(e, SOM_ALARME, !silencio && e->alarme_t > 0.0f, 0.35f);
    mantem_laco(e, SOM_RESPIRACAO, !silencio && e->jog.modo == MODO_ARMARIO, 0.55f);
}
