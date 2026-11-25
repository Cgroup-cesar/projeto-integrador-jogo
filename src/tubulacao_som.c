// tubulacao_som.c — os sons da tubulação. A tampa vem de um arquivo longo com vários golpes
// (som-tampa-tubulacao.mp3): dois trechos são cortados ao carregar, com rampas nas pontas para o corte não
// estalar. Os passos (passos-tubulacao.wav) são quatro baques a cada 0,61 s e emendam sem corte: tocam em
// laço enquanto ele atravessa por baixo, e a fonte anda em linha reta da boca da cela até o duto do corredor.
#include "tubulacao_som.h"
#include "audio_sintese.h"
#include "ui.h"
#include <math.h>

#define FADE_ENTRA_S  0.015f // as rampas do corte da tampa
#define FADE_SAI_S    0.2f
#define FADE_PASSOS_S 0.04f // só para o começo e o fim dos passos não estalarem

// ================================================================== carregar
static int limita(int v, int de, int ate) {
    return v < de ? de : v > ate ? ate : v;
}

// O trecho [de, ate] (segundos) de uma onda mono de 16 bits, com rampa de entrada e de saída.
static Sound trecho(Wave origem, float de, float ate) {
    Wave w = WaveCopy(origem);
    int total = (int)w.frameCount;
    int a = limita((int)(de * w.sampleRate), 0, total - 1);
    WaveCrop(&w, a, limita((int)(ate * w.sampleRate), a + 1, total));
    short *amostra = (short *)w.data;
    int n = (int)w.frameCount;
    float entra = FADE_ENTRA_S * w.sampleRate, sai = FADE_SAI_S * w.sampleRate;
    for (int i = 0; i < n; i++) // padrão MAP: o ganho de cada amostra (as rampas nas pontas)
        amostra[i] = (short)(amostra[i] * fminf(1.0f, fminf(i / entra, (n - 1 - i) / sai)));
    Sound som = LoadSoundFromWave(w);
    UnloadWave(w);
    return som;
}

static void carrega_tampa(Recursos *rec) {
    char caminho[512];
    if (!ui_caminho_asset("audio/som-tampa-tubulacao.mp3", caminho, sizeof(caminho))) return;
    Wave w = LoadWave(caminho);
    if (!IsWaveValid(w)) return;
    WaveFormat(&w, (int)w.sampleRate, 16, 1); // mono: o lado (pan) é do jogo
    rec->tampa_sai = trecho(w, TAMPA_SAI_DE_S, TAMPA_SAI_ATE_S);
    rec->tampa_entra = trecho(w, TAMPA_ENTRA_DE_S, TAMPA_ENTRA_ATE_S);
    rec->tem_tampa = IsSoundValid(rec->tampa_sai) && IsSoundValid(rec->tampa_entra);
    UnloadWave(w);
}

// [REQ:RNF4] asset ausente: o som fica mudo e só fica um TraceLog
void tubulacao_som_carrega(Recursos *rec) {
    rec->tem_tampa = rec->tem_passos_tubo = false;
    if (!rec->audio_ok) return;
    carrega_tampa(rec);
    if (!rec->tem_tampa)
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/audio/som-tampa-tubulacao.mp3; a tampa fica muda");
    char caminho[512];
    if (ui_caminho_asset("audio/passos-tubulacao.wav", caminho, sizeof(caminho))) {
        rec->passos_tubo = LoadMusicStream(caminho);
        rec->tem_passos_tubo = IsMusicValid(rec->passos_tubo);
        rec->passos_tubo.looping = true;
    }
    if (!rec->tem_passos_tubo)
        TraceLog(LOG_INFO,
                 "PRECURSOR: sem assets/audio/passos-tubulacao.wav; os passos na tubulação ficam mudos");
}

void tubulacao_som_descarrega(Recursos *rec) {
    if (rec->tem_tampa) {
        UnloadSound(rec->tampa_sai);
        UnloadSound(rec->tampa_entra);
    }
    if (rec->tem_passos_tubo) UnloadMusicStream(rec->passos_tubo);
    rec->tem_tampa = rec->tem_passos_tubo = false;
}

