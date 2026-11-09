// estado.h — struct Estado: todo o estado do jogo mora aqui.
// Regra de inclusão: raylib.h vem SEMPRE antes de raymath.h (a raylib define Vector3 sem proteção).
// Por isso todo módulo impuro inclui este arquivo primeiro.
#ifndef ESTADO_H
#define ESTADO_H

#include "raylib.h"
#include "config.h"
#include "logica.h"
#include "regras.h"
#include "colecoes.h"

// ------------------------------------------------------------------ entrada do quadro
// main.c lê teclado e mouse e preenche isto; os módulos só leem daqui.
// No modo --smoke, quem preenche é o piloto automático.
typedef struct {
    Vector2 mover; // x: lado (A/D), y: frente (W/S)
    Vector2 mouse; // deslocamento do mouse no quadro
    bool correr;
    bool interagir;
    bool voltar;
    bool confirmar;
    bool cima, baixo, esquerda, direita;
    bool tecla1, tecla2, tecla3;
    bool f1, f2, f3, f5, f9, f12;
    bool alguma; // qualquer tecla ou movimento (modo feira)
} Entrada;

// ------------------------------------------------------------------ recursos carregados uma vez
#define SOM_MAXIMO 24

typedef struct {
    Font fonte;
    bool tem_fonte; // false = fonte padrão da raylib + translitera()
    Shader vhs;
    bool vhs_ok;
    int loc_vhs_tempo, loc_vhs_resolucao, loc_vhs_ruido, loc_vhs_aberracao;
    int loc_vhs_scanlines, loc_vhs_vinheta, loc_vhs_wobble;
    Shader luz;
    bool luz_ok;
    int loc_luz_pos, loc_luz_cor, loc_luz_ambiente, loc_luz_nevoa, loc_luz_camera, loc_luz_modelo;
    RenderTexture2D alvo; // cena 3D renderizada a ESCALA_RENDER da janela
    int alvo_largura, alvo_altura;
    Sound sons[SOM_MAXIMO];
    bool audio_ok;
    Model modelo_precursor; // opcional: assets/modelos/precursor.glb
    bool tem_modelo;
} Recursos;

// ------------------------------------------------------------------ nível (gerado do mapa ASCII)
#define MAPA_LARGURA 24
#define MAPA_ALTURA 14
#define MAX_ARMARIOS 8

typedef struct {
    char celulas[MAPA_ALTURA][MAPA_LARGURA + 1];
    Vector3 inicio, console, cela_precursor, duto, painel1, painel2, diario, porta_sala;
    Vector3 lampadas[MAX_LAMPADAS];
    int n_lampadas;
    Vector3 armarios[MAX_ARMARIOS];      // centro do armário
    Vector3 armario_saida[MAX_ARMARIOS]; // onde o jogador fica ao sair
    int n_armarios;
    float vidro_z;                       // plano do vidro da contenção
    float vidro_x0, vidro_x1;
} Nivel;

// ------------------------------------------------------------------ jogador
typedef enum { MODO_ANDANDO, MODO_TERMINAL, MODO_PAINEL, MODO_DIARIO, MODO_ARMARIO } ModoJogador;

typedef struct {
    Vector3 pos; // pés
    float yaw, pitch;
    float folego;
    bool exausto;
    bool correndo;
    ModoJogador modo;
    int armario; // índice em Nivel.armarios quando escondido
    int mira;    // índice do interativo na mira (−1 = nenhum)
    float tempo_passo;
    float balanco;       // balanço da câmera ao andar
    int ultima_celula;   // para notar quando ele cruza uma porta
} Jogador;

// ------------------------------------------------------------------ o Precursor
typedef struct {
    Vector3 pos;
    float yaw;
    bool ativo;         // nos corredores (saídas)
    bool contido;       // na cela (fases de sala)
    EstadoCriatura estado;
    float tempo_estado;
    float tempo_para_surgir;
    Vector3 destino;
    Vector3 ultima_vista;
    int caminho[MAX_CAMINHO]; // células (linha * MAPA_LARGURA + coluna)
    int caminho_n, caminho_i;
    float tempo_replanejar;
    int patrulha_i;
    bool ve_jogador;
    float distorcao;    // 0..1 ao ser "puxado" pela contenção
    float passo_t;
    float fala_t;       // > 0 enquanto ele "fala" (fenda aberta)
    float avanco_vidro; // 0..1: o passo em direção ao vidro depois da R1
    bool mao_no_vidro;
    float congelado_t;  // final bom
} Criatura;

