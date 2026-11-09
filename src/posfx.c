// posfx.c — render target, filtro VHS (GLSL embutido, versões 330 e 100).
#include "posfx.h"
#include "rlgl.h"
#include "raymath.h"
#include <stddef.h>

// ------------------------------------------------------------------ shaders do VHS
// Ruído ~4%, aberração cromática ~1,5 px, scanlines ~6%, vinheta leve e um "wobble" horizontal raro.
static const char *VHS_FS_330 =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "uniform float tempo; uniform vec2 resolucao;\n"
    "uniform float ruido; uniform float aberracao; uniform float scanlines; uniform float vinheta; uniform "
    "float wobble;\n"
    "out vec4 finalColor;\n"
    "float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    float janela = step(0.97, hash(vec2(floor(tempo * 3.0), 7.0)));\n"
    "    float faixa = smoothstep(0.0, 0.08, 0.08 - abs(uv.y - fract(tempo * 0.37)));\n"
    "    uv.x += janela * faixa * wobble / resolucao.x;\n"
    "    float px = aberracao / resolucao.x;\n"
    "    vec3 c = vec3(texture(texture0, uv + vec2(px, 0.0)).r, texture(texture0, uv).g,\n"
    "                  texture(texture0, uv - vec2(px, 0.0)).b);\n"
    "    c += (hash(uv * resolucao + vec2(tempo * 61.0, tempo * 17.0)) - 0.5) * ruido;\n"
    "    c *= 1.0 - scanlines * (0.5 + 0.5 * sin(uv.y * resolucao.y * 3.14159));\n"
    "    vec2 d = uv - 0.5;\n"
    "    c *= 1.0 - vinheta * dot(d, d) * 2.0;\n"
    "    finalColor = vec4(c, 1.0) * colDiffuse * fragColor;\n"
    "}\n";

static const char *VHS_FS_100 =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord; varying vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "uniform float tempo; uniform vec2 resolucao;\n"
    "uniform float ruido; uniform float aberracao; uniform float scanlines; uniform float vinheta; uniform "
    "float wobble;\n"
    "float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    float janela = step(0.97, hash(vec2(floor(tempo * 3.0), 7.0)));\n"
    "    float faixa = smoothstep(0.0, 0.08, 0.08 - abs(uv.y - fract(tempo * 0.37)));\n"
    "    uv.x += janela * faixa * wobble / resolucao.x;\n"
    "    float px = aberracao / resolucao.x;\n"
    "    vec3 c = vec3(texture2D(texture0, uv + vec2(px, 0.0)).r, texture2D(texture0, uv).g,\n"
    "                  texture2D(texture0, uv - vec2(px, 0.0)).b);\n"
    "    c += (hash(uv * resolucao + vec2(tempo * 61.0, tempo * 17.0)) - 0.5) * ruido;\n"
    "    c *= 1.0 - scanlines * (0.5 + 0.5 * sin(uv.y * resolucao.y * 3.14159));\n"
    "    vec2 d = uv - 0.5;\n"
    "    c *= 1.0 - vinheta * dot(d, d) * 2.0;\n"
    "    gl_FragColor = vec4(c, 1.0) * colDiffuse * fragColor;\n"
    "}\n";

// ------------------------------------------------------------------ shaders de luz e névoa
// Até 4 luzes pontuais, ambiente bem baixo e névoa exponencial quase preta. A normal sai das derivadas
// da posição (sombreado chapado, bom para low-poly); na versão 100 só há atenuação por distância.
static const char *LUZ_VS_330 = "#version 330\n"
                                "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
                                "uniform mat4 mvp; uniform mat4 matModel;\n"
                                "out vec3 fragPosition; out vec4 fragColor; out vec2 fragTexCoord;\n"
                                "void main() {\n"
                                "    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
                                "    fragColor = vertexColor; fragTexCoord = vertexTexCoord;\n"
                                "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
                                "}\n";

