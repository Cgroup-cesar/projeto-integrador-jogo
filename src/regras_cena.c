// regras_cena.c — a cena do final bom como funções puras (declaradas em regras.h): as etapas, a caminhada
// até o corredor, a volta do helicóptero na base e as luzes. Não inclui raylib.h: cena_final.c aplica.
#include "regras.h"
#include <math.h>

static float suave(float x) {
    x = Clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

static const float FIM_VIRA = CENA_COMECA_S + CENA_VIRA_S;
static const float FIM_PORTA = CENA_COMECA_S + CENA_VIRA_S + CENA_PORTA_S;
static const float FIM_ANDA = CENA_COMECA_S + CENA_VIRA_S + CENA_PORTA_S + CENA_ANDA_S;

// [REQ:PURA-regras_cena_etapa] em que parte da cena o final está
EtapaCena regras_cena_etapa(float t) {
    if (t < CENA_COMECA_S) return CENA_ANTES;
    if (t < FIM_VIRA) return CENA_VIRA;
    if (t < FIM_PORTA) return CENA_PORTA;
    if (t < FIM_ANDA) return CENA_ANDA;
    if (t < CENA_FORA_INICIO_S) return CENA_BRANCO;
    if (t < CENA_FORA_INICIO_S + CENA_FORA_S) return CENA_FORA;
    if (t < FIM_BOM_TEXTO_S) return CENA_PRETO;
    return CENA_FIM;
}

static PoseJogador mistura(PoseJogador a, PoseJogador b, float u_anda, float u_vira) {
    return (PoseJogador){Vector3Lerp(a.pos, b.pos, u_anda), regras_angulo_mistura(a.yaw, b.yaw, u_vira),
                         Lerp(a.pitch, b.pitch, u_vira)};
}

// [REQ:PURA-regras_cena_pose] a caminhada: vira para a porta, espera, anda pelo corredor
PoseJogador regras_cena_pose(float t, PoseJogador de, PoseJogador porta, PoseJogador fim) {
    if (t < CENA_COMECA_S) return de;
    if (t < FIM_VIRA) { // levanta (o olhar sobe do teclado) enquanto vira e dá os passos até a porta
        float u = suave((t - CENA_COMECA_S) / CENA_VIRA_S);
        return mistura(de, porta, u, suave(u * 1.4f));
    }
    if (t < FIM_PORTA) return porta;
    float u = suave((t - FIM_PORTA) / CENA_ANDA_S);
    return mistura(porta, fim, u, u);
}

// [REQ:PURA-regras_cena_fora] lá fora: sai pela porta e anda até o pátio, gira 360° devagar (começa e
// termina macio) e levanta o olhar no fim; o tremor da câmera na mão é uma soma de senos (repetível)
PoseJogador regras_cena_fora(float t, PoseJogador de, Vector3 meio) {
    float anda = suave(t / CENA_SAI_S);
    float giro = suave((t - CENA_SAI_S) / (CENA_FORA_S - CENA_SAI_S - CENA_OLHA_CEU_S));
    float ceu = suave((t - (CENA_FORA_S - CENA_OLHA_CEU_S - 0.6f)) / CENA_OLHA_CEU_S);
    float treme_yaw = 0.008f * sinf(t * 1.9f) + 0.004f * sinf(t * 4.7f + 1.0f);
    float treme_pitch = 0.006f * sinf(t * 2.6f + 2.0f) + 0.003f * sinf(t * 6.3f);
    Vector3 pos = Vector3Lerp(de.pos, meio, anda);
    return (PoseJogador){pos, de.yaw + 2.0f * PI * giro + treme_yaw, Lerp(0.04f, 0.42f, ceu) + treme_pitch};
}

// [REQ:PURA-regras_cena_branco] a lanterna ofusca no último segundo da caminhada, a tela fica branca e a fita
// abre a partir do branco
float regras_cena_branco(float t) {
    if (t < FIM_ANDA - 1.2f) return 0.0f;
    if (t < FIM_ANDA) return 0.6f * suave((t - (FIM_ANDA - 1.2f)) / 1.2f);
    if (t < CENA_FORA_INICIO_S) return 0.6f + 0.4f * suave((t - FIM_ANDA) / CENA_BRANCO_S);
    return 1.0f - suave((t - CENA_FORA_INICIO_S) / 0.7f);
}

float regras_cena_preto(float t) {
    return suave((t - (CENA_FORA_INICIO_S + CENA_FORA_S)) / CENA_PRETO_S);
}

float regras_cena_faixa(float t) {
    if (t < CENA_COMECA_S || t >= CENA_FORA_INICIO_S) return 0.0f;
    return suave((t - CENA_COMECA_S) / CENA_FAIXA_S);
}

// [REQ:PURA-regras_cena_acende] as luminárias do corredor acendem à frente do jogador, uma a uma
float regras_cena_acende(float t, float dist) {
    float frente = (t - FIM_VIRA) * CENA_ACENDE_M_S; // a porta começa a abrir: a primeira acende
    float u = Clamp((frente - dist) / 1.5f, 0.0f, 1.0f);
    if (u <= 0.0f || u >= 1.0f) return u;
    // acendendo: falha algumas vezes (um "sorteio" pelo instante e pela distância, sem rand())
    float s = sinf(floorf(t * 24.0f) * 12.9898f + dist * 78.233f) * 43758.5453f;
    return s - floorf(s) < 0.45f ? 0.15f * u : u;
}

// [REQ:PURA-regras_cena_nave] a luz da nave parada sobre a base; aos NAVE_SOBE_S, sobe acelerando e some
float regras_cena_nave_altura(float t) {
    float subida = fmaxf(0.0f, t - NAVE_SOBE_S);
    return NAVE_ALTURA_M + 0.8f * sinf(t * 1.3f) + 2.5f * subida * subida;
}

float regras_cena_nave_brilho(float t) {
    return 1.0f - suave((t - NAVE_SOBE_S - 2.2f) / 1.5f);
}
