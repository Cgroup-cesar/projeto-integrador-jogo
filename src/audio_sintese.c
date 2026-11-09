// audio_sintese.c — sons gerados em código. (M2: só a interface; a síntese chega no M7.)
#include "audio_sintese.h"

void audio_inicia(Recursos *rec) {
    rec->audio_ok = false;
}

void audio_finaliza(Recursos *rec) {
    (void)rec;
}

void audio_toca(const Estado *e, IdSom som, float volume, float tom) {
    (void)e;
    (void)som;
    (void)volume;
    (void)tom;
}

void audio_para(const Estado *e, IdSom som) {
    (void)e;
    (void)som;
}

void audio_atualiza(Estado *e, float dt) {
    (void)e;
    (void)dt;
}
