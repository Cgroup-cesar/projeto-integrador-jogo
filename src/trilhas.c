// trilhas.c — os sons longos de assets/audio (Music, por streaming). Quem decide o volume de cada um é a
// função pura regras_volumes_trilhas; aqui só se lê a situação do quadro, faz o fade e começa, pausa ou
// para o streaming. Tudo sincronizado com o que aparece na tela:
//   voz do Precursor: só enquanto ele "diz" uma frase (os mesmos quadros em que os símbolos aparecem);
//   voz do rádio: enquanto a legenda da frente for do Varela; cada fala nova começa noutro trecho;
//   coração: pela distância do Precursor solto; passos: andando ou correndo.
#include "trilhas.h"
#include "ui.h"
#include <math.h>

static const char *const ARQUIVOS[TRILHA_TOTAL] = {
    [TRILHA_MENU] = "audio/trilha_menu.wav",
    [TRILHA_VOZ_PRECURSOR] = "audio/voz_precursor_fala.wav",
    [TRILHA_VOZ_RADIO] = "audio/voz_radio.wav",
    [TRILHA_CORACAO_NORMAL] = "audio/coracao_normal.wav",
    [TRILHA_CORACAO_RAPIDO] = "audio/coracao_rapido.wav",
    [TRILHA_PASSO_ANDAR] = "audio/passo_andar.mp3",
    [TRILHA_PASSO_CORRER] = "audio/passo_correr.mp3",
};

// ================================================================== carregar
// [REQ:RNF4] asset ausente: a trilha fica muda e só fica um TraceLog
void trilhas_carrega(Recursos *rec) {
    for (int i = 0; i < TRILHA_TOTAL; i++) {
        char caminho[512];
        rec->tem_trilha[i] = false;
        if (!rec->audio_ok) continue;
        if (!ui_caminho_asset(ARQUIVOS[i], caminho, sizeof(caminho))) {
            TraceLog(LOG_INFO, "PRECURSOR: sem assets/%s; essa trilha fica muda", ARQUIVOS[i]);
            continue;
        }
        rec->trilhas[i] = LoadMusicStream(caminho);
        rec->tem_trilha[i] = IsMusicValid(rec->trilhas[i]);
        if (rec->tem_trilha[i])
            rec->trilhas[i].looping = true;
        else
            TraceLog(LOG_WARNING, "PRECURSOR: não deu para abrir %s", caminho);
    }
}

void trilhas_descarrega(Recursos *rec) {
    for (int i = 0; i < TRILHA_TOTAL; i++)
        if (rec->tem_trilha[i]) UnloadMusicStream(rec->trilhas[i]);
}

bool trilhas_tem(const Estado *e, IdTrilha trilha) {
    return trilha >= 0 && trilha < TRILHA_TOTAL && e->rec.tem_trilha[trilha];
}

// ================================================================== a situação do quadro
// Na resposta final ele fica com a mão no vidro (fala_t ligado o tempo todo), mas só "fala" ao receber a
// transmissão; nas rodadas, fala enquanto os símbolos aparecem.
static bool precursor_falando(const Estado *e) {
    if (e->fase == FASE_RESPOSTA_FINAL) return e->comp.transmitindo;
    return regras_fase_eh_rodada(e->fase) && e->cri.fala_t > 0.0f;
}

static bool andando(const Estado *e) {
    bool jogando = e->fase != FASE_MENU && e->fase != FASE_INTRO && e->fase != FASE_FIM_BOM &&
                   e->fase != FASE_FIM_RUIM && !e->pausado;
    bool mexendo = e->in.mover.x != 0.0f || e->in.mover.y != 0.0f;
    return jogando && e->jog.modo == MODO_ANDANDO && mexendo;
}

