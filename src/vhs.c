// vhs.c — filtro VHS (GLSL embutido, versões 330 e 100) e os presets escolhíveis com F1.
#include "vhs.h"
#include "ui.h"
#include "rlgl.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

// A ordem é a das colunas de VHS_PRESETS (config.h) e a dos uniformes depois de tempo e resolução.
typedef enum {
    VHS_RUIDO,
    VHS_ABERRACAO,
    VHS_SCANLINES,
    VHS_VINHETA,
    VHS_WOBBLE,
    VHS_SANGRAMENTO,
    VHS_TRACKING,
    VHS_SATURACAO,
    VHS_TOM,
    VHS_CURVATURA,
    VHS_CINTILACAO,
    VHS_ROLAGEM,
    VHS_BRILHO
} ParametroVhs;

static const float PRESETS[][VHS_PARAMETROS] = VHS_PRESETS;
#define N_PRESETS ((int)(sizeof(PRESETS) / sizeof(PRESETS[0])))

static const TextoId NOMES[] = {TXT_VHS_DESLIGADO,  TXT_VHS_ORIGINAL,    TXT_VHS_FITA_CASEIRA,
                                TXT_VHS_FITA_GASTA, TXT_VHS_MONITOR_CRT, TXT_VHS_VIGILANCIA};
// C99 não tem _Static_assert: um vetor de tamanho negativo não compila se faltar (ou sobrar) nome.
typedef char cada_preset_tem_um_nome[(sizeof(NOMES) / sizeof(NOMES[0]) == N_PRESETS) ? 1 : -1];

static const char *const UNIFORMES[VHS_UNIFORMES] = {
    "tempo",    "resolucao", "ruido", "aberracao", "scanlines",  "vinheta", "wobble", "sangramento",
    "tracking", "saturacao", "tom",   "curvatura", "cintilacao", "rolagem", "brilho", "insanidade"};

#define VHS_ROTULO_S 2.5f

// ------------------------------------------------------------------ shaders
// Pensado para ser sentido, não visto: a curvatura só arredonda os cantos, a faixa de tracking fica no
// rodapé (fora da área de jogo) e o ruído quase não clareia o preto (o escuro continua escuro).
// hash sem seno (Dave Hoskins): o antigo fract(sin(...)) fazia padrões diagonais com coordenadas grandes, e
// em mediump pior ainda. As entradas ficam pequenas: o tempo entra como número de quadro com mod (quadro()).
#define VHS_UNIFORMES_GLSL                                                                                   \
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"                                                 \
    "uniform float tempo; uniform vec2 resolucao;\n"                                                         \
    "uniform float ruido; uniform float aberracao; uniform float scanlines; uniform float vinheta;\n"        \
    "uniform float wobble; uniform float sangramento; uniform float tracking; uniform float saturacao;\n"    \
    "uniform float tom; uniform float curvatura; uniform float cintilacao;\n"                                \
    "uniform float rolagem; uniform float brilho; uniform float insanidade;\n"                               \
    "float hash(vec2 p) {\n"                                                                                 \
    "    vec3 p3 = fract(vec3(p.xyx) * 0.1031);\n"                                                           \
    "    p3 += dot(p3, p3.yzx + 33.33);\n"                                                                   \
    "    return fract((p3.x + p3.y) * p3.z);\n"                                                              \
    "}\n"                                                                                                    \
    "float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }\n"                                     \
    "float quadro(float hz, float ciclo) { return mod(floor(tempo * hz), ciclo); }\n"                        \
    "float sente() { return pow(clamp(insanidade, 0.0, 1.0), SAN_CURVA); }\n"

// Curvatura de tubo: o centro fica parado e os cantos saem da imagem (viram preto, arredondados).
#define VHS_CURVA_GLSL                                                                                       \
    "vec2 curva(vec2 uv) {\n"                                                                                \
    "    vec2 d = uv - 0.5;\n"                                                                               \
    "    return 0.5 + d * (1.0 + curvatura * dot(d, d));\n"                                                  \
    "}\n"

// Sangramento: a cor (não o brilho) é borrada na horizontal, como na fita; o contorno continua nítido.
#define VHS_SANGRA_GLSL                                                                                      \
    "vec3 sangra(vec3 c, vec2 uv) {\n"                                                                       \
    "    float px = sangramento / resolucao.x;\n"                                                            \
    "    vec3 b = vec3(0.0);\n"                                                                              \
    "    for (int i = -2; i <= 2; i++) b += AMOSTRA(texture0, uv + vec2(float(i) * px, 0.0)).rgb;\n"         \
    "    b *= 0.2;\n"                                                                                        \
    "    return mix(c, vec3(luma(c)) + b - vec3(luma(b)), step(0.01, sangramento));\n"                       \
    "}\n"