// ------------------------------------------------------------------ rodadas (sala de observação)
typedef enum { ETAPA_DECIFRAR, ETAPA_ESCOLHER_CONECTIVO, ETAPA_TRADUZIR, ETAPA_CONCLUIDA } EtapaRodada;

typedef struct {
    int numero; // 1..3
    EtapaRodada etapa;
    bool luz_a, luz_b;
    bool linha_vista[2][4];
    bool linha_valor[2][4];
    int coluna;   // na R2 há duas frases: o jogador identifica uma de cada vez
    int cursor;
    float resposta_t;   // > 0 enquanto o Precursor "responde" à combinação de luzes
    int resposta_linha;
    bool resposta_valor;
    float aviso_t;
    const char *aviso;
    float concluida_t;
    bool rompendo;      // 3º erro: jumpscare e recomeço
} Rodada;

// ------------------------------------------------------------------ painéis de circuito (saídas)
typedef struct {
    int numero; // 1 ou 2
    bool chaves[3];
    int escolha[2]; // painel 2: TipoPorta escolhida em cada encaixe
    int cursor;
    bool resolvido;
    float resolvido_t;
} Painel;

// ------------------------------------------------------------------ resposta final
typedef struct {
    Ficha fichas[MAX_FICHAS_FRASE];
    int n;
    int cursor;
    const char *mensagem;
    char detalhe[128];
    float mensagem_t;
    bool transmitindo;
    float transmitindo_t;
    EventoFase resultado;
} Compositor;

// ------------------------------------------------------------------ intro, legendas e fim
typedef enum { FALANTE_VARELA, FALANTE_ELIAS, FALANTE_SISTEMA } Falante;

typedef struct {
    const char *texto;
    float duracao;
    Falante falante;
} Legenda;

typedef struct {
    Legenda fila[MAX_LEGENDAS];
    int n;
    float tempo; // tempo da legenda da frente
} FilaLegendas;

typedef struct {
    int linha;
    float letras; // quantas letras da linha atual já apareceram
    float espera;
    bool interfone; // terminou o texto datilografado; agora são as falas do Varela
} Intro;

typedef struct {
    float tempo;
    int etapa;
    bool tela_final; // a cinemática acabou: mostra o placar
} Fim;

typedef struct {
    int cursor;
} Menu;

// ------------------------------------------------------------------ o estado inteiro
typedef struct Estado {
    Fase fase;
    float tempo_fase;
    float tempo_partida;
    float tempo_total;
    EventoFase evento; // pedido de mudança de fase feito neste quadro (aplicado no fim do atualiza)
    bool sair, pausado, vhs_ligado, debug_criatura, pedir_captura;
    bool modo_smoke, modo_feira;
    float tempo_inativo;

    Entrada in;
    Recursos rec;
    Camera3D camera;
    Nivel nivel;
    Jogador jog;
    Criatura cri;
    Rodada rod;
    Painel painel;
    Compositor comp;
    Intro intro;
    FilaLegendas legendas;
    Fim fim;
    Menu menu;

    // As seis coleções do jogo
    ColRuidos ruidos;
    ColParticulas particulas;
    ColRachaduras rachaduras;
    ColEventos eventos;
    ColInterativos interativos;
    ColParedes paredes;

    bool conectivo_decifrado[4]; // NÃO, E, OU, SE…ENTÃO
    bool diario_lido;
    int vidas;          // cache do fold das rachaduras
    int placar;         // cache do fold dos eventos
    int erros_na_fase, erros_totais;
    int pegos_na_fase;
    float apagao_t;     // > 0 durante um apagão
    float falha_luz_t;  // luzes piscando depois de um erro
    float alarme_t;
    float jumpscare_t;  // > 0 durante o jumpscare
    float silencio_t;   // os 0,2 s de silêncio antes do estrondo
    float retorno_t;    // saída: tempo desde que o painel foi resolvido
} Estado;

#endif