static SituacaoSom situacao(const Estado *e) {
    const Criatura *c = &e->cri;
    SituacaoSom s;
    s.tela_inicial = e->fase == FASE_MENU || (e->fase == FASE_INTRO && !e->intro.interfone);
    s.precursor_falando = precursor_falando(e);
    s.radio_falando = e->legendas.n > 0 && e->legendas.fila[0].falante == FALANTE_VARELA;
    s.criatura_solta = regras_fase_eh_saida(e->fase) && c->ativo && c->distorcao <= 0.0f;
    s.distancia_criatura = Vector3Distance(c->pos, e->jog.pos);
    s.andando = andando(e);
    s.correndo = s.andando && e->jog.correndo;
    s.silencio = e->silencio_t > 0.0f || e->jumpscare_t > 0.0f ||
                 (e->fase == FASE_FIM_BOM && e->fim.tempo > 2.8f && e->fim.tempo < 4.8f);
    return s;
}

// ================================================================== tocar
// Vozes começam de um trecho sorteado: cada frase soa diferente, sem repetir o começo da gravação.
static void comeca(Estado *e, IdTrilha t) {
    Music m = e->rec.trilhas[t];
    PlayMusicStream(m);
    bool voz = t == TRILHA_VOZ_PRECURSOR || t == TRILHA_VOZ_RADIO;
    float sobra = GetMusicTimeLength(m) - 2.0f;
    if (voz && sobra > 0.0f) SeekMusicStream(m, sobra * GetRandomValue(0, 1000) / 1000.0f);
}

// Passos e coração pausam (e continuam do mesmo ponto); a trilha e as vozes param (e recomeçam do zero
// ou de outro trecho).
static void silencia(Estado *e, IdTrilha t) {
    Music m = e->rec.trilhas[t];
    if (!IsMusicStreamPlaying(m)) return;
    bool continua = t == TRILHA_PASSO_ANDAR || t == TRILHA_PASSO_CORRER || t == TRILHA_CORACAO_NORMAL ||
                    t == TRILHA_CORACAO_RAPIDO;
    if (continua)
        PauseMusicStream(m);
    else
        StopMusicStream(m);
}

static void aplica(Estado *e, IdTrilha t, float alvo, bool corte_seco, float dt) {
    float *vol = &e->mix.volume[t];
    float passo = dt / (t == TRILHA_MENU ? TRILHA_MENU_FADE_S : TRILHA_FADE_S);
    *vol = corte_seco ? alvo : *vol + fmaxf(-passo, fminf(passo, alvo - *vol));
    Music m = e->rec.trilhas[t];
    if (*vol <= 0.001f) {
        silencia(e, t);
        return;
    }
    if (!IsMusicStreamPlaying(m)) {
        if (t == TRILHA_PASSO_ANDAR || t == TRILHA_PASSO_CORRER || t == TRILHA_CORACAO_NORMAL ||
            t == TRILHA_CORACAO_RAPIDO)
            ResumeMusicStream(m); // pausada: continua de onde parou (sem pausa antes, começa do zero)
        if (!IsMusicStreamPlaying(m)) comeca(e, t);
    }
    SetMusicVolume(m, *vol);
    UpdateMusicStream(m);
}

// Fala nova do Varela com o rádio já tocando: pula para outro trecho, para não soar como a mesma frase.
static void acompanha_radio(Estado *e) {
    const char *fala = e->legendas.n > 0 ? e->legendas.fila[0].texto : NULL;
    if (fala == e->mix.fala_no_radio) return;
    e->mix.fala_no_radio = fala;
    Music m = e->rec.trilhas[TRILHA_VOZ_RADIO];
    if (fala && IsMusicStreamPlaying(m)) comeca(e, TRILHA_VOZ_RADIO);
}

void trilhas_atualiza(Estado *e, float dt) {
    if (!e->rec.audio_ok) return;
    SituacaoSom s = situacao(e);
    float alvo[TRILHA_TOTAL];
    regras_volumes_trilhas(s, alvo);
    if (e->rec.tem_trilha[TRILHA_VOZ_RADIO]) acompanha_radio(e);
    for (int t = 0; t < TRILHA_TOTAL; t++) // padrão MAP: cada trilha segue o seu volume alvo
        if (e->rec.tem_trilha[t]) aplica(e, (IdTrilha)t, alvo[t], s.silencio, dt);
}