static const char *LUZ_FS_330 = "#version 330\n"
                                "in vec3 fragPosition; in vec4 fragColor; in vec2 fragTexCoord;\n"
                                "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
                                "uniform vec3 luzPos[4]; uniform vec3 luzCor[4];\n"
                                "uniform float ambiente; uniform float nevoa; uniform vec3 camera;\n"
                                "out vec4 finalColor;\n"
                                "void main() {\n"
                                "    vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
                                "    vec3 n = normalize(cross(dFdx(fragPosition), dFdy(fragPosition)));\n"
                                "    vec3 paraCamera = camera - fragPosition;\n"
                                "    if (dot(n, paraCamera) < 0.0) n = -n;\n"
                                "    vec3 luz = vec3(ambiente);\n"
                                "    for (int i = 0; i < 4; i++) {\n"
                                "        vec3 l = luzPos[i] - fragPosition;\n"
                                "        float d = length(l);\n"
                                "        float atenua = 1.0 / (1.0 + 0.09 * d + 0.07 * d * d);\n"
                                "        float difusa = max(dot(n, l / max(d, 0.001)), 0.0) * 0.85 + 0.15;\n"
                                "        luz += luzCor[i] * atenua * difusa;\n"
                                "    }\n"
                                "    vec3 cor = base.rgb * luz;\n"
                                "    float fator = clamp(1.0 - exp(-nevoa * length(paraCamera)), 0.0, 1.0);\n"
                                "    finalColor = vec4(mix(cor, vec3(0.012, 0.016, 0.013), fator), base.a);\n"
                                "}\n";

static const char *LUZ_VS_100 =
    "#version 100\n"
    "attribute vec3 vertexPosition; attribute vec2 vertexTexCoord; attribute vec4 vertexColor;\n"
    "uniform mat4 mvp; uniform mat4 matModel;\n"
    "varying vec3 fragPosition; varying vec4 fragColor; varying vec2 fragTexCoord;\n"
    "void main() {\n"
    "    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "    fragColor = vertexColor; fragTexCoord = vertexTexCoord;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *LUZ_FS_100 =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec3 fragPosition; varying vec4 fragColor; varying vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "uniform vec3 luzPos[4]; uniform vec3 luzCor[4];\n"
    "uniform float ambiente; uniform float nevoa; uniform vec3 camera;\n"
    "void main() {\n"
    "    vec4 base = texture2D(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "    vec3 luz = vec3(ambiente);\n"
    "    for (int i = 0; i < 4; i++) {\n"
    "        float d = length(luzPos[i] - fragPosition);\n"
    "        luz += luzCor[i] / (1.0 + 0.09 * d + 0.07 * d * d);\n"
    "    }\n"
    "    vec3 cor = base.rgb * luz;\n"
    "    float fator = clamp(1.0 - exp(-nevoa * length(camera - fragPosition)), 0.0, 1.0);\n"
    "    gl_FragColor = vec4(mix(cor, vec3(0.012, 0.016, 0.013), fator), base.a);\n"
    "}\n";

static bool usa_glsl_100(void) {
    return rlGetVersion() == RL_OPENGL_ES_20;
}

static void cria_alvo(Recursos *rec, int largura_janela, int altura_janela) {
    int w = (int)(largura_janela * ESCALA_RENDER);
    int h = (int)(altura_janela * ESCALA_RENDER);
    if (w < 64) w = 64;
    if (h < 36) h = 36;
    rec->alvo = LoadRenderTexture(w, h);
    SetTextureFilter(rec->alvo.texture, TEXTURE_FILTER_BILINEAR);
    rec->alvo_largura = w;
    rec->alvo_altura = h;
}

static void carrega_vhs(Recursos *rec) {
    rec->vhs = LoadShaderFromMemory(NULL, usa_glsl_100() ? VHS_FS_100 : VHS_FS_330);
    rec->vhs_ok = IsShaderValid(rec->vhs);
    if (!rec->vhs_ok) {
        TraceLog(LOG_WARNING, "PRECURSOR: shader VHS não compilou; seguindo sem filtro");
        return;
    }
    rec->loc_vhs_tempo = GetShaderLocation(rec->vhs, "tempo");
    rec->loc_vhs_resolucao = GetShaderLocation(rec->vhs, "resolucao");
    rec->loc_vhs_ruido = GetShaderLocation(rec->vhs, "ruido");
    rec->loc_vhs_aberracao = GetShaderLocation(rec->vhs, "aberracao");
    rec->loc_vhs_scanlines = GetShaderLocation(rec->vhs, "scanlines");
    rec->loc_vhs_vinheta = GetShaderLocation(rec->vhs, "vinheta");
    rec->loc_vhs_wobble = GetShaderLocation(rec->vhs, "wobble");
}

