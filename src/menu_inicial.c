// menu_inicial.c — Menu inicial VHS/CRT completo.
// Desenha toda a cena em RenderTexture2D e aplica o shader VHS ao final.
// Todos os valores de layout são proporcionais a W (largura) e H (altura).
#include "menu_inicial.h"
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ================================ CONSTANTES INTERNAS ================================

// Frases subliminares (fácil de editar)
static const char *FRASES_SUBLIMINARES[] = {
    "NAO OLHE",
    "ELE ESTA AQUI",
    "ATRAS DE VOCE",
    "NAO FOI ACIDENTE",
    "PRECURSOR",
    "CORRA"
};
#define NUM_FRASES 6

// Gradiente de fundo (sala escura)
#define GRAD_TOPO    (Color){2, 6, 10, 255}
#define GRAD_MEIO    (Color){4, 16, 15, 255}
#define GRAD_BASE    (Color){2, 5, 4, 255}
#define GRAD_STOP2   0.55f

// Brilho radial verde
#define BRILHO_CX    0.78f
#define BRILHO_CY    0.70f
#define BRILHO_COR   (Color){30, 90, 60, 46}  // alpha 0.18 ≈ 46
#define BRILHO_RAIO  0.45f

// Janela na sala
#define JAN_X        0.30f
#define JAN_Y        0.14f
#define JAN_W        0.34f
#define JAN_H        0.62f
#define JAN_BORDA    (Color){80, 160, 140, 31}  // alpha 0.12
#define JAN_GRAD1    (Color){60, 140, 150, 26}   // alpha 0.10
#define JAN_GRAD2    (Color){10, 40, 40, 10}     // alpha 0.04

// Cama/mesa
#define CAMA_COR     (Color){11, 42, 28, 255}
#define CAMA_SOMBRA  (Color){5, 10, 8, 255}
#define CAMA_DIR     0.08f   // borda direita
#define CAMA_BASEY   0.14f   // base a 14% de H (do fundo)
#define CAMA_W       0.22f
#define CAMA_H       0.12f
#define CAMA_SH_OFFY 30.0f
#define CAMA_SH_EXP  8.0f

// Aberração cromática do título
#define ABR_R_DX      4.0f
#define ABR_C_DX     -3.0f
#define ABR_R_COR    (Color){255, 30, 50, 191}   // alpha 0.75
#define ABR_C_COR    (Color){40, 220, 255, 102}   // alpha 0.4

// Brilho do título (desenhado em camadas)
#define BRILHO_R1     4.0f
#define BRILHO_A1     0.6f
#define BRILHO_R2    18.0f
#define BRILHO_A2     0.2f

// OSD - aberração cromática
#define OSD_ABR_R    (Color){255, 40, 80, 255}
#define OSD_ABR_C    (Color){40, 220, 255, 255}