// Tom: positivo = quente (fita caseira), negativo = esverdeado (monitor de fósforo).
#define VHS_TOM_GLSL                                                                                         \
    "vec3 tinge(vec3 c) {\n"                                                                                 \
    "    vec3 t = tom > 0.0 ? vec3(1.08, 1.0, 0.84) : vec3(0.82, 1.06, 0.88);\n"                             \
    "    return mix(c, c * t, abs(tom));\n"                                                                  \
    "}\n"

// Sanidade, camada 1 — cansaço (o olho embaçando): um desfoque de 8 amostras que vem em ondas lentas (duas
// senoides de períodos SAN_PULSO_A e SAN_PULSO_B somadas, para não ficar regular), mais forte nas bordas que
// no centro (a mira fica nítida), a vinheta fechando em túnel junto com a onda e a cor perdendo saturação.
// Tudo escala com `insanidade` (0..1 = 1 − sanidade/100, ver regras_sanidade_intensidade) passada por
// sente() = insanidade^SAN_CURVA: o ruído se soma ao que a cena já tem e, linear, sumia até ~50%.
#define VHS_CANSACO_GLSL                                                                                     \
    "float pulso() {\n"                                                                                      \
    "    float t = mod(tempo, 600.0);\n"                                                                     \
    "    float a = sin(t * 6.2832 / SAN_PULSO_A), b = sin(t * 6.2832 / SAN_PULSO_B + 1.7);\n"                \
    "    return clamp(0.5 + 0.32 * a + 0.18 * b, 0.0, 1.0);\n"                                               \
    "}\n"                                                                                                    \
    "float cansaco() { return sente() * (0.55 + 0.45 * pulso()); }\n"                                        \
    "vec3 cansa(vec3 c, vec2 uv) {\n"                                                                        \
    "    vec2 d = uv - 0.5;\n"                                                                               \
    "    float borda = smoothstep(0.02, 0.25, dot(d, d));\n"                                                 \
    "    float r = SAN_DESFOQUE * cansaco() * (0.15 + 0.85 * borda);\n"                                      \
    "    vec3 soma = vec3(0.0);\n"                                                                           \
    "    for (int i = 0; i < 8; i++) {\n"                                                                    \
    "        float ang = float(i) * 0.7854;\n"                                                               \
    "        soma += AMOSTRA(texture0, uv + vec2(cos(ang), sin(ang)) * r / resolucao).rgb;\n"                \
    "    }\n"                                                                                                \
    "    c = mix(c, soma * 0.125, clamp(r, 0.0, 1.0) * 0.8);\n"                                              \
    "    return mix(c, vec3(luma(c)), SAN_DESSATURA * sente());\n"                                           \
    "}\n"

// Sanidade, camada 2 — fita VHS gasta: ruído de luminância esticado na horizontal (células de 1 linha por
// SAN_RUIDO_LARGURA px, interpoladas, trocadas a 24 Hz com um quarto do quadro anterior), um pouco de ruído
// de cor só nas sombras, dropouts (riscos claros, curtos e raros; quase nunca acima de 70% de sanidade; ~2
// por quadro no máximo) e, abaixo de ~25%, um tremor horizontal breve numa faixa de linhas (tracking
// instável). O ruído é quase todo multiplicativo (proporcional ao brilho), com um piso aditivo bem pequeno
// para o preto: a cena é muito escura e um ruído aditivo forte cobria o contraste da porta e da criatura.
#define VHS_FITA_GLSL                                                                                        \
    "float ruido_fita(vec2 uv, float q) {\n"                                                                 \
    "    vec2 p = vec2(uv.x * resolucao.x / SAN_RUIDO_LARGURA, floor(uv.y * resolucao.y));\n"                \
    "    vec2 o = vec2(q * 7.0, q * 13.0);\n"                                                                \
    "    float x0 = floor(p.x), f = fract(p.x);\n"                                                           \
    "    float a = hash(vec2(x0, p.y) + o), b = hash(vec2(x0 + 1.0, p.y) + o);\n"                            \
    "    return mix(a, b, f * f * (3.0 - 2.0 * f)) - 0.5;\n"                                                 \
    "}\n"                                                                                                    \
    "float dropout(vec2 uv) {\n"                                                                             \
    "    float chance = smoothstep(0.3, 1.0, insanidade) * SAN_DROPOUT * 0.005;\n"                           \
    "    float slot = quadro(9.0, 499.0), linha = floor(uv.y * resolucao.y);\n"                              \
    "    float ativo = step(hash(vec2(linha, slot + 17.0)), chance);\n"                                      \
    "    float x0 = hash(vec2(linha, slot + 5.0));\n"                                                        \
    "    float fim = x0 + 0.03 + 0.09 * hash(vec2(slot, linha + 3.0));\n"                                    \
    "    return ativo * smoothstep(x0, x0 + 0.01, uv.x) * smoothstep(fim, fim - 0.01, uv.x);\n"              \
    "}\n"                                                                                                    \
    "float trilha(vec2 uv, float linha) {\n"                                                                 \
    "    float forte = smoothstep(0.75, 1.0, insanidade), slot = quadro(3.0, 499.0);\n"                      \
    "    float ativo = step(1.0 - 0.15 * forte, hash(vec2(slot, 29.0)));\n"                                  \
    "    float banda = smoothstep(0.025, 0.0, abs(uv.y - hash(vec2(slot, 31.0))));\n"                        \
    "    float vida = sin(fract(tempo * 3.0) * 3.14159);\n"                                                  \
    "    return ativo * banda * vida * (hash(vec2(linha, slot)) - 0.5) * 2.0 * SAN_TRACKING;\n"              \
    "}\n"                                                                                                    \
    "vec3 fita(vec3 c, vec2 uv) {\n"                                                                         \
    "    float q = quadro(24.0, 61.0);\n"                                                                    \
    "    float g = 0.75 * ruido_fita(uv, q) + 0.25 * ruido_fita(uv, mod(q + 60.0, 61.0));\n"                 \
    "    float lc = luma(c);\n"                                                                              \
    "    float f = sente();\n"                                                                               \
    "    c = c * (1.0 + 2.0 * g * SAN_RUIDO * f) + vec3(g * SAN_RUIDO_PRETO * f);\n"                         \
    "    vec2 pc = floor(uv * resolucao * 0.5);\n"                                                           \
    "    vec2 cr = vec2(hash(pc + vec2(q * 3.0, 41.0)), hash(pc + vec2(q * 5.0, 83.0))) - 0.5;\n"            \
    "    c += vec3(cr.x, -0.5 * (cr.x + cr.y), cr.y) * SAN_CROMA * f * (1.0 - lc);\n"                        \
    "    return mix(c, vec3(0.78, 0.80, 0.82), 0.5 * dropout(uv));\n"                                        \
    "}\n"