static void carrega_luz(Recursos *rec) {
    bool v100 = usa_glsl_100();
    rec->luz = LoadShaderFromMemory(v100 ? LUZ_VS_100 : LUZ_VS_330, v100 ? LUZ_FS_100 : LUZ_FS_330);
    rec->luz_ok = IsShaderValid(rec->luz);
    if (!rec->luz_ok) {
        TraceLog(LOG_WARNING, "PRECURSOR: shader de luz não compilou; seguindo com cores chapadas");
        return;
    }
    rec->loc_luz_pos = GetShaderLocation(rec->luz, "luzPos");
    rec->loc_luz_cor = GetShaderLocation(rec->luz, "luzCor");
    rec->loc_luz_ambiente = GetShaderLocation(rec->luz, "ambiente");
    rec->loc_luz_nevoa = GetShaderLocation(rec->luz, "nevoa");
    rec->loc_luz_camera = GetShaderLocation(rec->luz, "camera");
    rec->loc_luz_modelo = GetShaderLocation(rec->luz, "matModel");
}

void posfx_inicia(Recursos *rec) {
    cria_alvo(rec, GetScreenWidth(), GetScreenHeight());
    carrega_vhs(rec);
    carrega_luz(rec);
}

void posfx_finaliza(Recursos *rec) {
    UnloadRenderTexture(rec->alvo);
    if (rec->vhs_ok) UnloadShader(rec->vhs);
    if (rec->luz_ok) UnloadShader(rec->luz);
}

void posfx_comeca_luz(const Estado *e, const Vector3 *posicoes, const Vector3 *cores, int n, float ambiente) {
    const Recursos *r = &e->rec;
    if (!r->luz_ok) return;
    Vector3 pos[4] = {0}, cor[4] = {0};
    for (int i = 0; i < n && i < 4; i++) {
        pos[i] = posicoes[i];
        cor[i] = cores[i];
    }
    float nevoa = NEVOA_DENSIDADE;
    SetShaderValueV(r->luz, r->loc_luz_pos, pos, SHADER_UNIFORM_VEC3, 4);
    SetShaderValueV(r->luz, r->loc_luz_cor, cor, SHADER_UNIFORM_VEC3, 4);
    SetShaderValue(r->luz, r->loc_luz_ambiente, &ambiente, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_nevoa, &nevoa, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_camera, &e->camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(r->luz, r->loc_luz_modelo, MatrixIdentity());
    BeginShaderMode(r->luz);
}

void posfx_termina_luz(const Estado *e) {
    if (e->rec.luz_ok) EndShaderMode();
}

void posfx_atualiza(Estado *e, float dt) {
    (void)dt;
    int w = (int)(GetScreenWidth() * ESCALA_RENDER);
    int h = (int)(GetScreenHeight() * ESCALA_RENDER);
    if (w != e->rec.alvo_largura || h != e->rec.alvo_altura) {
        UnloadRenderTexture(e->rec.alvo);
        cria_alvo(&e->rec, GetScreenWidth(), GetScreenHeight());
    }
#if TECLAS_DEBUG
    if (e->in.f1) e->vhs_ligado = !e->vhs_ligado;
#endif
}

void posfx_inicia_cena(const Estado *e) {
    BeginTextureMode(e->rec.alvo);
}

void posfx_termina_cena(const Estado *e) {
    (void)e;
    EndTextureMode();
}

static void envia_uniformes_vhs(const Estado *e) {
    const Recursos *r = &e->rec;
    float tempo = e->tempo_total;
    float resolucao[2] = {(float)r->alvo_largura, (float)r->alvo_altura};
    float ruido = VHS_RUIDO, aberracao = VHS_ABERRACAO_PX, scan = VHS_SCANLINES;
    float vinheta = VHS_VINHETA, wobble = VHS_WOBBLE_PX;
    SetShaderValue(r->vhs, r->loc_vhs_tempo, &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->vhs, r->loc_vhs_resolucao, resolucao, SHADER_UNIFORM_VEC2);
    SetShaderValue(r->vhs, r->loc_vhs_ruido, &ruido, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->vhs, r->loc_vhs_aberracao, &aberracao, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->vhs, r->loc_vhs_scanlines, &scan, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->vhs, r->loc_vhs_vinheta, &vinheta, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->vhs, r->loc_vhs_wobble, &wobble, SHADER_UNIFORM_FLOAT);
}

void posfx_desenha(const Estado *e) {
    const Recursos *r = &e->rec;
    // A textura do render target fica de cabeça para baixo: altura negativa desvira.
    Rectangle origem = {0, 0, (float)r->alvo.texture.width, -(float)r->alvo.texture.height};
    Rectangle destino = {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
    bool com_vhs = e->vhs_ligado && r->vhs_ok;
    if (com_vhs) {
        envia_uniformes_vhs(e);
        BeginShaderMode(r->vhs);
    }
    DrawTexturePro(r->alvo.texture, origem, destino, (Vector2){0, 0}, 0.0f, WHITE);
    if (com_vhs) EndShaderMode();
}