// Abertura de TV: 8 passos discretos
static const float ABERTURA_PASSOS[] = {1.0f, 0.4f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
#define NUM_PASSOS_ABERTURA 8

// ================================ FUNÇÕES AUXILIARES ================================

static inline float clampf(float v, float mn, float mx) {
    return v < mn ? mn : (v > mx ? mx : v);
}

// Tamanhos de fonte proporcionais
static inline float tam_titulo(float W) { return clampf(0.11f * W, 38.0f, 150.0f); }
static inline float tam_menu(float W)   { return clampf(0.03f * W, 20.0f, 34.0f); }
static inline float tam_dica(float W)   { return clampf(0.014f * W, 12.0f, 16.0f); }
static inline float tam_rodape(float W) { return clampf(0.012f * W, 10.0f, 14.0f); }
static inline float tam_osd(float W)    { return clampf(0.02f * W, 14.0f, 24.0f); }
static inline float tam_osd_peq(float W){ return clampf(0.018f * W, 14.0f, 22.0f); }
static inline float tam_carreg(float W) { return clampf(0.024f * W, 18.0f, 28.0f); }
static inline float tam_sublim(float W) { return clampf(0.10f * W, 34.0f, 140.0f); }

// Espaçamento em "em"
static inline float espac(float tam, float em) { return tam * em; }

// Interpolação de cor
static Color lerp_cor(Color a, Color b, float t) {
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

// Aleatório entre min e max
static float rand_range(float mn, float mx) {
    return mn + (float)rand() / (float)RAND_MAX * (mx - mn);
}

// Cor com alpha modificado
static inline Color cor_alpha(Color c, unsigned char a) {
    return (Color){c.r, c.g, c.b, a};
}

// Cor com alpha multiplicado por fator
static inline Color cor_alpha_mul(Color c, float mul) {
    return (Color){c.r, c.g, c.b, (unsigned char)(c.a * mul)};
}

// Mede texto e devolve posição centralizada em X
static Vector2 centro_texto(Font f, const char *t, float tam, float esp, float cx, float y) {
    Vector2 m = MeasureTextEx(f, t, tam, esp);
    return (Vector2){cx - m.x * 0.5f, y};
}

// Desenha texto com aberração cromática OSD (sombra vermelha +2px e ciano -2px)
static void texto_osd(Font f, const char *t, Vector2 p, float tam, float esp, Color cor) {
    Color r = OSD_ABR_R; r.a = cor.a;
    Color c = OSD_ABR_C; c.a = cor.a;
    DrawTextEx(f, t, (Vector2){p.x + 2, p.y}, tam, esp, r);
    DrawTextEx(f, t, (Vector2){p.x - 2, p.y}, tam, esp, c);
    DrawTextEx(f, t, p, tam, esp, cor);
}

// ================================ INICIALIZAÇÃO ================================

void menu_inicial_init(MenuInicial *menu, int largura, int altura) {
    memset(menu, 0, sizeof(MenuInicial));

    // Carrega fontes em tamanho alto com filtro bilinear
    menu->fonteTitulo = LoadFontEx("assets/fonts/ChakraPetch-Bold.ttf", 160, NULL, 0);
    SetTextureFilter(menu->fonteTitulo.texture, TEXTURE_FILTER_BILINEAR);

    menu->fonteTexto = LoadFontEx("assets/fonts/ShareTechMono-Regular.ttf", 160, NULL, 0);
    SetTextureFilter(menu->fonteTexto.texture, TEXTURE_FILTER_BILINEAR);

    // Render target
    menu->renderTarget = LoadRenderTexture(largura, altura);

    // Shader VHS
    menu->shaderVHS = LoadShader(NULL, "assets/shaders/vhs.fs");
    menu->locTime = GetShaderLocation(menu->shaderVHS, "time");
    menu->locResolution = GetShaderLocation(menu->shaderVHS, "resolution");
    menu->locTvOff = GetShaderLocation(menu->shaderVHS, "tvOffProgress");
    menu->locBootAlpha = GetShaderLocation(menu->shaderVHS, "bootBlackAlpha");

    float res[2] = {(float)largura, (float)altura};
    SetShaderValue(menu->shaderVHS, menu->locResolution, res, SHADER_UNIFORM_VEC2);

    // Estado inicial
    menu->estado = MI_ESTADO_NORMAL;
    menu->itemSelecionado = MI_ITEM_INICIAR;

    // Abertura de TV começa com tela preta
    menu->aberturaAlpha = 1.0f;

    // Primeira mensagem subliminar
    menu->proximoSubliminar = rand_range(MI_SUBLIMINAR_MIN_INT, MI_SUBLIMINAR_MAX_INT);

    TraceLog(LOG_INFO, "MENU: Menu VHS/CRT carregado");
}

void menu_inicial_unload(MenuInicial *menu) {
    UnloadFont(menu->fonteTitulo);
    UnloadFont(menu->fonteTexto);
    UnloadRenderTexture(menu->renderTarget);
    UnloadShader(menu->shaderVHS);
    TraceLog(LOG_INFO, "MENU: Menu VHS/CRT descarregado");
}

void menu_inicial_resize(MenuInicial *menu, int largura, int altura) {
    UnloadRenderTexture(menu->renderTarget);
    menu->renderTarget = LoadRenderTexture(largura, altura);

    float res[2] = {(float)largura, (float)altura};
    SetShaderValue(menu->shaderVHS, menu->locResolution, res, SHADER_UNIFORM_VEC2);
}

// ================================ ATUALIZAÇÃO ================================

static void atualizar_timecode(MenuInicial *menu) {
    int total = (int)menu->tempo;
    menu->horas = total / 3600;
    menu->minutos = (total % 3600) / 60;
    menu->segundos = total % 60;
}

static void atualizar_piscar(MenuInicial *menu, float dt) {
    menu->piscarRec += dt;
    if (menu->piscarRec >= MI_REC_PISCAR * 2.0f) menu->piscarRec -= MI_REC_PISCAR * 2.0f;

    menu->piscarSeta += dt;
    if (menu->piscarSeta >= MI_SETA_PISCAR * 2.0f) menu->piscarSeta -= MI_SETA_PISCAR * 2.0f;
}

static void atualizar_subliminares(MenuInicial *menu, float dt) {
    float W = (float)menu->renderTarget.texture.width;

    if (menu->subliminarAtivo) {
        menu->tempoSubliminar += dt;
        if (menu->tempoSubliminar >= menu->duracaoSubliminar) {
            menu->subliminarAtivo = false;
            menu->proximoSubliminar = rand_range(MI_SUBLIMINAR_MIN_INT, MI_SUBLIMINAR_MAX_INT);
        }
    } else {
        menu->proximoSubliminar -= dt;
        if (menu->proximoSubliminar <= 0.0f) {
            menu->subliminarAtivo = true;
            menu->tempoSubliminar = 0.0f;
            menu->duracaoSubliminar = rand_range(MI_SUBLIMINAR_MIN_DUR, MI_SUBLIMINAR_MAX_DUR);
            menu->indiceSubliminar = rand() % NUM_FRASES;

            // Offset aleatório: ±20% W horizontal, ±80% da altura do texto vertical
            menu->offsetSubliminar.x = rand_range(-0.20f * W, 0.20f * W);
            float tamT = tam_sublim(W);
            menu->offsetSubliminar.y = rand_range(-0.80f * tamT, 0.80f * tamT);
        }
    }
}

// Abertura de TV: 8 passos discretos em 1.6s
static void atualizar_abertura(MenuInicial *menu) {
    if (menu->tempo >= MI_ABERTURA_DURACAO) {
        menu->aberturaAlpha = 0.0f;
        return;
    }
    float prog = menu->tempo / MI_ABERTURA_DURACAO;
    int passo = (int)(prog * NUM_PASSOS_ABERTURA);
    if (passo >= NUM_PASSOS_ABERTURA) passo = NUM_PASSOS_ABERTURA - 1;
    menu->aberturaAlpha = ABERTURA_PASSOS[passo];
}

static void atualizar_entrada(MenuInicial *menu) {
    if (menu->estado != MI_ESTADO_NORMAL) return;

    // Navegação por teclado (com wrap)
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        menu->itemSelecionado = (menu->itemSelecionado + MI_ITEM_TOTAL - 1) % MI_ITEM_TOTAL;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        menu->itemSelecionado = (menu->itemSelecionado + 1) % MI_ITEM_TOTAL;
    }

    // Navegação por mouse (hover seleciona)
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < MI_ITEM_TOTAL; i++) {
        if (CheckCollisionPointRec(mouse, menu->hitboxItem[i])) {
            menu->itemSelecionado = i;
            // Clique confirma
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (i == MI_ITEM_INICIAR) {
                    menu->estado = MI_ESTADO_CARREGANDO;
                    menu->tempoEstado = 0.0f;
                    menu->cicloCarregando = 0.0f;
                    menu->ciclosFeitos = 0;
                } else {
                    menu->estado = MI_ESTADO_FECHANDO;
                    menu->tempoEstado = 0.0f;
                    menu->progressoFechamento = 0.0f;
                }
                return;
            }
            break;
        }
    }

    // Confirmação por teclado
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (menu->itemSelecionado == MI_ITEM_INICIAR) {
            menu->estado = MI_ESTADO_CARREGANDO;
            menu->tempoEstado = 0.0f;
            menu->cicloCarregando = 0.0f;
            menu->ciclosFeitos = 0;
        } else {
            menu->estado = MI_ESTADO_FECHANDO;
            menu->tempoEstado = 0.0f;
            menu->progressoFechamento = 0.0f;
        }
    }
}