// ================================================================== a tampa
// O volume espacial com um piso: a distância e a parede ainda pesam, mas o som nunca some (nem por baixo
// da voz do rádio, que toca ao mesmo tempo, sem abaixar nada).
static float com_piso(float volume, float piso) {
    return piso + (1.0f - piso) * volume;
}

static void toca_em(const Estado *e, Sound som, Vector3 fonte) {
    SomEspacial s = audio_espacial(e, fonte, TAMPA_ALCANCE_M);
    SetSoundVolume(som, fminf(1.0f, com_piso(s.volume, TAMPA_PISO) * TAMPA_VOLUME));
    SetSoundPan(som, audio_pan(s.pan));
    PlaySound(som);
}

// Entrar: no começo da descida na cela. Sair: no começo da saída do duto do corredor (o baque cai quando as
// mãos agarram a borda). Cada um, uma vez: na virada do relógio.
static void toca_tampas(const Estado *e) {
    float entra = ANIM_CANO_DESCE_EM;
    if (e->cano && e->tubo_ant[0] < entra && e->cano_t >= entra)
        toca_em(e, e->rec.tampa_entra, e->nivel.cano_cela);
    bool esperando = regras_fase_eh_saida(e->fase) && !e->cri.ativo;
    float surge = ANIM_BUEIRO_PRE_S, agora = e->cri.tempo_para_surgir;
    if (esperando && e->tubo_ant[1] > surge && agora <= surge) toca_em(e, e->rec.tampa_sai, e->nivel.duto);
}

// ================================================================== os passos na tubulação
// Quanto da travessia ele já fez (0..1), ou −1 fora dela, e o ganho das rampas. Começa no quadro em que a
// entrada na cela termina e acaba no quadro em que a saída do duto começa (aí a tampa toma o lugar dos
// passos). Uma saída recomeçada (pego) não tem travessia: ele não passou de novo pela cela.
static float travessia(const Estado *e, float *ganho) {
    if (!e->cano || !regras_fase_eh_saida(e->fase) || e->cri.ativo || e->pegos_na_fase > 0) return -1.0f;
    float feito = e->cano_t - ANIM_CANO_FIM;
    float falta = e->cri.tempo_para_surgir - ANIM_BUEIRO_PRE_S;
    if (feito < 0.0f || falta <= 0.0f) return -1.0f;
    *ganho = fminf(1.0f, fminf(feito, falta) / FADE_PASSOS_S);
    return feito / (feito + falta);
}

static void passos(const Estado *e) {
    if (!e->rec.tem_passos_tubo) return;
    Music m = e->rec.passos_tubo;
    float ganho = 0.0f, p = travessia(e, &ganho);
    if (p < 0.0f || e->pausado || e->silencio_t > 0.0f) {
        if (IsMusicStreamPlaying(m)) StopMusicStream(m);
        return;
    }
    Vector3 fonte = Vector3Lerp(e->nivel.cano_cela, e->nivel.duto, p);
    fonte.y = -PASSOS_TUBO_FUNDO_M; // por baixo do chão
    SomEspacial s = audio_espacial(e, fonte, PASSOS_TUBO_ALCANCE_M);
    if (!IsMusicStreamPlaying(m)) PlayMusicStream(m);
    SetMusicVolume(m, com_piso(s.volume, PASSOS_TUBO_PISO) * PASSOS_TUBO_VOLUME * ganho);
    SetMusicPan(m, audio_pan(s.pan));
    UpdateMusicStream(m);
}

void tubulacao_som_atualiza(Estado *e, float dt) {
    (void)dt; // os relógios são os do jogo (cano_t, tempo_para_surgir): na pausa, nada vira
    if (!e->rec.audio_ok) return;
    if (e->rec.tem_tampa && e->silencio_t <= 0.0f) toca_tampas(e);
    passos(e);
    e->tubo_ant[0] = e->cano ? e->cano_t : -1.0f;
    e->tubo_ant[1] = e->cri.tempo_para_surgir;
}