// O corpo é o mesmo nas duas versões: AMOSTRA e SAIDA trocam texture/texture2D e finalColor/gl_FragColor.
// As scanlines compensam o brilho médio que tiram (o escuro não fica mais escuro do que sem filtro).
// rodape: a faixa de troca de cabeça (3% de baixo); rola: uma faixa fraca de ruído que sobe devagar.
#define VHS_CORPO_GLSL                                                                                       \
    "void main() {\n"                                                                                        \
    "    vec2 uv = curva(fragTexCoord);\n"                                                                   \
    "    vec2 fora = step(vec2(0.0), uv) * step(uv, vec2(1.0));\n"                                           \
    "    float linha = floor(uv.y * resolucao.y);\n"                                                         \
    "    float janela = step(0.97, hash(vec2(quadro(3.0, 997.0), 7.0)));\n"                                  \
    "    float faixa = smoothstep(0.0, 0.08, 0.08 - abs(uv.y - fract(tempo * 0.37)));\n"                     \
    "    float rodape = tracking * smoothstep(0.97, 1.0, 1.0 - uv.y);\n"                                     \
    "    float altura_rola = abs(uv.y - fract(-tempo * 0.06));\n"                                            \
    "    float rola = rolagem * smoothstep(0.015, 0.0, altura_rola);\n"                                      \
    "    float tremor = hash(vec2(linha, quadro(30.0, 61.0))) - 0.5;\n"                                      \
    "    float empurra = rodape * 14.0 + rola * 2.0;\n"                                                      \
    "    uv.x += (janela * faixa * wobble + empurra * tremor + trilha(uv, linha)) / resolucao.x;\n"          \
    "    float px = aberracao / resolucao.x;\n"                                                              \
    "    vec3 c = vec3(AMOSTRA(texture0, uv + vec2(px, 0.0)).r, AMOSTRA(texture0, uv).g,\n"                  \
    "                  AMOSTRA(texture0, uv - vec2(px, 0.0)).b);\n"                                          \
    "    c = cansa(sangra(c, uv), uv);\n"                                                                    \
    "    c = tinge(mix(vec3(luma(c)), c, saturacao));\n"                                                     \
    "    float q = quadro(30.0, 61.0);\n"                                                                    \
    "    float n = hash(floor(uv * resolucao) + vec2(q * 7.0, q * 3.0)) - 0.5;\n"                            \
    "    c += n * (ruido * (0.3 + 0.7 * luma(c)) + rodape * 0.6 + rola * 0.12);\n"                           \
    "    c = fita(c, uv);\n"                                                                                 \
    "    float listra = 0.5 + 0.5 * sin(uv.y * resolucao.y * 3.14159);\n"                                    \
    "    c *= (1.0 + scanlines * 0.5) * (1.0 - scanlines * listra);\n"                                       \
    "    vec2 d = uv - 0.5;\n"                                                                               \
    "    c *= 1.0 - (vinheta + SAN_TUNEL * cansaco()) * dot(d, d) * 2.0;\n"                                  \
    "    c *= brilho * (1.0 - cintilacao * hash(vec2(quadro(20.0, 997.0), 3.0)));\n"                         \
    "    SAIDA = vec4(c * fora.x * fora.y, 1.0) * colDiffuse * fragColor;\n"                                 \
    "}\n"