void menu_inicial_update(MenuInicial *menu, float dt) {
    menu->tempo += dt;

    atualizar_timecode(menu);
    atualizar_piscar(menu, dt);
    atualizar_subliminares(menu, dt);
    atualizar_abertura(menu);

    // Passa uniforms ao shader
    SetShaderValue(menu->shaderVHS, menu->locTime, &menu->tempo, SHADER_UNIFORM_FLOAT);

    switch (menu->estado) {
        case MI_ESTADO_NORMAL:
            atualizar_entrada(menu);
            break;

        case MI_ESTADO_CARREGANDO:
            menu->tempoEstado += dt;
            menu->cicloCarregando += dt;
            // Conta ciclos completos de 260ms
            while (menu->cicloCarregando >= MI_CARREGANDO_CICLO) {
                menu->cicloCarregando -= MI_CARREGANDO_CICLO;
                menu->ciclosFeitos++;
            }
            if (menu->ciclosFeitos >= 9) {
                menu->estado = MI_ESTADO_INICIANDO;
                menu->deveIniciar = true;
            }
            break;

        case MI_ESTADO_INICIANDO:
            break;

        case MI_ESTADO_FECHANDO:
            menu->tempoEstado += dt;
            menu->progressoFechamento = menu->tempoEstado / MI_FECHAMENTO_DURACAO;
            if (menu->progressoFechamento > 1.0f) menu->progressoFechamento = 1.0f;
            if (menu->progressoFechamento >= 1.0f) {
                menu->deveSair = true;
            }
            break;
    }
}

bool menu_inicial_deve_iniciar(const MenuInicial *menu) { return menu->deveIniciar; }
bool menu_inicial_deve_sair(const MenuInicial *menu) { return menu->deveSair; }

// ================================ DESENHO - FUNDO ================================

// Gradiente vertical de 3 cores
static void desenhar_fundo(float W, float H) {
    for (int y = 0; y < (int)H; y++) {
        float t = (float)y / H;
        Color cor;
        if (t < GRAD_STOP2) {
            cor = lerp_cor(GRAD_TOPO, GRAD_MEIO, t / GRAD_STOP2);
        } else {
            cor = lerp_cor(GRAD_MEIO, GRAD_BASE, (t - GRAD_STOP2) / (1.0f - GRAD_STOP2));
        }
        DrawRectangle(0, y, (int)W, 1, cor);
    }
}

