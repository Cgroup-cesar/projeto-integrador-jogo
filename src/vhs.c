// vhs.c — filtro VHS (GLSL embutido, versões 330 e 100) e os presets escolhíveis com F1.
#include "vhs.h"
#include "ui.h"
#include "rlgl.h"
#include <stdio.h>

// A ordem é a das colunas de VHS_PRESETS (config.h) e a dos uniformes depois de tempo e resolução.
typedef enum { VHS_RUIDO, VHS_ABERRACAO, VHS_SCANLINES, VHS_VINHETA, VHS_WOBBLE } ParametroVhs;

static const float PRESETS[][VHS_PARAMETROS] = VHS_PRESETS;
#define N_PRESETS ((int)(sizeof(PRESETS) / sizeof(PRESETS[0])))

static const TextoId NOMES[] = {TXT_VHS_DESLIGADO, TXT_VHS_ORIGINAL};
// C99 não tem _Static_assert: um vetor de tamanho negativo não compila se faltar (ou sobrar) nome.
typedef char cada_preset_tem_um_nome[(sizeof(NOMES) / sizeof(NOMES[0]) == N_PRESETS) ? 1 : -1];

static const char *const UNIFORMES[VHS_UNIFORMES] = {"tempo",     "resolucao", "ruido", "aberracao",
                                                     "scanlines", "vinheta",   "wobble"};

#define VHS_ROTULO_S 2.5f

// ------------------------------------------------------------------ shaders
// Ruído, aberração cromática, scanlines, vinheta e um "wobble" horizontal raro.
#define VHS_UNIFORMES_GLSL                                                                                   \
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"                                                 \
    "uniform float tempo; uniform vec2 resolucao;\n"                                                         \
    "uniform float ruido; uniform float aberracao; uniform float scanlines; uniform float vinheta;\n"        \
    "uniform float wobble;\n"                                                                                \
    "float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }\n"

// O corpo é o mesmo nas duas versões: AMOSTRA e SAIDA trocam texture/texture2D e finalColor/gl_FragColor.
#define VHS_CORPO_GLSL                                                                                       \
    "void main() {\n"                                                                                        \
    "    vec2 uv = fragTexCoord;\n"                                                                          \
    "    float janela = step(0.97, hash(vec2(floor(tempo * 3.0), 7.0)));\n"                                  \
    "    float faixa = smoothstep(0.0, 0.08, 0.08 - abs(uv.y - fract(tempo * 0.37)));\n"                     \
    "    uv.x += janela * faixa * wobble / resolucao.x;\n"                                                   \
    "    float px = aberracao / resolucao.x;\n"                                                              \
    "    vec3 c = vec3(AMOSTRA(texture0, uv + vec2(px, 0.0)).r, AMOSTRA(texture0, uv).g,\n"                  \
    "                  AMOSTRA(texture0, uv - vec2(px, 0.0)).b);\n"                                          \
    "    c += (hash(uv * resolucao + vec2(tempo * 61.0, tempo * 17.0)) - 0.5) * ruido;\n"                    \
    "    c *= 1.0 - scanlines * (0.5 + 0.5 * sin(uv.y * resolucao.y * 3.14159));\n"                          \
    "    vec2 d = uv - 0.5;\n"                                                                               \
    "    c *= 1.0 - vinheta * dot(d, d) * 2.0;\n"                                                            \
    "    SAIDA = vec4(c, 1.0) * colDiffuse * fragColor;\n"                                                   \
    "}\n"

static const char *VHS_FS_330 = "#version 330\n"
                                "#define AMOSTRA texture\n"
                                "#define SAIDA finalColor\n"
                                "in vec2 fragTexCoord; in vec4 fragColor;\n"
                                "out vec4 finalColor;\n" VHS_UNIFORMES_GLSL VHS_CORPO_GLSL;

