// sombreadores_pos.c — os textos GLSL dos passes de tela (efeitos.c): oclusão ambiente (SSAO), névoa
// volumétrica, bloom e a composição final (tone mapping e gradação de cor). Só 330: sem HDR no OpenGL ES 2,
// esses passes não rodam. Cada passe desenha um retângulo na tela inteira do alvo; texture0 é a entrada
// principal (a profundidade ou a cena) e as outras entram como samplers extras.
#include "sombreadores.h"
#include "config.h"

#define TEXTO(x)    #x
#define TEXTO_DE(x) TEXTO(x)
#define N_LUZES     TEXTO_DE(LUZES_SHADER)

// Posição a partir da profundidade (inversa da projeção; em espaço de vista no SSAO, de mundo na névoa).
#define RECONSTROI                                                                                           \
    "vec3 reconstroi(mat4 inversa, vec2 uv, float z) {\n"                                                    \
    "    vec4 p = inversa * vec4(uv * 2.0 - 1.0, z * 2.0 - 1.0, 1.0);\n"                                     \
    "    return p.xyz / p.w;\n"                                                                              \
    "}\n"

// ------------------------------------------------------------------ SSAO
// Oclusão ambiente em espaço de tela: 12 amostras num hemisfério em volta da normal (tirada das derivadas da
// posição), giradas por pixel; a amostra está ocluída se a superfície naquele ponto da tela está mais perto
// da câmera do que ela. A checagem de alcance evita que um objeto longe escureça o que está na frente dele.
static const char *SSAO_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform mat4 projInv; uniform mat4 proj; uniform float raio;\n"
    "out vec4 finalColor;\n" RECONSTROI
    "float sorteio(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }\n"
    "void main() {\n"
    "    float z = texture(texture0, fragTexCoord).r;\n"
    "    if (z >= 0.9999) { finalColor = vec4(1.0); return; }\n"
    "    vec3 p = reconstroi(projInv, fragTexCoord, z);\n"
    "    vec3 n = normalize(cross(dFdx(p), dFdy(p)));\n"
    "    if (dot(n, p) > 0.0) n = -n;\n"
    "    float a = sorteio(gl_FragCoord.xy) * 6.2831853;\n"
    "    vec3 r = normalize(vec3(cos(a), sin(a), 0.3));\n"
    "    vec3 t = normalize(r - n * dot(r, n)), b = cross(n, t);\n"
    "    float ocluido = 0.0;\n"
    "    for (int i = 0; i < 12; i++) {\n"
    "        float fi = float(i) + 0.5;\n"
    "        float ang = fi * 2.3999632, h = fract(fi * 0.618034);\n" // espiral de Fibonacci no hemisfério
    "        vec3 k = vec3(cos(ang) * sqrt(1.0 - h * h), sin(ang) * sqrt(1.0 - h * h), h);\n"
    "        float escala = mix(0.15, 1.0, (fi / 12.0) * (fi / 12.0));\n"
    "        vec3 s = p + (t * k.x + b * k.y + n * k.z) * raio * escala;\n"
    "        vec4 o = proj * vec4(s, 1.0);\n"
    "        vec2 uv = o.xy / o.w * 0.5 + 0.5;\n"
    "        float zc = reconstroi(projInv, uv, texture(texture0, uv).r).z;\n"
    "        float alcance = smoothstep(0.0, 1.0, raio / max(abs(p.z - zc), 1e-4));\n"
    "        ocluido += (zc >= s.z + 0.025 ? 1.0 : 0.0) * alcance;\n"
    "    }\n"
    "    finalColor = vec4(vec3(pow(1.0 - ocluido / 12.0, 1.5)), 1.0);\n"
    "}\n";