// Brilho radial verde-escuro
static void desenhar_brilho_radial(float W, float H) {
    float cx = W * BRILHO_CX;
    float cy = H * BRILHO_CY;
    float raio = H * BRILHO_RAIO;
    // Desenha círculos concêntricos do mais externo ao mais interno
    int passos = (int)(raio / 4.0f);
    if (passos < 1) passos = 1;
    for (int i = passos; i >= 0; i--) {
        float r = raio * (float)i / (float)passos;
        float fator = 1.0f - (float)i / (float)passos; // 0 no exterior, 1 no centro
        unsigned char a = (unsigned char)(fator * 46.0f); // alpha 0.18 ≈ 46
        DrawCircle((int)cx, (int)cy, r, (Color){30, 90, 60, a});
    }
}

// Janela na sala
static void desenhar_janela(float W, float H) {
    Rectangle jan = {W * JAN_X, H * JAN_Y, W * JAN_W, H * JAN_H};

    // Gradiente vertical interno
    for (int y = 0; y < (int)jan.height; y++) {
        float t = (float)y / jan.height;
        Color cor = lerp_cor(JAN_GRAD1, JAN_GRAD2, t);
        DrawRectangle((int)jan.x, (int)(jan.y + y), (int)jan.width, 1, cor);
    }

    // Borda
    DrawRectangleLinesEx(jan, 2.0f, JAN_BORDA);

    // Brilho suave ao redor
    Rectangle brilho = {jan.x - 6, jan.y - 6, jan.width + 12, jan.height + 12};
    DrawRectangleLinesEx(brilho, 4.0f, cor_alpha(JAN_BORDA, 31));
}

// Cama/mesa
static void desenhar_cama(float W, float H) {
    // Borda direita a 8% de W, base a 14% de H (do fundo = H - 14% H = 86% H)
    float direita = W * CAMA_DIR;
    float base = H * (1.0f - CAMA_BASEY);
    float larg = W * CAMA_W;
    float alt = H * CAMA_H;

    Rectangle cama = {direita, base - alt, larg, alt};

    // Sombra abaixo
    Rectangle sombra = {cama.x - CAMA_SH_EXP, cama.y + CAMA_SH_OFFY,
                        cama.width + CAMA_SH_EXP * 2, cama.height};
    DrawRectangleRec(sombra, CAMA_SOMBRA);

    // Cama
    DrawRectangleRec(cama, CAMA_COR);
}

// ================================ DESENHO - TÍTULO ================================

