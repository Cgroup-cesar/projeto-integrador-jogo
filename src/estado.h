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
typedef struct {
    Font fonte;
    bool tem_fonte; // false = fonte padrão da raylib + translitera()
    Shader vhs;
    bool vhs_ok;
    int loc_vhs_tempo, loc_vhs_resolucao, loc_vhs_ruido, loc_vhs_aberracao;
    int loc_vhs_scanlines, loc_vhs_vinheta, loc_vhs_wobble;
    RenderTexture2D alvo; // cena 3D renderizada a ESCALA_RENDER da janela
    int alvo_largura, alvo_altura;
} Recursos;

typedef struct Estado {
    float tempo_total; // tempo desde que o programa abriu
    bool sair;
    bool vhs_ligado;
    bool modo_smoke;
    bool modo_feira;
    Entrada in;
    Recursos rec;
} Estado;

#endif
