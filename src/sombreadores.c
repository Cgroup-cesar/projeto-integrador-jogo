// sombreadores.c — os textos GLSL do jogo (embutidos: o jogo não depende de arquivo de shader). As versões
// 330 são o caminho principal (OpenGL 3.3 core); as 100 são o fallback de OpenGL ES 2 (navegador), sem PBR.
#include "sombreadores.h"
#include "config.h"

// ------------------------------------------------------------------ shaders de luz e névoa
// O número de luzes entra no texto do shader a partir de LUZES_SHADER (config.h): um lugar só para mudar.
#define TEXTO(x)    #x
#define TEXTO_DE(x) TEXTO(x)
#define N_LUZES     TEXTO_DE(LUZES_SHADER)
#define BRILHO_AGUA "90.0" // versão 100: expoente do brilho no chão molhado
#define ALCANCE_SOMBRA "30.0" // metros: a distância guardada no mapa de sombra vai de 0 a 1 até aqui
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

// PBR metálico/rugoso (GGX, Smith-Schlick, Fresnel de Schlick) em espaço linear, saída HDR (o tone mapping
// vem depois, em posfx.c). Com usaMapas = 1, a normal sai de mapaNormal pelo cotangent frame (sem tangentes
// na malha: as derivadas da posição e da UV dão a base) e a oclusão, a rugosidade e o metal, de mapaOrm; sem
// mapas, a normal é a da face e a rugosidade e o metal vêm dos uniformes. molhado (0..1): água por cima,
// quase espelho. A difusa tem um embrulho leve (15%) para a face de costas para a luz não ficar preta.
static const char *LUZ_FS_330 =
    "#version 330\n"
    "in vec3 fragPosition; in vec4 fragColor; in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "uniform sampler2D mapaNormal; uniform sampler2D mapaOrm; uniform float usaMapas;\n"
    "uniform float rugosidade; uniform float metalico;\n"
    "uniform vec3 luzPos[" N_LUZES "]; uniform vec3 luzCor[" N_LUZES "]; uniform vec4 luzCone[" N_LUZES "];\n"
    "uniform float luzQueda[" N_LUZES "];\n"
    "uniform float ambiente; uniform float nevoa; uniform vec3 camera;\n"
    "uniform vec3 contraluz; uniform float molhado;\n"
    "uniform sampler2D mapaReflexo; uniform float reflexo; uniform float agua; uniform vec2 tela;\n"
    "uniform float direto; uniform float exposicao;\n"
    "uniform float modoSombra; uniform vec3 luzSombra;\n"
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
    "uniform sampler2D mapaSombra; uniform mat4 matSombra; uniform float usaSombra;\n"
    "out vec4 finalColor;\n"
    "const float PI = 3.14159265;\n"
    // ruído de valor em coordenadas de mundo: a umidade do chão não repete junto com a textura
    "float sorteio(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }\n"
    "float ruido(vec2 p) {\n"
    "    vec2 i = floor(p), f = fract(p);\n"
    "    f = f * f * (3.0 - 2.0 * f);\n"
    "    return mix(mix(sorteio(i), sorteio(i + vec2(1.0, 0.0)), f.x),\n"
    "               mix(sorteio(i + vec2(0.0, 1.0)), sorteio(i + vec2(1.0)), f.x), f.y);\n"
    "}\n"
    "float fbm(vec2 p) {\n"
    "    float s = 0.0, a = 0.5;\n"
    "    for (int k = 0; k < 4; k++) { s += a * ruido(p); p = p * 2.03 + 17.0; a *= 0.5; }\n"
    "    return s;\n"
    "}\n"
    "float foco(vec4 cone, vec3 paraLuz) {\n"
    "    if (cone.w < -1.0) return 1.0;\n"
    "    return mix(0.15, 1.0, smoothstep(cone.w, cone.w + 0.06, dot(-paraLuz, cone.xyz)));\n"
    "}\n"
    "uniform vec3 celaMin; uniform vec3 celaMax; uniform float celaDensidade; uniform vec3 celaCor;\n"
    "uniform float celaEspalha; uniform float celaEspalhaLanterna; uniform int indiceLanterna;\n"
    "vec3 luzNaNevoa(vec3 p) {\n"
    "    vec3 soma = vec3(0.0);\n"
    "    for (int i = 0; i < " N_LUZES "; i++) {\n"
    "        vec3 l = luzPos[i] - p;\n"
    "        float d = length(l);\n"
    "        float atenua = 1.0 / (1.0 + luzQueda[i] * (0.09 * d + 0.07 * d * d));\n"
    "        bool dentro = all(greaterThanEqual(luzPos[i], celaMin)) &&\n"
    "                      all(lessThanEqual(luzPos[i], celaMax));\n"
    // a névoa devolve as luzes de dentro da cela (holofotes e flash) e, bem menos, a lanterna
    "        float peso = i == indiceLanterna ? celaEspalhaLanterna : (dentro ? celaEspalha : 0.0);\n"
    "        soma += luzCor[i] * atenua * foco(luzCone[i], l / max(d, 0.001)) * peso;\n"
    "    }\n"
    "    return soma;\n"
    "}\n"
    "vec3 nevoaDaCela(vec3 cor, vec3 ponto) {\n"
    "    vec3 raio = ponto - camera;\n"
    "    float dist = length(raio);\n"
    "    vec3 d = raio / max(dist, 0.0001);\n"
    "    vec3 dd = mix(d, vec3(0.00001), vec3(lessThan(abs(d), vec3(0.00001))));\n"
    "    vec3 t0 = (celaMin - camera) / dd, t1 = (celaMax - camera) / dd;\n"
    "    vec3 tmin = min(t0, t1), tmax = max(t0, t1);\n"
    "    float a = max(max(max(tmin.x, tmin.y), tmin.z), 0.0);\n"
    "    float b = min(min(min(tmax.x, tmax.y), tmax.z), dist);\n"
    "    float dentro = max(b - a, 0.0);\n"
    "    if (dentro <= 0.0 || celaDensidade <= 0.0) return cor;\n"
    "    vec3 meio = camera + d * (a + b) * 0.5;\n"
    // o fundo da cela (z menor, longe do vidro) brilha um pouco mais: ele vira silhueta contra a névoa
    "    float fundo = clamp((celaMax.z - meio.z) / max(celaMax.z - celaMin.z, 0.01), 0.0, 1.0);\n"
    "    vec3 nevoa_cor = celaCor * mix(0.35, 2.2, fundo) + luzNaNevoa(meio);\n"
    "    return mix(cor, nevoa_cor, 1.0 - exp(-celaDensidade * dentro));\n"
    "}\n"
    // cotangent frame (Schüler, 2013): a base da textura a partir das derivadas, sem tangentes na malha
    "vec3 normalDoMapa(vec3 n, vec3 p, vec2 uv) {\n"
    "    vec3 dp1 = dFdx(p), dp2 = dFdy(p);\n"
    "    vec2 duv1 = dFdx(uv), duv2 = dFdy(uv);\n"
    "    vec3 dp2perp = cross(dp2, n), dp1perp = cross(n, dp1);\n"
    "    vec3 t = dp2perp * duv1.x + dp1perp * duv2.x;\n"
    "    vec3 b = dp2perp * duv1.y + dp1perp * duv2.y;\n"
    "    float escala = inversesqrt(max(max(dot(t, t), dot(b, b)), 1e-12));\n"
    "    vec3 m = texture(mapaNormal, uv).xyz * 2.0 - 1.0;\n"
    "    return normalize(mat3(t * escala, b * escala, n) * m);\n"
    "}\n"
    // a sombra da lanterna: o mapa guarda a distância até ela (÷ ALCANCE_SOMBRA); 8 amostras num disco de
    // Poisson (PCF) deixam a borda suave
    "float sombraLanterna(vec3 p, vec3 luz) {\n"
    "    vec4 q = matSombra * vec4(p, 1.0);\n"
    "    if (q.w <= 0.0) return 1.0;\n"
    "    vec2 s = q.xy / q.w * 0.5 + 0.5;\n"
    "    if (s.x < 0.0 || s.x > 1.0 || s.y < 0.0 || s.y > 1.0) return 1.0;\n"
    "    float d = length(p - luz) / " ALCANCE_SOMBRA ";\n"
    "    vec2 texel = 1.0 / vec2(textureSize(mapaSombra, 0));\n"
    "    const vec2 disco[8] = vec2[](vec2(-0.613, 0.617), vec2(0.170, -0.040), vec2(-0.299, -0.791),\n"
    "        vec2(0.645, 0.493), vec2(-0.651, -0.181), vec2(0.421, -0.698), vec2(0.077, 0.888),\n"
    "        vec2(0.902, -0.142));\n"
    "    float acesa = 0.0;\n"
    "    for (int k = 0; k < 8; k++)\n"
    "        acesa += texture(mapaSombra, s + disco[k] * texel * 2.5).r < d - 0.003 ? 0.0 : 1.0;\n"
    "    return acesa / 8.0;\n"
    "}\n"
    "float ggx(float nh, float a) {\n"
    "    float a2 = a * a, d = nh * nh * (a2 - 1.0) + 1.0;\n"
    "    return a2 / (PI * d * d);\n"
    "}\n"
    "float smith(float nv, float nl, float r) {\n"
    "    float k = (r + 1.0) * (r + 1.0) / 8.0;\n"
    "    return (nv / (nv * (1.0 - k) + k)) * (nl / (nl * (1.0 - k) + k));\n"
    "}\n"
    "void main() {\n"
    // passe da sombra: só a distância até a lanterna (o mesmo shader, para os modelos entrarem sem troca)
    "    if (modoSombra > 0.5) {\n"
    "        finalColor = vec4(length(fragPosition - luzSombra) / " ALCANCE_SOMBRA ", 0.0, 0.0, 1.0);\n"
    "        return;\n"
    "    }\n"
    "    vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "    vec3 albedo = pow(base.rgb, vec3(2.2));\n" // as texturas são sRGB: a luz é somada em linear
    "    vec3 n = normalize(cross(dFdx(fragPosition), dFdy(fragPosition)));\n"
    "    vec3 paraCamera = camera - fragPosition;\n"
    "    if (dot(n, paraCamera) < 0.0) n = -n;\n"
    "    vec3 nGeo = n;\n" // a normal da face, antes do mapa (a água fica plana)
    "    float ao = 1.0, rug = rugosidade, met = metalico;\n"
    "    if (usaMapas > 0.5) {\n"
    "        vec3 orm = texture(mapaOrm, fragTexCoord).rgb;\n"
    "        ao = orm.r; rug = orm.g; met = orm.b;\n"
    "        n = normalDoMapa(n, fragPosition, fragTexCoord);\n"
    "    }\n"
    // o chão molhado: manchas grandes de água (em metros, no mundo: não repetem com a textura) e a água
    // parada no rejunte (onde a oclusão é baixa). A água escurece o piso, deixa liso e aplaina a normal.
    "    if (agua > 0.0) {\n"
    "        float manchas = smoothstep(0.36, 0.52, fbm(fragPosition.xz * 0.42));\n"
    "        float molha = agua * max(max(manchas, 0.3), 0.6 * smoothstep(0.85, 0.55, ao));\n"
    "        albedo *= mix(1.0, 0.42, molha);\n"
    "        rug = mix(rug, 0.035, molha);\n"
    "        n = normalize(mix(n, nGeo, molha));\n"
    "    }\n"
    "    rug = clamp(mix(rug, 0.05, clamp(molhado, 0.0, 1.0)), 0.045, 1.0);\n"
    "    vec3 vista = normalize(paraCamera);\n"
    "    float nv = max(dot(n, vista), 1e-4);\n"
    "    vec3 f0 = mix(vec3(0.04), albedo, met);\n"
    "    float a = rug * rug;\n"
    "    vec3 direta = vec3(0.0);\n"
    "    for (int i = 0; i < " N_LUZES "; i++) {\n"
    "        vec3 l = luzPos[i] - fragPosition;\n"
    "        float d = length(l);\n"
    "        vec3 dir = l / max(d, 0.001);\n"
    "        if (!alcanca(luzId[i], fragPosition + nGeo * 0.1)) continue;\n"
    "        float atenua = 1.0 / (1.0 + luzQueda[i] * (0.09 * d + 0.07 * d * d));\n"
    // longe demais: nem entra na conta
    "        if (atenua * max(luzCor[i].r, max(luzCor[i].g, luzCor[i].b)) < 0.004) continue;\n"
    "        vec3 radiancia = luzCor[i] * foco(luzCone[i], dir) * atenua;\n"
    "        if (i == indiceLanterna && usaSombra > 0.5)\n"
    "            radiancia *= sombraLanterna(fragPosition + nGeo * 0.02, luzPos[i]);\n"
    "        float nl = dot(n, dir);\n"
    "        vec3 h = normalize(dir + vista);\n"
    "        vec3 f = f0 + (1.0 - f0) * pow(1.0 - max(dot(h, vista), 0.0), 5.0);\n"
    "        float nlp = max(nl, 0.0);\n"
    "        vec3 especular = ggx(max(dot(n, h), 0.0), a) * smith(nv, nlp, rug) * f /\n"
    "                         (4.0 * nv * max(nlp, 1e-4) + 1e-4);\n"
    "        vec3 difusa = (1.0 - f) * (1.0 - met) * albedo * (nlp * 0.85 + 0.15);\n"
    "        direta += (difusa + especular * PI * nlp) * radiancia;\n"
    "    }\n"
    "    float borda = 1.0 - clamp(dot(n, vista), 0.0, 1.0);\n"
    "    vec3 cor = albedo * ambiente * ao + direta * mix(1.0, ao, 0.6) + contraluz * pow(borda, 2.5);\n"
    // reflexo planar (só no chão): a cena espelhada, lida na posição de tela virada de cabeça para baixo,
    // entortada pela normal, borrada pela rugosidade (mipmaps) e pesada pelo Fresnel (Schlick com rugosidade)
    "    if (reflexo > 0.0) {\n"
    "        vec2 uv = gl_FragCoord.xy / tela;\n"
    "        uv.y = 1.0 - uv.y;\n"
    "        uv += (n.xz - nGeo.xz) * 0.08;\n"
    "        vec3 espelho = textureLod(mapaReflexo, uv, rug * 7.0).rgb;\n"
    "        vec3 fr = f0 + (max(vec3(1.0 - rug), f0) - f0) * pow(1.0 - nv, 5.0);\n"
    "        cor += espelho * fr * reflexo * ao * (1.0 - rug) * (1.0 - rug);\n" // fosco: quase não espelha
    "    }\n"
    "    cor = nevoaDaCela(cor, fragPosition);\n"
    "    float fator = clamp(1.0 - exp(-nevoa * length(paraCamera)), 0.0, 1.0);\n"
    "    cor = mix(cor, vec3(0.0008, 0.0012, 0.001), fator);\n"
    // saída direta (o rosto do susto, ou sem alvo HDR): o tone mapping e a volta para sRGB aqui mesmo
    "    if (direto > 0.5) {\n"
    "        vec3 x = cor * exposicao;\n"
    "        x = clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);\n"
    "        cor = pow(x, vec3(1.0 / 2.2));\n"
    "    }\n"
    "    finalColor = vec4(cor, base.a);\n"
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
    "uniform vec3 contraluz; uniform float molhado; uniform float relevo;\n"
    "float foco(vec4 cone, vec3 paraLuz) {\n"
    "    if (cone.w < -1.0) return 1.0;\n"
    "    return mix(0.15, 1.0, smoothstep(cone.w, cone.w + 0.06, dot(-paraLuz, cone.xyz)));\n"
    "}\n"
    "uniform vec3 celaMin; uniform vec3 celaMax; uniform float celaDensidade; uniform vec3 celaCor;\n"
    "uniform float celaEspalha; uniform float celaEspalhaLanterna; uniform int indiceLanterna;\n"
    "vec3 luzNaNevoa(vec3 p) {\n"
    "    vec3 soma = vec3(0.0);\n"
    "    for (int i = 0; i < " N_LUZES "; i++) {\n"
    "        vec3 l = luzPos[i] - p;\n"
    "        float d = length(l);\n"
    "        float atenua = 1.0 / (1.0 + luzQueda[i] * (0.09 * d + 0.07 * d * d));\n"
    "        bool dentro = all(greaterThanEqual(luzPos[i], celaMin)) &&\n"
    "                      all(lessThanEqual(luzPos[i], celaMax));\n"
    // a névoa devolve as luzes de dentro da cela (holofotes e flash) e, bem menos, a lanterna
    "        float peso = i == indiceLanterna ? celaEspalhaLanterna : (dentro ? celaEspalha : 0.0);\n"
    "        soma += luzCor[i] * atenua * foco(luzCone[i], l / max(d, 0.001)) * peso;\n"
    "    }\n"
    "    return soma;\n"
    "}\n"
    "vec3 nevoaDaCela(vec3 cor, vec3 ponto) {\n"
    "    vec3 raio = ponto - camera;\n"
    "    float dist = length(raio);\n"
    "    vec3 d = raio / max(dist, 0.0001);\n"
    "    vec3 dd = mix(d, vec3(0.00001), vec3(lessThan(abs(d), vec3(0.00001))));\n"
    "    vec3 t0 = (celaMin - camera) / dd, t1 = (celaMax - camera) / dd;\n"
    "    vec3 tmin = min(t0, t1), tmax = max(t0, t1);\n"
    "    float a = max(max(max(tmin.x, tmin.y), tmin.z), 0.0);\n"
    "    float b = min(min(min(tmax.x, tmax.y), tmax.z), dist);\n"
    "    float dentro = max(b - a, 0.0);\n"
    "    if (dentro <= 0.0 || celaDensidade <= 0.0) return cor;\n"
    "    vec3 meio = camera + d * (a + b) * 0.5;\n"
    // o fundo da cela (z menor, longe do vidro) brilha um pouco mais: ele vira silhueta contra a névoa
    "    float fundo = clamp((celaMax.z - meio.z) / max(celaMax.z - celaMin.z, 0.01), 0.0, 1.0);\n"
    "    vec3 nevoa_cor = celaCor * mix(0.35, 2.2, fundo) + luzNaNevoa(meio);\n"
    "    return mix(cor, nevoa_cor, 1.0 - exp(-celaDensidade * dentro));\n"
    "}\n"
    "void main() {\n"
    "    vec4 base = texture2D(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "    vec3 luz = vec3(ambiente), brilho = vec3(0.0);\n"
    "    vec3 vista = normalize(camera - fragPosition);\n"
    "    for (int i = 0; i < " N_LUZES "; i++) {\n"
    "        vec3 l = luzPos[i] - fragPosition;\n"
    "        float d = length(l);\n"
    "        vec4 cone = luzCone[i];\n"
    "        float foco = cone.w < -1.0 ? 1.0\n"
    "            : mix(0.15, 1.0, smoothstep(cone.w, cone.w + 0.06, dot(-l / max(d, 0.001), cone.xyz)));\n"
    "        float atenua = foco / (1.0 + luzQueda[i] * (0.09 * d + 0.07 * d * d));\n"
    "        luz += luzCor[i] * atenua;\n"
    // sem normal na versão 100: o molhado só existe no chão, então a normal é para cima
    "        vec3 meio = normalize(l / max(d, 0.001) + vista);\n"
    "        if (molhado > 0.0) brilho += luzCor[i] * atenua * pow(max(meio.y, 0.0), " BRILHO_AGUA ");\n"
    "    }\n"
    // contraluz: sem normal na versão 100, um brilho fraco por igual
    "    vec3 agua = (brilho + luz * (0.03 + 0.97 * pow(1.0 - max(vista.y, 0.0), 5.0)) * 0.3) * molhado;\n"
    "    vec3 cor = nevoaDaCela(base.rgb * luz + agua + contraluz * 0.2, fragPosition);\n"
    "    float fator = clamp(1.0 - exp(-nevoa * length(camera - fragPosition)), 0.0, 1.0);\n"
    "    gl_FragColor = vec4(mix(cor, vec3(0.012, 0.016, 0.013), fator), base.a);\n"
    "}\n";