static void desenhar_titulo(const MenuInicial *menu, float W, float H, float centroY) {
    (void)H;
    const char *titulo = "PRECURSOR";
    float tam = tam_titulo(W);
    float esp = espac(tam, 0.08f);
    Vector2 med = MeasureTextEx(menu->fonteTitulo, titulo, tam, esp);
    Vector2 pos = {W * 0.5f - med.x * 0.5f, centroY};

    // ========== TREMIDA HORIZONTAL (ciclo 3.4s) ==========
    float ct = fmodf(menu->tempo / MI_TITULO_CICLO_TREMIDA, 1.0f);
    float offX = 0.0f;
    // 41-42%: x=-4, inclinação -3°; 43%: x=+5; 44%: volta
    if (ct >= 0.41f && ct < 0.42f) { offX = -4.0f; }
    else if (ct >= 0.42f && ct < 0.43f) { offX = 5.0f; }
    // Nota: a inclinação -3° não é suportada diretamente por DrawTextEx,
    // usamos apenas o deslocamento horizontal para efeito aproximado.
    pos.x += offX;

    // ========== QUEDA DE ENERGIA (ciclo 9s) ==========
    float ce = fmodf(menu->tempo / MI_TITULO_CICLO_ENERGIA, 1.0f);
    float alphaMul = 1.0f;
    if      (ce >= 0.61f && ce < 0.62f) alphaMul = 0.0f;
    else if (ce >= 0.62f && ce < 0.63f) alphaMul = 0.7f;
    else if (ce >= 0.63f && ce < 0.64f) alphaMul = 0.0f;
    else if (ce >= 0.89f && ce < 0.90f) alphaMul = 0.2f;

    // ========== BRILHO (texto desfocado ao redor) ==========
    // Camada externa (raio ~18px, alpha 0.2)
    unsigned char a2 = (unsigned char)(BRILHO_A2 * 255.0f * alphaMul);
    if (a2 > 0) {
        Color brilho2 = cor_alpha(MI_COR_VERDE_FOSFORO, a2);
        // 4 cópias deslocadas simulam desfoque
        DrawTextEx(menu->fonteTitulo, titulo, (Vector2){pos.x - 2, pos.y - 2}, tam, esp, brilho2);
        DrawTextEx(menu->fonteTitulo, titulo, (Vector2){pos.x + 2, pos.y + 2}, tam, esp, brilho2);
        DrawTextEx(menu->fonteTitulo, titulo, (Vector2){pos.x - 2, pos.y + 2}, tam, esp, brilho2);
        DrawTextEx(menu->fonteTitulo, titulo, (Vector2){pos.x + 2, pos.y - 2}, tam, esp, brilho2);
    }
    // Camada interna (raio ~4px, alpha 0.6)
    unsigned char a1 = (unsigned char)(BRILHO_A1 * 255.0f * alphaMul);
    if (a1 > 0) {
        Color brilho1 = cor_alpha(MI_COR_VERDE_FOSFORO, a1);
        DrawTextEx(menu->fonteTitulo, titulo, (Vector2){pos.x - 1, pos.y - 1}, tam, esp, brilho1);
        DrawTextEx(menu->fonteTitulo, titulo, (Vector2){pos.x + 1, pos.y + 1}, tam, esp, brilho1);
    }

    // ========== ABERRAÇÃO CROMÁTICA PERMANENTE ==========
    // Vermelha +4px
    Color abr_r = ABR_R_COR;
    abr_r.a = (unsigned char)(abr_r.a * alphaMul);
    DrawTextEx(menu->fonteTitulo, titulo,
               (Vector2){pos.x + ABR_R_DX, pos.y}, tam, esp, abr_r);

    // Ciano -3px
    Color abr_c = ABR_C_COR;
    abr_c.a = (unsigned char)(abr_c.a * alphaMul);
    DrawTextEx(menu->fonteTitulo, titulo,
               (Vector2){pos.x + ABR_C_DX, pos.y}, tam, esp, abr_c);

    // ========== GLITCH POR FATIAS (ciclo 4.6s, entre 41% e 44%) ==========
    float cg = fmodf(menu->tempo / MI_TITULO_CICLO_GLITCH, 1.0f);
    if (cg >= 0.41f && cg < 0.44f && alphaMul > 0.0f) {
        // Usa scissor test para recortar fatias do texto
        // Cópia vermelha: faixa 10%-40% deslocada -8px, faixa 55%-88% deslocada +6px
        // Cópia ciano: faixa 60%-92% deslocada +8px, faixa 18%-48% deslocada -6px
        float texTop = pos.y;
        float texH = med.y;
        Color glitchR = (Color){255, 30, 50, (unsigned char)(200 * alphaMul)};
        Color glitchC = (Color){40, 220, 255, (unsigned char)(180 * alphaMul)};

        // Habilita blend aditivo
        BeginBlendMode(BLEND_ADDITIVE);

        // Vermelha faixa 10%-40% deslocada -8px
        BeginScissorMode(0, (int)(texTop + texH * 0.10f), (int)W, (int)(texH * 0.30f));
        DrawTextEx(menu->fonteTitulo, titulo,
                   (Vector2){pos.x - 8, pos.y}, tam, esp, glitchR);
        EndScissorMode();

        // Vermelha faixa 55%-88% deslocada +6px
        BeginScissorMode(0, (int)(texTop + texH * 0.55f), (int)W, (int)(texH * 0.33f));
        DrawTextEx(menu->fonteTitulo, titulo,
                   (Vector2){pos.x + 6, pos.y}, tam, esp, glitchR);
        EndScissorMode();

        // Ciano faixa 60%-92% deslocada +8px
        BeginScissorMode(0, (int)(texTop + texH * 0.60f), (int)W, (int)(texH * 0.32f));
        DrawTextEx(menu->fonteTitulo, titulo,
                   (Vector2){pos.x + 8, pos.y}, tam, esp, glitchC);
        EndScissorMode();

        // Ciano faixa 18%-48% deslocada -6px
        BeginScissorMode(0, (int)(texTop + texH * 0.18f), (int)W, (int)(texH * 0.30f));
        DrawTextEx(menu->fonteTitulo, titulo,
                   (Vector2){pos.x - 6, pos.y}, tam, esp, glitchC);
        EndScissorMode();

        EndBlendMode();
    }

    // ========== TEXTO PRINCIPAL ==========
    Color principal = cor_alpha(MI_COR_VERDE_FOSFORO, (unsigned char)(255 * alphaMul));
    DrawTextEx(menu->fonteTitulo, titulo, pos, tam, esp, principal);
}

// ================================ DESENHO - MENU ================================

