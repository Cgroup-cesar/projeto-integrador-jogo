// menu_inicial.h — Menu inicial com visual VHS/CRT
#ifndef MENU_INICIAL_H
#define MENU_INICIAL_H

#include <raylib.h>
#include <stdbool.h>

// ================================ CONSTANTES AJUSTÁVEIS ================================

// Cores RGB do menu
#define MI_COR_FUNDO         (Color){2, 5, 4, 255}
#define MI_COR_VERDE_FOSFORO (Color){125, 255, 160, 255}
#define MI_COR_VERDE_APAGADO (Color){63, 154, 94, 255}
#define MI_COR_AZUL_DICA     (Color){74, 154, 212, 255}
#define MI_COR_AMBAR         (Color){240, 160, 48, 255}
#define MI_COR_OSD           (Color){232, 255, 240, 255}
#define MI_COR_VERMELHO      (Color){255, 32, 32, 255}

// Animações e timings (em segundos)
#define MI_ABERTURA_DURACAO      1.6f
#define MI_FECHAMENTO_DURACAO    0.5f
#define MI_CARREGANDO_DURACAO    2.34f   // 9 ciclos × 260ms
#define MI_CARREGANDO_CICLO      0.26f   // 260ms por ciclo de pontinhos
#define MI_SUBLIMINAR_MIN_INT    6.0f
#define MI_SUBLIMINAR_MAX_INT    15.0f
#define MI_SUBLIMINAR_MIN_DUR    0.07f
#define MI_SUBLIMINAR_MAX_DUR    0.16f
#define MI_REC_PISCAR            1.0f    // período do piscar do REC
#define MI_SETA_PISCAR           0.8f    // período do piscar da seta ▶

// Ciclos de animação do título (segundos)
#define MI_TITULO_CICLO_TREMIDA  3.4f
#define MI_TITULO_CICLO_GLITCH   4.6f
#define MI_TITULO_CICLO_ENERGIA  9.0f

// Layout proporcional (frações de W ou H)
#define MI_MARGEM_OSD_X          0.04f
#define MI_MARGEM_OSD_Y          0.04f
#define MI_TITULO_MARGEM_INF     0.09f   // margem abaixo do título até o menu
#define MI_MENU_GAP_V            0.006f  // gap entre itens do menu
#define MI_MENU_PADDING_V        0.005f  // padding vertical de cada item
#define MI_MENU_PADDING_H        0.022f  // padding horizontal de cada item
#define MI_DICA_MARGEM_SUP       0.05f   // margem acima da dica
#define MI_RODAPE_Y              0.055f  // posição do rodapé de controles (de baixo)
#define MI_RODAPE_TEXTO_Y        0.022f  // SP/data (de baixo)
#define MI_SETA_DIST_EM          1.4f    // distância da seta em "em"
#define MI_CARREGANDO_Y          0.14f   // posição do "CARREGANDO FITA" (de baixo)

// ================================ ESTRUTURAS ================================

typedef enum {
    MI_ITEM_INICIAR = 0,
    MI_ITEM_SAIR = 1,
    MI_ITEM_TOTAL = 2
} MIItem;

typedef enum {
    MI_ESTADO_NORMAL,
    MI_ESTADO_CARREGANDO,
    MI_ESTADO_INICIANDO,
    MI_ESTADO_FECHANDO
} MIEstado;

typedef struct {
    // Fontes
    Font fonteTitulo;     // Chakra Petch Bold
    Font fonteTexto;      // Share Tech Mono Regular

    // Renderização
    RenderTexture2D renderTarget;
    Shader shaderVHS;
    int locTime;          // uniform: time
    int locResolution;    // uniform: resolution
    int locTvOff;         // uniform: tvOffProgress (fechamento)
    int locBootAlpha;     // uniform: bootBlackAlpha (abertura)

    // Estado
    MIEstado estado;
    MIItem itemSelecionado;
    float tempo;          // tempo total desde init
    float tempoEstado;    // tempo no estado atual

    // Timecode
    int horas, minutos, segundos;

    // Animações de piscar
    float piscarRec;      // acumulador do REC
    float piscarSeta;     // acumulador da seta ▶

    // Carregando
    float cicloCarregando;
    int ciclosFeitos;

    // Subliminares
    float proximoSubliminar;
    float duracaoSubliminar;
    float tempoSubliminar;
    int indiceSubliminar;
    bool subliminarAtivo;
    Vector2 offsetSubliminar;

    // Abertura de TV
    float aberturaAlpha;  // alpha do preto da abertura

    // Fechamento de TV
    float progressoFechamento; // 0..1

    // Hitboxes dos itens do menu (para mouse)
    Rectangle hitboxItem[MI_ITEM_TOTAL];

    // Callbacks
    bool deveIniciar;
    bool deveSair;
} MenuInicial;

// ================================ FUNÇÕES PÚBLICAS ================================

void menu_inicial_init(MenuInicial *menu, int largura, int altura);
void menu_inicial_update(MenuInicial *menu, float dt);
void menu_inicial_draw(const MenuInicial *menu);
void menu_inicial_unload(MenuInicial *menu);
bool menu_inicial_deve_iniciar(const MenuInicial *menu);
bool menu_inicial_deve_sair(const MenuInicial *menu);
void menu_inicial_resize(MenuInicial *menu, int largura, int altura);

#endif // MENU_INICIAL_H