// Tone mapping: a cena HDR (linear) × exposição, a curva filmica ACES (Narkowicz, 2015) e a volta para sRGB.
static const char *TOM_FS_330 =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform float exposicao;\n"
    "out vec4 finalColor;\n"
    "vec3 aces(vec3 x) {\n"
    "    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);\n"
    "}\n"
    "void main() {\n"
    "    vec3 hdr = texture(texture0, fragTexCoord).rgb * exposicao;\n"
    "    finalColor = vec4(pow(aces(hdr), vec3(1.0 / 2.2)), 1.0);\n"
    "}\n";

// Emissivo: a cor do vértice (sRGB) em linear × intensidade. Os painéis das luminárias passam de 1 na cena
// HDR: estouram no bloom e aparecem fortes no reflexo do chão.
static const char *EMISSIVO_FS_330 =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse; uniform float intensidade;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec4 c = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "    finalColor = vec4(pow(c.rgb, vec3(2.2)) * intensidade, c.a);\n"
    "}\n";

const char *sombreador_emissivo_fs(void) {
    return EMISSIVO_FS_330;
}

const char *sombreador_luz_vs(bool v100) {
    return v100 ? LUZ_VS_100 : LUZ_VS_330;
}

const char *sombreador_luz_fs(bool v100) {
    return v100 ? LUZ_FS_100 : LUZ_FS_330;
}

const char *sombreador_tom_fs(void) {
    return TOM_FS_330;
}