static void desenhar_menu(MenuInicial *menu, float W, float H, float centroY) {
    if (menu->estado == MI_ESTADO_CARREGANDO || menu->estado == MI_ESTADO_INICIANDO ||
        menu->estado == MI_ESTADO_FECHANDO) return;

    const char *itens[] = {"INICIAR", "SAIR"};
    float tam = tam_menu(W);
    float esp_val = espac(tam, 0.14f);
    float padV = H * MI_MENU_PADDING_V;
    float padH = W * MI_MENU_PADDING_H;
    float gap = H * MI_MENU_GAP_V;

    float y = centroY;
    for (int i = 0; i < MI_ITEM_TOTAL; i++) {
        Vector2 med = MeasureTextEx(menu->fonteTitulo, itens[i], tam, esp_val);
        float itemX = W * 0.5f - med.x * 0.5f;

        // Calcula hitbox para mouse
        menu->hitboxItem[i] = (Rectangle){
            itemX - padH, y - padV,
            med.x + padH * 2, med.y + padV * 2
        };

        bool sel = (menu->itemSelecionado == (MIItem)i);

        if (sel) {
            // Fundo com gradiente horizontal (verde com alpha decrescente)
            DrawRectangleGradientH(
                (int)(itemX - padH), (int)(y - padV),
                (int)(med.x + padH * 2), (int)(med.y + padV * 2),
                (Color){125, 255, 160, 41},   // alpha 0.16 ≈ 41
                (Color){125, 255, 160, 8}      // alpha 0.03 ≈ 8
            );

            // Barra verde na borda esquerda (3px)
            DrawRectangle((int)(itemX - padH), (int)(y - padV),
                          3, (int)(med.y + padV * 2), MI_COR_VERDE_FOSFORO);

            // Seta ▶ piscando (on/off seco a cada 0.8s)
            bool mostrarSeta = menu->piscarSeta < MI_SETA_PISCAR;
            if (mostrarSeta) {
                float setaX = itemX - tam * MI_SETA_DIST_EM;
                float setaY = y + med.y * 0.5f;
                float sz = tam * 0.25f;
                // Triângulo apontando para direita
                Vector2 v1 = {setaX, setaY - sz};
                Vector2 v2 = {setaX, setaY + sz};
                Vector2 v3 = {setaX + sz * 1.2f, setaY};
                DrawTriangle(v1, v2, v3, MI_COR_VERDE_FOSFORO);
            }

            // Texto com brilho (camada suave + texto cheio)
            Color brilho = cor_alpha(MI_COR_VERDE_FOSFORO, 100);
            DrawTextEx(menu->fonteTitulo, itens[i], (Vector2){itemX, y}, tam, esp_val, brilho);
            DrawTextEx(menu->fonteTitulo, itens[i], (Vector2){itemX, y}, tam, esp_val, MI_COR_VERDE_FOSFORO);
        } else {
            DrawTextEx(menu->fonteTitulo, itens[i], (Vector2){itemX, y}, tam, esp_val, MI_COR_VERDE_APAGADO);
        }

        y += med.y + padV * 2 + gap;
    }
}

// ================================ DESENHO - TEXTOS AUXILIARES ================================

// Dica: "Setas e ENTER para escolher"
static void desenhar_dica(const MenuInicial *menu, float W, float H, float y) {
    (void)H;
    if (menu->estado != MI_ESTADO_NORMAL) return;
    const char *t = "Setas e ENTER para escolher";
    float tam = tam_dica(W);
    float esp_val = espac(tam, 0.08f);
    Vector2 p = centro_texto(menu->fonteTexto, t, tam, esp_val, W * 0.5f, y);
    DrawTextEx(menu->fonteTexto, t, p, tam, esp_val, MI_COR_AZUL_DICA);
}

// Rodapé de controles
static void desenhar_rodape(const MenuInicial *menu, float W, float H) {
    const char *t = "WASD andar | mouse olhar | Shift correr | E interagir | Esc pausar";
    float tam = tam_rodape(W);
    float esp_val = espac(tam, 0.08f);
    float y = H * (1.0f - MI_RODAPE_Y);
    Vector2 p = centro_texto(menu->fonteTexto, t, tam, esp_val, W * 0.5f, y);
    DrawTextEx(menu->fonteTexto, t, p, tam, esp_val, MI_COR_VERDE_APAGADO);
}

// ================================ DESENHO - OSD ================================

