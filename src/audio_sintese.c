// audio_sintese.c — todos os sons gerados em código (22050 Hz, mono, 16 bits).
// Se existir assets/audio/<nome>.ogg (ou .wav, .mp3), ele substitui o som sintetizado. Sem placa de som,
// tudo vira silêncio.
#include "audio_sintese.h"
#include "ui.h"
#include "jogador.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TAXA    22050
#define DOIS_PI 6.28318530718f

static const char *const NOMES[SOM_TOTAL] = {
    "zumbido_lampada", "voz_precursor",    "verdadeiro",     "falso",
    "bip_interfone",   "maquina_escrever", "interruptor",    "passo",
    "passo_criatura",  "vidro_estalo",     "estrondo",       "alarme",
    "respiracao",      "ambiente_sala",    "ambiente_saida", "papel",
    "energia",         "distorcao",        "jumpscare",      "respiracao_ofegante",
    "gota",            "camera_motor",
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

static void passo(Buffer *b, float grave, float queda) { // baque abafado do passo do jogador
    float lp = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float e = expf(-t * queda);
        b->amostras[i] = e * (0.8f * filtra(&lp, ruido(), 0.08f) + 0.6f * sinf(DOIS_PI * grave * t));
    }
}

// Passo da criatura: o corpo grave (que desce de tom) dá o peso; o estalo médio por cima é o que se ouve
// num fone ou na caixa de um notebook, que quase não tocam o grave sozinho.
static void passo_pesado(Buffer *b) {
    float lp = 0, lp_medio = 0;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        float corpo = expf(-t * 11.0f) * sinf(DOIS_PI * (120.0f - 60.0f * t) * t);
        float baque = expf(-t * 16.0f) * filtra(&lp, ruido(), 0.1f);
        float estalo = expf(-t * 50.0f) * filtra(&lp_medio, ruido(), 0.35f);
        b->amostras[i] = tanhf(2.2f * (0.6f * corpo + 0.9f * baque + 0.5f * estalo)); // satura: mais cheio
    }
}

// Pingo na poça: o "plim" de uma bolha (o tom sobe rápido e some) e um toque seco na água.
static void gota(Buffer *b) {
    float fase = 0.0f;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        fase += DOIS_PI * (700.0f + 1300.0f * (1.0f - expf(-t * 55.0f))) / TAXA;
        float bolha = expf(-t * 32.0f) * sinf(fase);
        float toque = expf(-t * 380.0f) * ruido();
        b->amostras[i] = 0.6f * bolha + 0.3f * toque;
    }
}

// O motor de uma câmera digital (como o do zoom e do foco): um zumbido agudo e liso de engrenagem pequena,
// que oscila devagar, com o tique fino dos dentes e um sopro leve. Entra e sai em 30 ms, para emendar nele
// mesmo enquanto a câmera gira (assets/audio/camera_motor.* substitui, como os outros sons).
static void motor(Buffer *b) {
    float fase = 0.0f, lp = 0.0f, d = (float)b->n / TAXA;
    for (int i = 0; i < b->n; i++) {
        float t = (float)i / TAXA;
        fase += DOIS_PI * (1150.0f + 60.0f * sinf(DOIS_PI * 2.5f * t)) / TAXA;
        float zumbido = 0.55f * sinf(fase) + 0.2f * sinf(2.0f * fase + 0.5f);
        float dentes = 0.65f + 0.35f * sinf(DOIS_PI * 180.0f * t);
        float sopro = filtra(&lp, ruido(), 0.6f);
        float forma = fminf(1.0f, t / 0.03f) * fminf(1.0f, (d - t) / 0.03f);
        b->amostras[i] = forma * (0.3f * zumbido * dentes + 0.08f * sopro);
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
    static const float d[SOM_TOTAL] = {1.0f, 1.6f, 0.6f, 0.5f, 0.25f, 0.05f, 0.06f, 0.14f, 0.3f, 0.45f,
                                       1.2f, 1.0f, 2.0f, 4.0f, 4.0f,  0.3f,  1.2f,  1.0f,  1.2f,
                                       0.0f, 0.3f, 1.2f}; // a respiração ofegante só existe como arquivo
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
        passo_pesado(b);
        break;
    case SOM_GOTA:
        gota(b);
        break;
    case SOM_CAMERA_MOTOR:
        motor(b);
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
    case SOM_JUMPSCARE: // sem o arquivo, o susto usa o mesmo estrondo do apagão
        estrondo(b);
        break;
    default:
        break;
    }
}

