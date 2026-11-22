// posfx.c — render target e luz e névoa (GLSL embutido, versões 330 e 100); o VHS fica em vhs.c.
#include "posfx.h"
#include "vhs.h"
#include "rlgl.h"
#include "raymath.h"
#include <stddef.h>

// ------------------------------------------------------------------ shaders de luz e névoa
// O número de luzes entra no texto do shader a partir de LUZES_SHADER (config.h): um lugar só para mudar.
#define TEXTO(x)    #x
#define TEXTO_DE(x) TEXTO(x)
#define N_LUZES     TEXTO_DE(LUZES_SHADER)
// Até LUZES_SHADER luzes pontuais, ambiente bem baixo e névoa exponencial quase preta. A normal sai das
// derivadas da posição (sombreado chapado, bom para low-poly); na versão 100 só há atenuação por distância.
// luzCone[i]: xyz = direção do holofote, w = cosseno da meia-abertura (w < -1: luz pontual comum); fora
// do cone sobram 15%. luzQueda[i]: quanto a luz enfraquece com a distância (1 = lâmpada comum; menor
// = vai mais longe, como a lanterna). contraluz: cor somada nas bordas (só o Precursor liga; o resto recebe
// zero).
static const char *LUZ_VS_330 = "#version 330\n"
                                "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;\n"
                                "uniform mat4 mvp; uniform mat4 matModel;\n"
                                "out vec3 fragPosition; out vec4 fragColor; out vec2 fragTexCoord;\n"
                                "void main() {\n"
                                "    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
                                "    fragColor = vertexColor; fragTexCoord = vertexTexCoord;\n"
                                "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
                                "}\n";

static const char *LUZ_FS_330 =
    "#version 330\n"
    "in vec3 fragPosition; in vec4 fragColor; in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "uniform vec3 luzPos[" N_LUZES "]; uniform vec3 luzCor[" N_LUZES "]; uniform vec4 luzCone[" N_LUZES "];\n"
    "uniform float luzQueda[" N_LUZES "];\n"
    "uniform float ambiente; uniform float nevoa; uniform vec3 camera;\n"
    "uniform vec3 contraluz;\n"
    "out vec4 finalColor;\n"
    "float foco(vec4 cone, vec3 paraLuz) {\n"
    "    if (cone.w < -1.0) return 1.0;\n"
    "    return mix(0.15, 1.0, smoothstep(cone.w, cone.w + 0.06, dot(-paraLuz, cone.xyz)));\n"
    "}\n"
    "void main() {\n"
    "    vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "    vec3 n = normalize(cross(dFdx(fragPosition), dFdy(fragPosition)));\n"
    "    vec3 paraCamera = camera - fragPosition;\n"
    "    if (dot(n, paraCamera) < 0.0) n = -n;\n"
    "    vec3 luz = vec3(ambiente);\n"
    "    for (int i = 0; i < " N_LUZES "; i++) {\n"
    "        vec3 l = luzPos[i] - fragPosition;\n"
    "        float d = length(l);\n"
    "        vec3 dir = l / max(d, 0.001);\n"
    "        float atenua = 1.0 / (1.0 + luzQueda[i] * (0.09 * d + 0.07 * d * d));\n"
    "        float difusa = max(dot(n, dir), 0.0) * 0.85 + 0.15;\n"
    "        luz += luzCor[i] * atenua * difusa * foco(luzCone[i], dir);\n"
    "    }\n"
    "    float borda = 1.0 - clamp(dot(n, normalize(paraCamera)), 0.0, 1.0);\n"
    "    vec3 cor = base.rgb * luz + contraluz * pow(borda, 2.5);\n"
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
    "uniform vec3 luzPos[" N_LUZES "]; uniform vec3 luzCor[" N_LUZES "]; uniform vec4 luzCone[" N_LUZES "];\n"
    "uniform float luzQueda[" N_LUZES "];\n"
    "uniform float ambiente; uniform float nevoa; uniform vec3 camera;\n"
    "uniform vec3 contraluz;\n"
    "void main() {\n"
    "    vec4 base = texture2D(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "    vec3 luz = vec3(ambiente);\n"
    "    for (int i = 0; i < " N_LUZES "; i++) {\n"
    "        vec3 l = luzPos[i] - fragPosition;\n"
    "        float d = length(l);\n"
    "        vec4 cone = luzCone[i];\n"
    "        float foco = cone.w < -1.0 ? 1.0\n"
    "            : mix(0.15, 1.0, smoothstep(cone.w, cone.w + 0.06, dot(-l / max(d, 0.001), cone.xyz)));\n"
    "        luz += luzCor[i] * foco / (1.0 + luzQueda[i] * (0.09 * d + 0.07 * d * d));\n"
    "    }\n"
    "    vec3 cor = base.rgb * luz + contraluz * 0.2;\n" // sem normal na versão 100: um brilho fraco por
                                                         // igual
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
    rec->alvo_susto = LoadRenderTexture(w, h);
    SetTextureFilter(rec->alvo.texture, TEXTURE_FILTER_BILINEAR);
    rec->alvo_largura = w;
    rec->alvo_altura = h;
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
    rec->loc_luz_cone = GetShaderLocation(rec->luz, "luzCone");
    rec->loc_luz_queda = GetShaderLocation(rec->luz, "luzQueda");
    rec->loc_luz_contraluz = GetShaderLocation(rec->luz, "contraluz");
}