static void desenhar_osd(const MenuInicial *menu, float W, float H) {
    float tam = tam_osd(W);
    float tamP = tam_osd_peq(W);
    float esp_val = espac(tam, 0.08f);
    float espP = espac(tamP, 0.08f);

    float mx = W * MI_MARGEM_OSD_X;
    float my = H * MI_MARGEM_OSD_Y;

    // ▶ PLAY (canto superior esquerdo)
    texto_osd(menu->fonteTexto, "> PLAY", (Vector2){mx, my}, tam, esp_val, MI_COR_OSD);

    // REC (canto superior direito)
    // Mede "REC" para alinhar à direita
    Vector2 medRec = MeasureTextEx(menu->fonteTexto, "REC", tam, esp_val);
    float recX = W - mx - medRec.x;

    // Quadrado vermelho piscando (a cada 1s, seco)
    bool mostrarQuad = menu->piscarRec < MI_REC_PISCAR;
    float quadTam = tam * 0.6f;
    float quadX = recX - quadTam - 8;
    float quadY = my + (tam - quadTam) * 0.5f;
    if (mostrarQuad) {
        DrawRectangle((int)quadX, (int)quadY, (int)quadTam, (int)quadTam, (Color){255, 59, 59, 255});
    }

    // "REC"
    texto_osd(menu->fonteTexto, "REC", (Vector2){recX, my}, tam, esp_val, MI_COR_OSD);

    // Timecode HH:MM:SS (embaixo do REC)
    char tc[16];
    snprintf(tc, sizeof(tc), "%02d:%02d:%02d", menu->horas, menu->minutos, menu->segundos);
    Vector2 medTC = MeasureTextEx(menu->fonteTexto, tc, tamP, espP);
    float tcX = W - mx - medTC.x;
    texto_osd(menu->fonteTexto, tc, (Vector2){tcX, my + tam + 4}, tamP, espP, MI_COR_OSD);

    // SP ●●● (canto inferior esquerdo)
    Color corSP = cor_alpha(MI_COR_OSD, (unsigned char)(0.85f * 255));
    float yInf = H * (1.0f - MI_RODAPE_TEXTO_Y) - tamP;
    texto_osd(menu->fonteTexto, "SP", (Vector2){mx, yInf}, tamP, espP, corSP);

    // Três círculos após "SP"
    Vector2 medSP = MeasureTextEx(menu->fonteTexto, "SP", tamP, espP);
    for (int i = 0; i < 3; i++) {
        DrawCircle((int)(mx + medSP.x + 10 + i * (tamP * 0.7f)),
                   (int)(yInf + tamP * 0.5f), tamP * 0.2f, corSP);
    }

    // NOV 17 2026 (canto inferior direito)
    const char *data = "NOV 17 2026";
    Vector2 medD = MeasureTextEx(menu->fonteTexto, data, tamP, espP);
    texto_osd(menu->fonteTexto, data,
              (Vector2){W - mx - medD.x, yInf}, tamP, espP, corSP);
}

// ================================ DESENHO - CARREGANDO ================================

static void desenhar_carregando(const MenuInicial *menu, float W, float H) {
    if (menu->estado == MI_ESTADO_CARREGANDO) {
        // "CARREGANDO FITA" com 0 a 3 pontinhos ciclando a cada 260ms
        int pontos = (int)(menu->cicloCarregando / (MI_CARREGANDO_CICLO / 4.0f));
        if (pontos > 3) pontos = 3;
        // Contamos o ciclo geral de 0-3 pontos
        pontos = menu->ciclosFeitos % 4;

        char txt[48];
        snprintf(txt, sizeof(txt), "CARREGANDO FITA%.*s", pontos, "...");

        float tam = tam_carreg(W);
        float esp_val = espac(tam, 0.1f);
        float y = H * (1.0f - MI_CARREGANDO_Y);
        Vector2 p = centro_texto(menu->fonteTexto, txt, tam, esp_val, W * 0.5f, y);
        DrawTextEx(menu->fonteTexto, txt, p, tam, esp_val, MI_COR_AMBAR);
    }
    else if (menu->estado == MI_ESTADO_INICIANDO) {
        const char *txt = "INICIANDO...";
        float tam = tam_carreg(W);
        float esp_val = espac(tam, 0.1f);
        float y = H * (1.0f - MI_CARREGANDO_Y);
        Vector2 p = centro_texto(menu->fonteTexto, txt, tam, esp_val, W * 0.5f, y);
        DrawTextEx(menu->fonteTexto, txt, p, tam, esp_val, MI_COR_AMBAR);
    }
}

// ================================ DESENHO - SUBLIMINARES ================================

static void desenhar_subliminar(const MenuInicial *menu, float W, float H) {
    if (!menu->subliminarAtivo) return;

    const char *frase = FRASES_SUBLIMINARES[menu->indiceSubliminar];
    float tam = tam_sublim(W);
    float esp_val = espac(tam, 0.06f);

    Vector2 med = MeasureTextEx(menu->fonteTitulo, frase, tam, esp_val);
    Vector2 pos = {
        W * 0.5f - med.x * 0.5f + menu->offsetSubliminar.x,
        H * 0.5f - med.y * 0.5f + menu->offsetSubliminar.y
    };

    // Blend aditivo para efeito subliminar
    BeginBlendMode(BLEND_ADDITIVE);

    // Cópia ciano deslocada +4px
    DrawTextEx(menu->fonteTitulo, frase,
               (Vector2){pos.x + 4, pos.y}, tam, esp_val,
               (Color){40, 220, 255, 115}); // alpha 0.45

    // Texto vermelho principal
    DrawTextEx(menu->fonteTitulo, frase, pos, tam, esp_val,
               (Color){255, 30, 30, 255});

    EndBlendMode();
}

// ================================ DESENHO PRINCIPAL ================================