// Converte as amostras (−1..1) em 16 bits numa Wave da raylib.
static Wave onda_sintetizada(IdSom som) {
    Buffer b = {0};
    b.n = (int)(duracao(som) * TAXA);
    if (b.n <= 0) return (Wave){0}; // som que so existe como arquivo
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

// assets/audio/<nome>.ogg, .wav ou .mp3 (o primeiro que existir); sem nenhum, um Sound inválido.
static Sound som_do_arquivo(const char *nome) {
    static const char *const EXTENSOES[] = {"ogg", "wav", "mp3"};
    for (int k = 0; k < 3; k++) { // padrão FILTER: fica a primeira extensão que existe
        char relativo[64], caminho[512];
        snprintf(relativo, sizeof(relativo), "audio/%s.%s", nome, EXTENSOES[k]);
        if (ui_caminho_asset(relativo, caminho, sizeof(caminho))) return LoadSound(caminho);
    }
    return (Sound){0};
}

void audio_inicia(Recursos *rec) {
    InitAudioDevice();
    rec->audio_ok = IsAudioDeviceReady();
    if (!rec->audio_ok) {
        TraceLog(LOG_WARNING, "PRECURSOR: sem dispositivo de áudio; o jogo segue em silêncio");
        return;
    }
    for (int i = 0; i < SOM_TOTAL; i++) { // padrão MAP: cada nome → um som (do arquivo ou sintetizado)
        rec->sons[i] = som_do_arquivo(NOMES[i]);
        bool do_arquivo = IsSoundValid(rec->sons[i]);
        if (!do_arquivo) rec->sons[i] = som_sintetizado((IdSom)i);
        // O susto dura até o fim do trecho alto do som (medido no arquivo); o sintetizado é mais curto.
        if (i == SOM_JUMPSCARE) rec->susto_s = do_arquivo ? JUMPSCARE_AUDIO_S : JUMPSCARE_S;
    }
}

void audio_finaliza(Recursos *rec) {
    if (!rec->audio_ok) return;
    for (int i = 0; i < SOM_TOTAL; i++)
        if (IsSoundValid(rec->sons[i])) UnloadSound(rec->sons[i]);
    CloseAudioDevice();
}

// ================================================================== tocar
static void toca(const Estado *e, IdSom som, float volume, float tom, float pan) {
    if (!e->rec.audio_ok || som < 0 || som >= SOM_TOTAL) return;
    Sound s = e->rec.sons[som];
    SetSoundVolume(s, fmaxf(0.0f, fminf(1.0f, volume)));
    SetSoundPitch(s, tom);
#if RAYLIB_VERSION_MAJOR < 6
    pan = 0.5f - 0.5f * pan; // até a 5.5 o pan ia de 0 a 1, com 0,5 no centro e 1 na esquerda
#endif
    SetSoundPan(s, pan);
    PlaySound(s);
}

void audio_toca(const Estado *e, IdSom som, float volume, float tom) {
    toca(e, som, volume, tom, 0.0f);
}

// O ouvido é o olho do jogador (no armário, dentro dele); a parede no meio vem do mesmo raycast da visão.
static SomEspacial espacial(const Estado *e, Vector3 fonte, float alcance) {
    Vector3 ouvido = jogador_olho(e);
    Vector3 para = {fonte.x - ouvido.x, 0.0f, fonte.z - ouvido.z};
    float dist = Vector3Length(para);
    bool parede = dist > 0.5f && col_paredes_distancia_raio(&e->paredes, ouvido, para, dist) < dist - 0.3f;
    return regras_som_espacial(ouvido, e->jog.yaw, fonte, alcance, parede);
}

void audio_toca_em(const Estado *e, IdSom som, Vector3 fonte, float alcance, float tom) {
    SomEspacial s = espacial(e, fonte, alcance);
    if (s.volume > 0.0f) toca(e, som, s.volume, tom, s.pan);
}

// Um som que dura enquanto algo acontece (o motor da câmera): começa se não está tocando e, se está, só
// acompanha o volume e o lado (sem recomeçar: um arquivo longo toca inteiro e emenda no fim).
void audio_mantem_em(const Estado *e, IdSom som, Vector3 fonte, float alcance, float tom) {
    if (!e->rec.audio_ok || som < 0 || som >= SOM_TOTAL) return;
    SomEspacial s = espacial(e, fonte, alcance);
    Sound snd = e->rec.sons[som];
    if (!IsSoundPlaying(snd)) {
        if (s.volume > 0.0f) toca(e, som, s.volume, tom, s.pan);
        return;
    }
    float pan = s.pan;
#if RAYLIB_VERSION_MAJOR < 6
    pan = 0.5f - 0.5f * pan; // até a 5.5 o pan ia de 0 a 1, com 0,5 no centro e 1 na esquerda
#endif
    SetSoundVolume(snd, fmaxf(0.0f, fminf(1.0f, s.volume)));
    SetSoundPan(snd, pan);
    SetSoundPitch(snd, tom);
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
    // Na tela inicial (menu e texto datilografado) só toca a trilha (trilhas.c): sem chiado nem zumbido.
    bool tela_inicial = e->fase == FASE_MENU || (e->fase == FASE_INTRO && e->intro.etapa != INTRO_RADIO);
    bool sala = !saida && e->fase != FASE_FIM_RUIM && !tela_inicial;
    bool lampada_acesa = e->apagao_t <= 0.0f && (sala || (saida && e->painel.resolvido));
    mantem_laco(e, SOM_AMBIENTE_SALA, !silencio && sala, 0.35f);
    mantem_laco(e, SOM_AMBIENTE_SAIDA, !silencio && (saida || e->fase == FASE_FIM_RUIM), 0.5f);
    mantem_laco(e, SOM_ZUMBIDO_LAMPADA, !silencio && lampada_acesa, e->falha_luz_t > 0.0f ? 0.3f : 0.12f);
    mantem_laco(e, SOM_ALARME, !silencio && e->alarme_t > 0.0f, 0.35f);
    mantem_laco(e, SOM_RESPIRACAO, !silencio && e->jog.modo == MODO_ARMARIO, 0.55f);
}