static const char *VHS_FS_100 = "#version 100\n"
                                "precision mediump float;\n"
                                "#define AMOSTRA texture2D\n"
                                "#define SAIDA gl_FragColor\n"
                                "varying vec2 fragTexCoord; varying vec4 fragColor;\n" VHS_UNIFORMES_GLSL
                                    VHS_CORPO_GLSL;

// ------------------------------------------------------------------ carga
void vhs_carrega(Recursos *rec) {
    bool v100 = rlGetVersion() == RL_OPENGL_ES_20;
    rec->vhs = LoadShaderFromMemory(NULL, v100 ? VHS_FS_100 : VHS_FS_330);
    rec->vhs_ok = IsShaderValid(rec->vhs);
    if (!rec->vhs_ok) {
        TraceLog(LOG_WARNING, "PRECURSOR: shader VHS não compilou; seguindo sem filtro");
        return;
    }
    for (int i = 0; i < VHS_UNIFORMES; i++) // padrão MAP: nome do uniforme → posição no shader
        rec->loc_vhs[i] = GetShaderLocation(rec->vhs, UNIFORMES[i]);
}

void vhs_descarrega(Recursos *rec) {
    if (rec->vhs_ok) UnloadShader(rec->vhs);
}

// ------------------------------------------------------------------ presets
int vhs_total(void) {
    return N_PRESETS;
}

const char *vhs_nome(int preset) {
    if (preset < 0 || preset >= N_PRESETS) return "";
    return dialogo_texto(NOMES[preset]);
}

void vhs_escolhe(Estado *e, int preset) {
    e->vhs_preset = ((preset % N_PRESETS) + N_PRESETS) % N_PRESETS;
    e->vhs_rotulo_t = VHS_ROTULO_S;
}

void vhs_atualiza(Estado *e, float dt) {
    e->vhs_rotulo_t = e->vhs_rotulo_t > dt ? e->vhs_rotulo_t - dt : 0.0f;
#if TECLAS_DEBUG
    if (e->in.f1 && !e->modo_feira) vhs_escolhe(e, e->vhs_preset + 1);
#endif
}

// ------------------------------------------------------------------ desenho
bool vhs_comeca(const Estado *e) {
    const Recursos *r = &e->rec;
    if (!r->vhs_ok || e->vhs_preset <= 0 || e->vhs_preset >= N_PRESETS) return false;
    float tempo = e->tempo_total;
    float resolucao[2] = {(float)r->alvo_largura, (float)r->alvo_altura};
    SetShaderValue(r->vhs, r->loc_vhs[0], &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->vhs, r->loc_vhs[1], resolucao, SHADER_UNIFORM_VEC2);
    const float *p = PRESETS[e->vhs_preset];
    for (int i = 0; i < VHS_PARAMETROS; i++) // padrão MAP: coluna do preset → uniforme do shader
        SetShaderValue(r->vhs, r->loc_vhs[i + 2], &p[i], SHADER_UNIFORM_FLOAT);
    BeginShaderMode(r->vhs);
    return true;
}

void vhs_termina(const Estado *e) {
    (void)e;
    EndShaderMode();
}

void vhs_desenha(const Estado *e) {
    if (e->vhs_rotulo_t <= 0.0f) return;
    char texto[96];
    snprintf(texto, sizeof(texto), dialogo_texto(TXT_VHS_ROTULO), e->vhs_preset, N_PRESETS - 1,
             vhs_nome(e->vhs_preset));
    float s = ui_escala(), tamanho = 18.0f * s;
    float alfa = e->vhs_rotulo_t < 0.5f ? e->vhs_rotulo_t / 0.5f : 1.0f; // some devagar no fim
    Vector2 m = ui_mede(e, texto, tamanho);
    Rectangle caixa = {GetScreenWidth() - m.x - 28 * s, 16 * s, m.x + 16 * s, m.y + 10 * s};
    DrawRectangleRec(caixa, Fade(BLACK, 0.6f * alfa));
    ui_texto(e, texto, (Vector2){caixa.x + 8 * s, caixa.y + 5 * s}, tamanho, Fade(RAYWHITE, alfa));
}
