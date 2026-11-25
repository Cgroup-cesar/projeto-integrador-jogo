#version 330

// Shader VHS/CRT para o menu inicial do PRECURSOR
// Efeitos: ruído de TV, scanlines, barra de tracking, rasgos, vinheta, flicker, abertura/fechamento

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Uniforms controlados pelo menu
uniform vec2 resolution;
uniform float time;
uniform float tvOffProgress;    // 0..1: animação de TV desligando (brilho sobe)
uniform float bootBlackAlpha;   // 0..1: abertura de TV (preto sobre tudo)

out vec4 finalColor;

// ================================ CONSTANTES ================================

// Ruído de TV
const float NOISE_OPACITY = 0.22;
const vec2 NOISE_RES = vec2(160.0, 90.0);    // resolução baixa do ruído
const float NOISE_FPS = 12.0;                 // renova a ~12 FPS

// Scanlines
const float SCANLINE_OPACITY = 0.34;
const float SCANLINE_SPACING = 3.0;

// Barra de tracking
const float TRACKING_HEIGHT = 0.14;           // 14% da altura
const float TRACKING_DURATION = 7.0;          // 7s para descer

// Rasgos horizontais
const float TEAR_HEIGHT = 0.03;              // 3% de H

// Vinheta
const float VIGNETTE_START = 0.38;
const vec3 VIGNETTE_COLOR = vec3(0.07, 0.0, 0.0);  // vermelho-sangue escuro
const float VIGNETTE_OPACITY = 0.92;

// Flicker global (ciclo de 6s)
const float FLICKER_CYCLE = 6.0;

// ================================ FUNÇÕES ================================

