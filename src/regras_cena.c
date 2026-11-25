// regras_cena.c — a cena do final bom como funções puras (declaradas em regras.h): as etapas, a subida pela
// escada até a escotilha, a tampa, o clarão, a volta lá fora na base e a luz da nave. Não inclui raylib.h:
// cena_final.c aplica.
#include "regras.h"
#include <math.h>

static float suave(float x) {
    x = Clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

static const float FIM_VIRA = CENA_COMECA_S + CENA_VIRA_S;
static const float FIM_ABRE = CENA_ABRE_INICIO_S + CENA_ABRE_S;

// [REQ:PURA-regras_cena_etapa] em que parte da cena o final está
EtapaCena regras_cena_etapa(float t) {
    if (t < CENA_COMECA_S) return CENA_ANTES;
    if (t < FIM_VIRA) return CENA_VIRA;
    if (t < CENA_SOBE_INICIO_S) return CENA_ANDA;
    if (t < CENA_ABRE_INICIO_S) return CENA_SOBE;
    if (t < FIM_ABRE) return CENA_ABRE;
    if (t < CENA_FORA_INICIO_S) return CENA_BRANCO;
    if (t < CENA_FORA_INICIO_S + CENA_FORA_S) return CENA_FORA;
    if (t < FIM_BOM_TEXTO_S) return CENA_PRETO;
    return CENA_FIM;
}

static PoseJogador mistura(PoseJogador a, PoseJogador b, float u_anda, float u_vira) {
    return (PoseJogador){Vector3Lerp(a.pos, b.pos, u_anda), regras_angulo_mistura(a.yaw, b.yaw, u_vira),
                         Lerp(a.pitch, b.pitch, u_vira)};
}

// O fim da virada: um passo para trás, saindo do console, já olhando para o caminho até a escada.
static PoseJogador recuo(PoseJogador de, PoseJogador pe) {
    Vector3 ida = Vector3Subtract(pe.pos, de.pos);
    ida.y = 0.0f;
    Vector3 passo = Vector3Scale(Vector3Normalize(ida), CENA_RECUA_M);
    return (PoseJogador){Vector3Add(de.pos, passo), atan2f(ida.x, ida.z), 0.0f};
}

// Na escada: degrau a degrau (sobe rápido e firma o pé), o olhar subindo; empurrando a tampa, mais um pouco.
static PoseJogador subida(float t, PoseJogador pe) {
    PoseJogador p = pe;
    if (t < CENA_ABRE_INICIO_S) {
        float u = Clamp((t - CENA_SOBE_INICIO_S) / CENA_SOBE_S, 0.0f, 1.0f), n = u * CENA_SOBE_DEGRAUS;
        float degrau = floorf(n);
        p.pos.y = (degrau + suave((n - degrau) * 1.6f)) / CENA_SOBE_DEGRAUS * CENA_SOBE_M;
        p.pitch = Lerp(0.0f, 0.5f, suave(u));
        return p;
    }
    float u = (t - CENA_ABRE_INICIO_S) / CENA_ABRE_S, clarao = suave((t - FIM_ABRE) / CENA_BRANCO_S);
    p.pos.y = CENA_SOBE_M + CENA_EMPURRA_M * suave(u * 2.0f) + CENA_CLARAO_SOBE_M * clarao;
    p.pitch = Lerp(0.5f, CENA_OLHA_CIMA_PITCH, suave(u * 3.0f));
    return p;
}

// [REQ:PURA-regras_cena_pose] a subida: recua do console e vira, anda até a escada e sobe até a escotilha
PoseJogador regras_cena_pose(float t, PoseJogador de, PoseJogador pe) {
    if (t < CENA_COMECA_S) return de;
    PoseJogador volta = recuo(de, pe);
    if (t < FIM_VIRA) { // levanta (o olhar sobe do teclado) enquanto recua e vira
        float u = suave((t - CENA_COMECA_S) / CENA_VIRA_S);
        return mistura(de, volta, u, suave(u * 1.4f));
    }
    if (t < CENA_SOBE_INICIO_S) { // anda e, no fim, vira de frente para os degraus
        float u = (t - FIM_VIRA) / CENA_ANDA_S;
        PoseJogador p = mistura(volta, pe, suave(u), 0.0f);
        p.yaw = regras_angulo_mistura(volta.yaw, pe.yaw, suave((u - 0.6f) / 0.4f));
        return p;
    }
    return subida(t, pe);
}

// [REQ:PURA-regras_cena_escotilha] a tampa: ele empurra e ela emperra (um tranco que cede e volta), depois
// abre devagar até o fim do empurrão
float regras_cena_escotilha(float t) {
    float tranco = CENA_ABRE_INICIO_S + 0.15f, solta = tranco + CENA_EMPERRA_S;
    if (t < tranco) return 0.0f;
    if (t < solta) return 0.04f * sinf(PI * (t - tranco) / CENA_EMPERRA_S);
    return suave((t - solta) / (FIM_ABRE - solta));
}

int regras_cena_degrau(float t) {
    if (t < CENA_SOBE_INICIO_S || t >= CENA_ABRE_INICIO_S) return -1;
    float u = (t - CENA_SOBE_INICIO_S) / CENA_SOBE_S;
    return (int)fminf(u * CENA_SOBE_DEGRAUS, CENA_SOBE_DEGRAUS - 1);
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

// [REQ:PURA-regras_cena_branco] a luz entra com a tampa abrindo, estoura a tela em branco e a fita abre a
// partir do branco
float regras_cena_branco(float t) {
    if (t < CENA_ABRE_INICIO_S) return 0.0f;
    if (t < FIM_ABRE) {
        float a = regras_cena_escotilha(t);
        return 0.5f * a * a;
    }
    if (t < CENA_FORA_INICIO_S) return 0.5f + 0.5f * suave((t - FIM_ABRE) / CENA_BRANCO_S);
    return 1.0f - suave((t - CENA_FORA_INICIO_S) / 0.7f);
}

float regras_cena_preto(float t) {
    return suave((t - (CENA_FORA_INICIO_S + CENA_FORA_S)) / CENA_PRETO_S);
}

float regras_cena_faixa(float t) {
    if (t < CENA_COMECA_S || t >= CENA_FORA_INICIO_S) return 0.0f;
    return suave((t - CENA_COMECA_S) / CENA_FAIXA_S);
}

// [REQ:PURA-regras_cena_nave] a luz da nave parada sobre a base; aos NAVE_SOBE_S, sobe acelerando e some
float regras_cena_nave_altura(float t) {
    float subida = fmaxf(0.0f, t - NAVE_SOBE_S);
    return NAVE_ALTURA_M + 0.8f * sinf(t * 1.3f) + 2.5f * subida * subida;
}

float regras_cena_nave_brilho(float t) {
    return 1.0f - suave((t - NAVE_SOBE_S - 2.2f) / 1.5f);
}
