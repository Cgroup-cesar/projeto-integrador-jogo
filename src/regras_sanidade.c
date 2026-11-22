// regras_sanidade.c — a sanidade como funções puras (declaradas em regras.h). Não inclui raylib.h.
// Fica fora de regras.c só para os dois módulos continuarem com ~500 linhas.
#include "regras.h"
#include <math.h>

static float limita(float s) {
    return Clamp(s, 0.0f, SANIDADE_MAX);
}

// 0 na borda do raio, 1 em SANIDADE_PERTO (e mais perto).
static float perto_de(float dist) {
    return Clamp((SANIDADE_RAIO - dist) / (SANIDADE_RAIO - SANIDADE_PERTO), 0.0f, 1.0f);
}

// [REQ:SANIDADE] cai com a criatura perto (já na borda do raio), volta devagar; bônus nas tarefas
float regras_sanidade_passo(float s, float dist, float surgindo, bool escondido, float tempo_longe,
                            bool fase_saida, float dt) {
    if (fase_saida && dist < SANIDADE_RAIO) {
        float p = perto_de(dist);
        float queda = Lerp(SANIDADE_QUEDA_MIN, SANIDADE_QUEDA_MAX, p * p) * Clamp(surgindo, 0.0f, 1.0f);
        if (escondido) queda *= SANIDADE_FATOR_ARMARIO;
        return limita(s - queda * dt);
    }
    if (fase_saida && tempo_longe < SANIDADE_ESPERA_REGEN) return limita(s);
    return limita(s + SANIDADE_REGEN * dt);
}

float regras_sanidade_bonus(float s, EventoFase ev) {
    switch (ev) {
    case EV_INICIAR:
    case EV_VOLTAR_MENU:
        return SANIDADE_MAX;
    case EV_RODADA_CONCLUIDA:
        return limita(s + SANIDADE_BONUS_RODADA);
    case EV_SAIDA_CONCLUIDA:
        return limita(s + SANIDADE_BONUS_SAIDA);
    case EV_PEGO:
    case EV_VIDRO_ROMPEU:
        return limita(fmaxf(s, SANIDADE_PISO_RECOMECO));
    default:
        return limita(s);
    }
}

float regras_sanidade_conectivos(float s, int antes, int agora) {
    if (agora <= antes) return limita(s);
    return limita(s + SANIDADE_BONUS_CONECTIVO * (float)(agora - antes));
}

float regras_sanidade_grao(float s) {
    float t = Clamp((SANIDADE_GRAO_INICIO - s) / SANIDADE_GRAO_INICIO, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t); // smoothstep(INICIO, 0, s)
}

float regras_sanidade_proximidade(float dist) {
    if (dist >= SANIDADE_RAIO) return 0.0f;
    return Lerp(SANIDADE_GRAO_BORDA, 1.0f, perto_de(dist));
}

float regras_sanidade_intensidade(float proximidade, float s) {
    float a = Clamp(proximidade, 0.0f, 1.0f), b = regras_sanidade_grao(s);
    return 1.0f - (1.0f - a) * (1.0f - b);
}