void posfx_inicia(Recursos *rec) {
    cria_alvo(rec, GetScreenWidth(), GetScreenHeight());
    vhs_carrega(rec);
    carrega_luz(rec);
}

void posfx_finaliza(Recursos *rec) {
    UnloadRenderTexture(rec->alvo);
    UnloadRenderTexture(rec->alvo_susto);
    vhs_descarrega(rec);
    if (rec->luz_ok) UnloadShader(rec->luz);
}

void posfx_comeca_luz(const Estado *e, const Vector3 *posicoes, const Vector3 *cores, const Vector4 *cones,
                      const float *quedas, int n, float ambiente) {
    const Recursos *r = &e->rec;
    if (!r->luz_ok) return;
    Vector3 pos[LUZES_SHADER] = {0}, cor[LUZES_SHADER] = {0}, sem_contraluz = {0};
    Vector4 cone[LUZES_SHADER];
    float queda[LUZES_SHADER];
    for (int i = 0; i < LUZES_SHADER; i++) { // padrão MAP: as luzes do quadro → os vetores do shader
        bool tem = i < n;
        pos[i] = tem ? posicoes[i] : (Vector3){0};
        cor[i] = tem ? cores[i] : (Vector3){0};
        cone[i] = tem ? cones[i] : (Vector4){0, -1, 0, -2.0f};
        queda[i] = tem ? quedas[i] : 1.0f;
    }
    float nevoa = NEVOA_DENSIDADE;
    SetShaderValueV(r->luz, r->loc_luz_pos, pos, SHADER_UNIFORM_VEC3, LUZES_SHADER);
    SetShaderValueV(r->luz, r->loc_luz_cor, cor, SHADER_UNIFORM_VEC3, LUZES_SHADER);
    SetShaderValueV(r->luz, r->loc_luz_cone, cone, SHADER_UNIFORM_VEC4, LUZES_SHADER);
    SetShaderValueV(r->luz, r->loc_luz_queda, queda, SHADER_UNIFORM_FLOAT, LUZES_SHADER);
    SetShaderValue(r->luz, r->loc_luz_contraluz, &sem_contraluz, SHADER_UNIFORM_VEC3);
    SetShaderValue(r->luz, r->loc_luz_ambiente, &ambiente, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_nevoa, &nevoa, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_camera, &e->camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(r->luz, r->loc_luz_modelo, MatrixIdentity());
    BeginShaderMode(r->luz);
}

void posfx_termina_luz(const Estado *e) {
    if (e->rec.luz_ok) EndShaderMode();
}

void posfx_olho(const Estado *e, Vector3 olho) {
    const Recursos *r = &e->rec;
    if (r->luz_ok) SetShaderValue(r->luz, r->loc_luz_camera, &olho, SHADER_UNIFORM_VEC3);
}

// A geometria imediata (DrawCube...) fica num lote até ser enviada; esvaziar o lote antes de trocar o
// uniforme garante que a contraluz pegue só o que for desenhado depois desta chamada.
// Chame FORA de rlPushMatrix/rlPopMatrix: ao enviar o lote, a raylib manda a transformação atual como
// matModel.
void posfx_contraluz(const Estado *e, Color cor, float intensidade) {
    const Recursos *r = &e->rec;
    if (!r->luz_ok) return;
    rlDrawRenderBatchActive();
    Vector3 v = {cor.r / 255.0f * intensidade, cor.g / 255.0f * intensidade, cor.b / 255.0f * intensidade};
    SetShaderValue(r->luz, r->loc_luz_contraluz, &v, SHADER_UNIFORM_VEC3);
}

void posfx_atualiza(Estado *e, float dt) {
    (void)dt;
    int w = (int)(GetScreenWidth() * ESCALA_RENDER);
    int h = (int)(GetScreenHeight() * ESCALA_RENDER);
    if (w != e->rec.alvo_largura || h != e->rec.alvo_altura) {
        UnloadRenderTexture(e->rec.alvo);
        UnloadRenderTexture(e->rec.alvo_susto);
        cria_alvo(&e->rec, GetScreenWidth(), GetScreenHeight());
    }
}

void posfx_inicia_cena(const Estado *e) {
    BeginTextureMode(e->rec.alvo);
}

void posfx_termina_cena(const Estado *e) {
    (void)e;
    EndTextureMode();
}

void posfx_desenha(const Estado *e) {
    const Recursos *r = &e->rec;
    // A textura do render target fica de cabeça para baixo: altura negativa desvira.
    Rectangle origem = {0, 0, (float)r->alvo.texture.width, -(float)r->alvo.texture.height};
    Rectangle destino = {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
    bool com_vhs = vhs_comeca(e);
    DrawTexturePro(r->alvo.texture, origem, destino, (Vector2){0, 0}, 0.0f, WHITE);
    if (com_vhs) vhs_termina(e);
}