// ------------------------------------------------------------------ névoa volumétrica
// Raymarch da câmera até a superfície (no máximo 28 m), com um passo sorteado por pixel (sem faixas): em cada
// ponto, a densidade da névoa varia com uma poeira em ruído 3D que se move devagar, e a luz que ela espalha
// vem das luminárias (atenuação) e do cone da lanterna (com a sombra dela: o feixe para nos objetos).
// O espalhamento puxa para a frente (Henyey-Greenstein): o feixe da lanterna aparece quando se olha ao longo
// dele. Saída: rgb = luz acumulada, a = quanto da cena ainda passa (transmitância).
static const char *VOLUME_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform mat4 vpInv; uniform vec3 camera;\n"
    "uniform vec3 luzPos[" N_LUZES "]; uniform vec3 luzCor[" N_LUZES "]; uniform vec4 luzCone[" N_LUZES "];\n"
    "uniform float luzQueda[" N_LUZES "]; uniform int nLuzes; uniform int indiceLanterna;\n"
    "uniform sampler2D mapaSombra; uniform mat4 matSombra; uniform float usaSombra;\n"
    "uniform float densidade; uniform float tempo; uniform vec3 corAmbiente; uniform int passos;\n"
    // as luzes não atravessam paredes: o bit da luz na célula do ponto (Nivel.alcance_luz)
    "uniform sampler2D mapaAlcance; uniform int luzId[" N_LUZES "];\n"
    "bool alcanca(int id, vec3 p) {\n"
    "    if (id < 0) return true;\n"
    "    ivec2 c = ivec2(floor(p.xz / 2.0));\n"
    "    ivec2 t = textureSize(mapaAlcance, 0);\n"
    "    if (c.x < 0 || c.y < 0 || c.x >= t.x || c.y >= t.y) return true;\n"
    "    vec2 m = texelFetch(mapaAlcance, c, 0).rg * 255.0 + 0.5;\n"
    "    return (((int(m.x) | (int(m.y) << 8)) >> id) & 1) != 0;\n"
    "}\n"
    "out vec4 finalColor;\n" RECONSTROI
    "float sorteio(vec3 p) { return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453); }\n"
    "float ruido(vec3 p) {\n"
    "    vec3 i = floor(p), f = fract(p);\n"
    "    f = f * f * (3.0 - 2.0 * f);\n"
    "    return mix(mix(mix(sorteio(i), sorteio(i + vec3(1, 0, 0)), f.x),\n"
    "                   mix(sorteio(i + vec3(0, 1, 0)), sorteio(i + vec3(1, 1, 0)), f.x), f.y),\n"
    "               mix(mix(sorteio(i + vec3(0, 0, 1)), sorteio(i + vec3(1, 0, 1)), f.x),\n"
    "                   mix(sorteio(i + vec3(0, 1, 1)), sorteio(i + vec3(1, 1, 1)), f.x), f.y), f.z);\n"
    "}\n"
    "float sombra(vec3 p) {\n"
    "    vec4 q = matSombra * vec4(p, 1.0);\n"
    "    if (q.w <= 0.0) return 1.0;\n"
    "    vec2 s = q.xy / q.w * 0.5 + 0.5;\n"
    "    if (s.x < 0.0 || s.x > 1.0 || s.y < 0.0 || s.y > 1.0) return 1.0;\n"
    "    return texture(mapaSombra, s).r < length(p - luzPos[indiceLanterna]) / 30.0 - 0.004 ? 0.0 : 1.0;\n"
    "}\n"
    "void main() {\n"
    "    float z = texture(texture0, fragTexCoord).r;\n"
    "    vec3 alvo = reconstroi(vpInv, fragTexCoord, z);\n"
    "    vec3 raio = alvo - camera;\n"
    "    float dist = min(length(raio), 28.0);\n"
    "    vec3 dir = normalize(raio);\n"
    "    float passo = dist / float(passos);\n"
    "    float t = passo * fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453 + tempo);\n"
    "    vec3 luz = vec3(0.0);\n"
    "    float passa = 1.0;\n"
    "    for (int k = 0; k < 64; k++) {\n"
    "        if (k >= passos) break;\n"
    "        vec3 x = camera + dir * t;\n"
    "        float poeira = ruido(x * 1.7 + vec3(0.0, -tempo * 0.06, tempo * 0.03));\n"
    "        float d = densidade * (0.45 + 1.1 * poeira * poeira);\n"
    "        vec3 espalha = corAmbiente;\n"
    "        for (int i = 0; i < " N_LUZES "; i++) {\n"
    "            if (i >= nLuzes) break;\n"
    "            if (!alcanca(luzId[i], x)) continue;\n"
    "            vec3 l = luzPos[i] - x;\n"
    "            float dl = length(l);\n"
    "            vec3 li = l / max(dl, 0.001);\n"
    "            vec4 cone = luzCone[i];\n"
    "            float foco = cone.w < -1.0 ? 1.0 : smoothstep(cone.w, cone.w + 0.06, dot(-li, cone.xyz));\n"
    "            float atenua = 1.0 / (1.0 + luzQueda[i] * (0.09 * dl + 0.07 * dl * dl));\n"
    // o feixe da lanterna espalha mais para a frente
    "            float g = i == indiceLanterna ? 0.55 : 0.2;\n"
    "            float mu = dot(dir, -li);\n"
    "            float fase = 0.0796 * (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5);\n"
    "            float vis = (i == indiceLanterna && usaSombra > 0.5) ? sombra(x) : 1.0;\n"
    "            espalha += luzCor[i] * foco * atenua * fase * vis * (i == indiceLanterna ? 0.25 : 0.6);\n"
    "        }\n"
    "        luz += passa * d * espalha * passo;\n"
    "        passa *= exp(-d * passo);\n"
    "        t += passo;\n"
    "    }\n"
    "    finalColor = vec4(luz, passa);\n"
    "}\n";

// ------------------------------------------------------------------ bloom
// Descida: a média de 4 amostras (com um corte nos brilhos no primeiro nível); subida: um filtro tenda de 9
// amostras somado ao nível de cima (o brilho se espalha largo sem quadriculado).
static const char *BLOOM_DESCE_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform vec2 texel; uniform float corte;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec3 c = texture(texture0, fragTexCoord + texel * vec2(-1, -1)).rgb +\n"
    "             texture(texture0, fragTexCoord + texel * vec2(1, -1)).rgb +\n"
    "             texture(texture0, fragTexCoord + texel * vec2(-1, 1)).rgb +\n"
    "             texture(texture0, fragTexCoord + texel * vec2(1, 1)).rgb;\n"
    "    c *= 0.25;\n"
    "    float brilho = max(c.r, max(c.g, c.b));\n"
    "    c *= corte > 0.0 ? max(brilho - corte, 0.0) / max(brilho, 1e-4) : 1.0;\n"
    "    finalColor = vec4(c, 1.0);\n"
    "}\n";