#define VHS_FUNCOES_GLSL                                                                                     \
    VHS_UNIFORMES_GLSL VHS_CURVA_GLSL VHS_SANGRA_GLSL VHS_TOM_GLSL VHS_CANSACO_GLSL VHS_FITA_GLSL            \
        VHS_CORPO_GLSL

static const char *VHS_CAB_330 = "#version 330\n"
                                 "#define AMOSTRA texture\n"
                                 "#define SAIDA finalColor\n"
                                 "in vec2 fragTexCoord; in vec4 fragColor;\n"
                                 "out vec4 finalColor;\n";

static const char *VHS_CAB_100 = "#version 100\n"
                                 "precision mediump float;\n"
                                 "#define AMOSTRA texture2D\n"
                                 "#define SAIDA gl_FragColor\n"
                                 "varying vec2 fragTexCoord; varying vec4 fragColor;\n";

// ------------------------------------------------------------------ carga
// Os números do efeito de sanidade ficam em config.h (SANIDADE_*) e entram no shader como #define.
static bool monta_fonte(char *fonte, size_t cap, bool v100) {
    int n = snprintf(fonte, cap,
                     "%s"
                     "#define SAN_CURVA %.4f\n"
                     "#define SAN_DESFOQUE %.4f\n"
                     "#define SAN_TUNEL %.4f\n"
                     "#define SAN_DESSATURA %.4f\n"
                     "#define SAN_PULSO_A %.4f\n"
                     "#define SAN_PULSO_B %.4f\n"
                     "#define SAN_RUIDO %.4f\n"
                     "#define SAN_RUIDO_PRETO %.4f\n"
                     "#define SAN_RUIDO_LARGURA %.4f\n"
                     "#define SAN_CROMA %.4f\n"
                     "#define SAN_DROPOUT %.4f\n"
                     "#define SAN_TRACKING %.4f\n"
                     "%s",
                     v100 ? VHS_CAB_100 : VHS_CAB_330, (double)SANIDADE_CURVA, (double)SANIDADE_DESFOQUE_PX,
                     (double)SANIDADE_TUNEL, (double)SANIDADE_DESSATURA, (double)SANIDADE_PULSO_A_S,
                     (double)SANIDADE_PULSO_B_S, (double)SANIDADE_RUIDO, (double)SANIDADE_RUIDO_PRETO,
                     (double)SANIDADE_RUIDO_LARGURA_PX, (double)SANIDADE_CROMA, (double)SANIDADE_DROPOUT,
                     (double)SANIDADE_TRACKING_PX, VHS_FUNCOES_GLSL);
    return n > 0 && (size_t)n < cap;
}

void vhs_carrega(Recursos *rec) {
    static char fonte[16384]; // montado uma vez, na carga
    bool v100 = rlGetVersion() == RL_OPENGL_ES_20;
    rec->vhs_ok = false;
    if (!monta_fonte(fonte, sizeof(fonte), v100)) {
        TraceLog(LOG_WARNING, "PRECURSOR: shader VHS não coube no buffer; seguindo sem filtro");
        return;
    }
    rec->vhs = LoadShaderFromMemory(NULL, fonte);
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
// Com o preset 0 ("desligado") o shader ainda roda quando há grão de sanidade: os parâmetros do preset 0 são
// neutros, então só o grão aparece.
bool vhs_comeca(const Estado *e) {
    const Recursos *r = &e->rec;
    bool so_grao = e->vhs_preset == 0 && e->sanidade_grao > 0.001f;
    if (!r->vhs_ok || e->vhs_preset < 0 || e->vhs_preset >= N_PRESETS) return false;
    if (e->vhs_preset == 0 && !so_grao) return false;
    float tempo = e->tempo_total;
    float resolucao[2] = {(float)r->alvo_largura, (float)r->alvo_altura};
    SetShaderValue(r->vhs, r->loc_vhs[0], &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->vhs, r->loc_vhs[1], resolucao, SHADER_UNIFORM_VEC2);
    const float *p = PRESETS[e->vhs_preset];
    for (int i = 0; i < VHS_PARAMETROS; i++) // padrão MAP: coluna do preset → uniforme do shader
        SetShaderValue(r->vhs, r->loc_vhs[i + 2], &p[i], SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->vhs, r->loc_vhs[VHS_UNIFORMES - 1], &e->sanidade_grao, SHADER_UNIFORM_FLOAT);
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
