// colecoes.h — as coleções do jogo: struct por objeto, vetor de capacidade fixa e contador.
// [REQ:RF3] seis coleções de múltiplos objetos, sem malloc por quadro
// Este cabeçalho não inclui raylib.h (usa só os tipos do raymath.h), para as regras puras poderem usá-lo.
#ifndef COLECOES_H
#define COLECOES_H

#include <stdbool.h>
#include "config.h"
#include "raymath.h"

// ------------------------------------------------------------------ ruídos (passos, corrida, portas, painéis)
typedef enum { RUIDO_PASSO, RUIDO_CORRIDA, RUIDO_PORTA, RUIDO_INTERRUPTOR, RUIDO_PAINEL } TipoRuido;

typedef struct {
    Vector3 pos;
    float raio;        // até onde ele chega
    float intensidade; // 1 = acabou de acontecer; vai a 0 com o tempo
    TipoRuido tipo;
} Ruido;

typedef struct {
    Ruido itens[MAX_RUIDOS];
    int n;
} ColRuidos;

// ------------------------------------------------------------------ partículas (faíscas dos glifos, poeira, fragmentos)
typedef enum { PARTICULA_FAGULHA, PARTICULA_POEIRA, PARTICULA_FRAGMENTO } TipoParticula;

typedef struct {
    Vector3 pos;
    Vector3 vel;
    float vida;     // segundos restantes
    float vida_max;
    TipoParticula tipo;
} Particula;

typedef struct {
    Particula itens[MAX_PARTICULAS];
    int n;
} ColParticulas;

// ------------------------------------------------------------------ rachaduras no vidro (cada erro cria de 1 a 3)
typedef struct {
    Vector2 origem;         // em coordenadas da tela, de 0 a 1
    float angulo;           // radianos
    float comprimento;      // cresce até o alvo
    float comprimento_alvo; // em fração da altura da tela
    float dano;             // a soma dos danos de um erro é 1 vida
    int galhos;
    unsigned semente;
} Rachadura;

typedef struct {
    Rachadura itens[MAX_RACHADURAS];
    int n;
} ColRachaduras;

// ------------------------------------------------------------------ eventos do placar
typedef enum { PONTO_RODADA_SEM_ERRO, PONTO_ERRO, PONTO_SAIDA_LIMPA, PONTO_BONUS_TEMPO, PONTO_FINAL_BOM } TipoPonto;

typedef struct {
    TipoPonto tipo;
    int pontos;
    int fase; // um valor de Fase (regras.h)
} EventoPlacar;

typedef struct {
    EventoPlacar itens[MAX_EVENTOS];
    int n;
} ColEventos;

// ------------------------------------------------------------------ interativos (console, painéis, diário, armários, portas)
typedef enum { INTER_CONSOLE, INTER_PAINEL1, INTER_PAINEL2, INTER_DIARIO, INTER_ARMARIO, INTER_PORTA } TipoInterativo;

typedef struct {
    TipoInterativo tipo;
    Vector3 pos;          // centro da caixa usada na mira
    Vector3 meio_tamanho; // metade das dimensões da caixa
    Vector3 frente;       // de onde o jogador usa (armário: para onde ele sai)
} Interativo;

typedef struct {
    Interativo itens[MAX_INTERATIVOS];
    int n;
} ColInterativos;

// ------------------------------------------------------------------ paredes (geradas do mapa)
typedef enum {
    PAREDE_MURO,  // corrida de '#' do mapa
    PAREDE_VIDRO, // o vidro da contenção: sólido, mas transparente
    PAREDE_PORTA, // a porta trancada da sala de observação (só nas fases de sala)
    PAREDE_MOVEL  // console, painéis, mesa do diário, armários
} TipoParede;

typedef struct {
    Vector3 min;
    Vector3 max;
    TipoParede tipo;
} Parede;

typedef struct {
    Parede itens[MAX_PAREDES];
    int n;
} ColParedes;

// ------------------------------------------------------------------ operações (colecoes.c)
// Vetor cheio: descarta o novo item com TraceLog e devolve false, sem travar o jogo.
bool col_ruidos_adiciona(ColRuidos *c, Ruido r);
void col_ruidos_atualiza(ColRuidos *c, float dt);
int col_ruidos_mais_forte(const ColRuidos *c, Vector3 ouvinte, float *intensidade);

bool col_particulas_adiciona(ColParticulas *c, Particula p);
void col_particulas_atualiza(ColParticulas *c, float dt, Vector3 limite_min, Vector3 limite_max);

bool col_rachaduras_adiciona(ColRachaduras *c, Rachadura r);
void col_rachaduras_atualiza(ColRachaduras *c, float dt);

bool col_eventos_adiciona(ColEventos *c, EventoPlacar ev);

bool col_interativos_adiciona(ColInterativos *c, Interativo it);
void col_interativos_filtra_fase(ColInterativos *c, int fase);
int col_interativos_na_mira(const ColInterativos *c, Vector3 olho, Vector3 frente, float alcance);

bool col_paredes_adiciona(ColParedes *c, Parede p);
bool col_paredes_colide(const ColParedes *c, Vector3 centro, float raio, float altura);
float col_paredes_distancia_raio(const ColParedes *c, Vector3 origem, Vector3 direcao, float alcance);

#endif