// Gerador pseudo-aleatório
float hash(vec2 st) {
    return fract(sin(dot(st, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Ruído de TV em baixa resolução, tingido de verde, renovado a ~12 FPS
vec3 tvNoise(vec2 uv) {
    // Amostragem "nearest" por quantização das coordenadas
    vec2 noiseUV = floor(uv * NOISE_RES) / NOISE_RES;
    float t = floor(time * NOISE_FPS);
    float n = hash(noiseUV + vec2(t * 0.17, t * 0.31));

    // 50% dos pixels transparentes
    if (n < 0.5) return vec3(0.0);
    float v = (n - 0.5) * 2.0;

    // Tingido de verde: r=v*0.6, g=v, b=v*0.7
    return vec3(v * 0.6, v, v * 0.7);
}

// Scanlines horizontais: 1px escura a cada 3px
float scanlines(vec2 fragCoord) {
    return (mod(fragCoord.y, SCANLINE_SPACING) < 1.0) ? (1.0 - SCANLINE_OPACITY) : 1.0;
}

// Barra de tracking descendo de -20% até 115% em 7s (loop linear)
vec3 trackingBar(vec2 uv) {
    float progress = mod(time / TRACKING_DURATION, 1.0);
    float barY = mix(-0.20, 1.15, progress);
    float dist = abs(uv.y - barY);
    float influence = smoothstep(TRACKING_HEIGHT, 0.0, dist);

    // Gradiente: verde na borda, branco no centro
    vec3 barColor = mix(vec3(0.49, 1.0, 0.63), vec3(1.0), pow(influence, 3.0));
    // Alpha cresce de 0.07 nas bordas para 0.12 no centro
    float barAlpha = influence * mix(0.07, 0.12, pow(influence, 2.0));

    return barColor * barAlpha;
}

// Rasgos horizontais (tear): deslocam a imagem em X por 1 frame
vec2 tearOffset(vec2 uv) {
    float cycle = mod(time / 5.3, 1.0);

    // Rasgo (a): 31% do ciclo, y=62%, +14px
    if (abs(cycle - 0.31) < 0.005) {
        if (abs(uv.y - 0.62) < TEAR_HEIGHT * 0.5)
            return vec2(14.0 / resolution.x, 0.0);
    }
    // Rasgo (b): 32% do ciclo, y=64%, -10px
    else if (abs(cycle - 0.32) < 0.005) {
        if (abs(uv.y - 0.64) < TEAR_HEIGHT * 0.5)
            return vec2(-10.0 / resolution.x, 0.0);
    }
    // Rasgo (c): 71% do ciclo, y=22%, -18px
    else if (abs(cycle - 0.71) < 0.005) {
        if (abs(uv.y - 0.22) < TEAR_HEIGHT * 0.5)
            return vec2(-18.0 / resolution.x, 0.0);
    }

    return vec2(0.0);
}

// Overlay de cor nos rasgos: (125,255,160) alpha 0.08
vec4 tearOverlay(vec2 uv) {
    float cycle = mod(time / 5.3, 1.0);

    bool inTear = false;
    if (abs(cycle - 0.31) < 0.005 && abs(uv.y - 0.62) < TEAR_HEIGHT * 0.5) inTear = true;
    if (abs(cycle - 0.32) < 0.005 && abs(uv.y - 0.64) < TEAR_HEIGHT * 0.5) inTear = true;
    if (abs(cycle - 0.71) < 0.005 && abs(uv.y - 0.22) < TEAR_HEIGHT * 0.5) inTear = true;

    if (inTear) return vec4(0.49, 1.0, 0.63, 0.08);
    return vec4(0.0);
}

// Vinheta radial: transparente até 38% do raio, depois escurece para vermelho-sangue
float vignette(vec2 uv) {
    vec2 center = vec2(0.5);
    float dist = distance(uv, center);
    float maxDist = distance(vec2(0.0), center); // ~0.707
    float norm = dist / maxDist;

    if (norm < VIGNETTE_START) return 1.0;
    float amount = smoothstep(VIGNETTE_START, 1.0, norm);
    return 1.0 - amount * VIGNETTE_OPACITY;
}

// Flicker global: ciclo de 6s com quedas de brilho específicas
float flicker() {
    float cycle = mod(time / FLICKER_CYCLE, 1.0);

    if (cycle >= 0.93 && cycle < 0.94) return 0.86;
    if (cycle >= 0.96 && cycle < 0.97) return 0.92;

    return 1.0;
}

// ================================ MAIN ================================

void main() {
    vec2 uv = fragTexCoord;

    // Aplica deslocamento dos rasgos horizontais
    vec2 tear = tearOffset(uv);
    vec2 sampledUV = uv + tear;

    // Amostra a textura base
    vec4 tex = texture(texture0, sampledUV);

    // Ruído de TV (blend screen: result = a + b - a*b)
    vec3 noise = tvNoise(uv) * NOISE_OPACITY;
    tex.rgb = tex.rgb + noise - tex.rgb * noise;

    // Scanlines
    tex.rgb *= scanlines(gl_FragCoord.xy);

    // Barra de tracking (blend screen)
    vec3 tracking = trackingBar(uv);
    tex.rgb = tex.rgb + tracking - tex.rgb * tracking;

    // Overlay dos rasgos
    vec4 tearCol = tearOverlay(uv);
    if (tearCol.a > 0.0) {
        tex.rgb = tex.rgb + tearCol.rgb * tearCol.a - tex.rgb * tearCol.rgb * tearCol.a;
    }

    // Vinheta radial avermelhada
    float vig = vignette(uv);
    tex.rgb = mix(VIGNETTE_COLOR, tex.rgb, vig);

    // Flicker global
    tex.rgb *= flicker();

    // TV desligando: brilho sobe até 4x (satura em branco)
    if (tvOffProgress > 0.0) {
        float brightness = 1.0 + tvOffProgress * 3.0;
        tex.rgb *= brightness;
    }

    // Abertura de TV: preto por cima
    if (bootBlackAlpha > 0.0) {
        tex.rgb = mix(tex.rgb, vec3(0.0), bootBlackAlpha);
    }

    finalColor = tex * colDiffuse * fragColor;
}