void menu_inicial_draw(const MenuInicial *menu) {
    float W = (float)menu->renderTarget.texture.width;
    float H = (float)menu->renderTarget.texture.height;

    // ========== DESENHA TUDO NO RENDER TARGET ==========
    BeginTextureMode(menu->renderTarget);
    ClearBackground(MI_COR_FUNDO);

    // Cena de fundo (sala escura)
    desenhar_fundo(W, H);
    desenhar_brilho_radial(W, H);
    desenhar_janela(W, H);
    desenhar_cama(W, H);

    // Calcula layout vertical centralizado
    float tamT = tam_titulo(W);
    float tamM = tam_menu(W);
    float tamD = tam_dica(W);
    Vector2 medT = MeasureTextEx(menu->fonteTitulo, "PRECURSOR", tamT, espac(tamT, 0.08f));
    Vector2 medM0 = MeasureTextEx(menu->fonteTitulo, "INICIAR", tamM, espac(tamM, 0.14f));
    Vector2 medM1 = MeasureTextEx(menu->fonteTitulo, "SAIR", tamM, espac(tamM, 0.14f));
    float padV = H * MI_MENU_PADDING_V;
    float gap = H * MI_MENU_GAP_V;

    // Altura total do bloco: título + margem + 2 itens menu + margem dica + dica
    float altMenu = (medM0.y + padV * 2) + gap + (medM1.y + padV * 2);
    float altTotal = medT.y + H * MI_TITULO_MARGEM_INF + altMenu + H * MI_DICA_MARGEM_SUP + tamD;
    float yIni = H * 0.5f - altTotal * 0.5f;

    float yTitulo = yIni;
    float yMenu = yTitulo + medT.y + H * MI_TITULO_MARGEM_INF;
    float yDica = yMenu + altMenu + H * MI_DICA_MARGEM_SUP;

    desenhar_titulo(menu, W, H, yTitulo);
    // O cast para (MenuInicial *) é seguro: só modifica hitboxItem que é cache de layout
    desenhar_menu((MenuInicial *)menu, W, H, yMenu);
    desenhar_dica(menu, W, H, yDica);
    desenhar_rodape(menu, W, H);
    desenhar_osd(menu, W, H);
    desenhar_carregando(menu, W, H);
    desenhar_subliminar(menu, W, H);

    EndTextureMode();

    // ========== APLICA SHADER VHS E DESENHA NA TELA ==========
    // Passa uniforms de animação especial
    float bootAlpha = menu->aberturaAlpha;
    SetShaderValue(menu->shaderVHS, menu->locBootAlpha, &bootAlpha, SHADER_UNIFORM_FLOAT);

    float tvOff = (menu->estado == MI_ESTADO_FECHANDO) ? menu->progressoFechamento : 0.0f;
    SetShaderValue(menu->shaderVHS, menu->locTvOff, &tvOff, SHADER_UNIFORM_FLOAT);

    // Desenha a textura do render target com o shader aplicado
    BeginShaderMode(menu->shaderVHS);

    // A textura da RenderTexture precisa ser invertida em Y
    Rectangle src = {0, 0, (float)menu->renderTarget.texture.width,
                     -(float)menu->renderTarget.texture.height};
    Rectangle dst;

    if (tvOff > 0.0f) {
        // Animação de TV desligando: colapso vertical com brilho
        float escalaY = 1.0f - tvOff * (1.0f - 0.004f);
        float centroYTela = H * 0.5f;
        float novaAltura = H * escalaY;
        dst = (Rectangle){0, centroYTela - novaAltura * 0.5f, W, novaAltura};
    } else {
        dst = (Rectangle){0, 0, W, H};
    }

    DrawTexturePro(menu->renderTarget.texture, src, dst, (Vector2){0, 0}, 0, WHITE);
    EndShaderMode();

    // Se estiver fechando, preenche o resto com preto e aplica brilho
    if (tvOff > 0.0f) {
        float escalaY = 1.0f - tvOff * (1.0f - 0.004f);
        float centroYTela = H * 0.5f;
        float novaAltura = H * escalaY;
        // Barras pretas acima e abaixo
        DrawRectangle(0, 0, (int)W, (int)(centroYTela - novaAltura * 0.5f), BLACK);
        DrawRectangle(0, (int)(centroYTela + novaAltura * 0.5f), (int)W,
                      (int)(H - centroYTela - novaAltura * 0.5f + 1), BLACK);

        // Brilho branco sobre tudo (cresce com o progresso, até 4x ≈ branco)
        float brilho = tvOff * 3.0f;
        if (brilho > 1.0f) brilho = 1.0f;
        DrawRectangle(0, 0, (int)W, (int)H,
                      (Color){255, 255, 255, (unsigned char)(brilho * 255)});

        // Tela preta final
        if (tvOff >= 0.98f) {
            DrawRectangle(0, 0, (int)W, (int)H, BLACK);
        }
    }
}
