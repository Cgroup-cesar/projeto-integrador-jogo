// regras_porta.c — a porta da sala de observação e a volta à sala como funções puras (declaradas em
// regras.h). Não inclui raylib.h: porta_sala.c e retorno.c aplicam o que sai daqui.
#include "regras.h"
#include <math.h>

static float suave(float x) {
    x = Clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

// [REQ:PURA-regras_porta_passo] a folha anda para onde `aberta` manda, sem passar de 0 nem de 1
float regras_porta_passo(float abertura, bool aberta, float dt, float duracao) {
    float passo = dt / fmaxf(duracao, 0.001f);
    return Clamp(abertura + (aberta ? passo : -passo), 0.0f, 1.0f);
}

// [REQ:PURA-regras_porta_solida] quase fechada, a folha já segura quem tenta passar
bool regras_porta_solida(float abertura) {
    return abertura < PORTA_SOLIDA_ATE;
}

float regras_angulo_mistura(float de, float para, float u) {
    float diferenca = atan2f(sinf(para - de), cosf(para - de)); // −π..π: o lado mais curto
    return de + diferenca * u;
}

EtapaRetorno regras_retorno_etapa(float t) {
    if (t < RETORNO_VIRA_S) return RETORNO_VIRA;
    if (t < RETORNO_VIRA_S + RETORNO_FECHA_S) return RETORNO_FECHA;
    if (t < RETORNO_VIRA_S + RETORNO_FECHA_S + RETORNO_ENTRA_S) return RETORNO_ENTRA;
    return RETORNO_FIM;
}

static PoseJogador mistura(PoseJogador a, PoseJogador b, float u_anda, float u_vira) {
    return (PoseJogador){Vector3Lerp(a.pos, b.pos, u_anda), regras_angulo_mistura(a.yaw, b.yaw, u_vira),
                         Lerp(a.pitch, b.pitch, u_vira)};
}

// [REQ:PURA-regras_retorno_pose] a coreografia da volta à sala: vira para a porta enquanto dá um passo,
// fica parado puxando a folha (o olhar desce um pouco e volta) e entra na sala virando antes de andar
PoseJogador regras_retorno_pose(float t, PoseJogador de, PoseJogador porta, PoseJogador sala) {
    if (t < RETORNO_VIRA_S) {
        float u = suave(t / RETORNO_VIRA_S);
        return mistura(de, porta, u, u);
    }
    float fecha = t - RETORNO_VIRA_S;
    if (fecha < RETORNO_FECHA_S) {
        PoseJogador p = porta;
        p.pitch -= RETORNO_PUXA_PITCH * sinf(PI * fminf(1.0f, fecha / PORTA_FECHA_S));
        return p;
    }
    float u = (fecha - RETORNO_FECHA_S) / RETORNO_ENTRA_S;
    return mistura(porta, sala, suave((u - 0.15f) / 0.85f), suave(u * 1.8f));
}

// [REQ:PURA-regras_retorno_faixa] as faixas entram em RETORNO_FAIXA_S e saem nos últimos RETORNO_FAIXA_S
float regras_retorno_faixa(float t) {
    float total = RETORNO_VIRA_S + RETORNO_FECHA_S + RETORNO_ENTRA_S;
    return suave(fminf(t, total - t) / RETORNO_FAIXA_S);
}

// [REQ:PURA-regras_retorno_luz] a luz da sala assenta: 0 = ainda a luz da saída (luminária fria e a luz
// fraca no jogador), 1 = a da rodada (a lâmpada âmbar). Passa de uma à outra enquanto ele entra na sala.
float regras_retorno_luz(float t) {
    return suave((t - RETORNO_VIRA_S - RETORNO_FECHA_S) / RETORNO_ENTRA_S);
}