static const char *BLOOM_SOBE_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform vec2 texel;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec3 c = texture(texture0, fragTexCoord).rgb * 4.0;\n"
    "    c += (texture(texture0, fragTexCoord + vec2(texel.x, 0)).rgb +\n"
    "          texture(texture0, fragTexCoord - vec2(texel.x, 0)).rgb +\n"
    "          texture(texture0, fragTexCoord + vec2(0, texel.y)).rgb +\n"
    "          texture(texture0, fragTexCoord - vec2(0, texel.y)).rgb) * 2.0;\n"
    "    c += texture(texture0, fragTexCoord + texel).rgb + texture(texture0, fragTexCoord - texel).rgb +\n"
    "         texture(texture0, fragTexCoord + vec2(texel.x, -texel.y)).rgb +\n"
    "         texture(texture0, fragTexCoord + vec2(-texel.x, texel.y)).rgb;\n"
    "    finalColor = vec4(c / 16.0, 1.0);\n"
    "}\n";

// ------------------------------------------------------------------ composição
// A cena × oclusão, a névoa volumétrica por cima (a cena atenuada pela transmitância), o bloom; depois a
// exposição, a curva ACES, a gradação de cor (sombras puxando para o azul-petróleo, luzes para o verde-claro,
// menos saturação, contraste, o tom frio nos escuros, mas não no preto), a vinheta e o grão (os dois
// parametrizáveis), e a volta para sRGB.
static const char *COMPOE_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform sampler2D mapaAo; uniform sampler2D mapaVolume;\n"
    "uniform sampler2D mapaBloom;\n"
    "uniform float exposicao; uniform float usaAo; uniform float usaVolume; uniform float bloom;\n"
    "uniform vec3 sombraCor; uniform vec3 luzCor; uniform float saturacao; uniform float contraste;\n"
    "uniform float vinheta; uniform float grao; uniform float aberracao; uniform float tempo;\n"
    "uniform vec2 tela; uniform vec3 elevacao; uniform float pisoPreto;\n"
    "out vec4 finalColor;\n"
    "vec3 aces(vec3 x) {\n"
    "    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);\n"
    "}\n"
    "vec3 cena(vec2 uv) {\n"
    "    vec3 c = texture(texture0, uv).rgb;\n"
    "    if (usaAo > 0.5) c *= mix(1.0, texture(mapaAo, uv).r, 0.85);\n"
    "    if (usaVolume > 0.5) { vec4 v = texture(mapaVolume, uv); c = c * v.a + v.rgb; }\n"
    "    return c + texture(mapaBloom, uv).rgb * bloom;\n"
    "}\n"
    "void main() {\n"
    "    vec2 centro = fragTexCoord - 0.5;\n"
    "    vec2 desvio = centro * aberracao * dot(centro, centro);\n" // aberração: só nas bordas
    "    vec3 hdr = vec3(cena(fragTexCoord + desvio).r, cena(fragTexCoord).g,\n"
    "                    cena(fragTexCoord - desvio).b);\n"
    "    vec3 c = aces(hdr * exposicao);\n"
    "    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    "    c = mix(vec3(luma), c, saturacao);\n"
    "    c = mix(c * sombraCor, c * luzCor, smoothstep(0.0, 0.6, luma));\n" // split toning
    "    c = clamp((c - 0.5) * contraste + 0.5, 0.0, 1.0);\n"
    "    c = c + elevacao * smoothstep(0.0, pisoPreto, luma) * (1.0 - c);\n" // o preto puro fica preto
    "    float borda = length(centro * vec2(tela.x / tela.y, 1.0)) * 0.85;\n"
    "    c *= mix(1.0, smoothstep(0.95, 0.25, borda), vinheta);\n"
    "    c = pow(c, vec3(1.0 / 2.2));\n"
    "    float g = fract(sin(dot(gl_FragCoord.xy + tempo * 61.0, vec2(12.9898, 78.233))) * 43758.5453);\n"
    "    c += (g - 0.5) * grao * (1.3 - luma);\n" // como em filme: mais grão no escuro
    "    finalColor = vec4(c, 1.0);\n"
    "}\n";

const char *sombreador_ssao_fs(void) {
    return SSAO_FS;
}

const char *sombreador_volume_fs(void) {
    return VOLUME_FS;
}

const char *sombreador_bloom_desce_fs(void) {
    return BLOOM_DESCE_FS;
}

const char *sombreador_bloom_sobe_fs(void) {
    return BLOOM_SOBE_FS;
}

const char *sombreador_compoe_fs(void) {
    return COMPOE_FS;
}
