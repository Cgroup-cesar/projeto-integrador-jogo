// materiais.h — o "desenho" das texturas procedurais como funções puras (sem raylib.h, sem I/O).
// Cada material devolve, por pixel, um brilho (multiplica a cor base) e uma mistura (0..1) com a cor
// secundária (umidade, tinta descascada, ferrugem...). Quem transforma isso em cor e em textura é texturas.c.
// Tudo é determinístico (mesma semente → mesma imagem) e contínuo nas bordas (a textura repete sem emenda).
#ifndef MATERIAIS_H
#define MATERIAIS_H

#include <stdbool.h>
#include "raymath.h"

typedef enum {
    MATERIAL_CONCRETO, // parede de cima, mureta, verga: reboco pintado descascando (materiais_parede.c)
    MATERIAL_AZULEJO,  // a metade de baixo das paredes: azulejo esmaltado (materiais_parede.c)
    MATERIAL_PISO,     // placas de granilite
    MATERIAL_TETO,     // a laje pintada, descascando e manchada (materiais_parede.c)
    MATERIAL_METAL,    // metal pintado (móveis, painéis, armários): cinza neutro, tingido no desenho
    MATERIAL_PORTA,    // porta de metal com almofadas
    MATERIAL_TOTAL
} TipoMaterial;

// Decalques: placas de estêncil (texto de dialogo.c, desenhado com a fonte) e manchas procedurais.
typedef enum {
    DECALQUE_POSTO,    // "POSTO 7"
    DECALQUE_RESTRITA, // "ÁREA RESTRITA"
    DECALQUE_ARQUIVO,  // "ARQUIVO"
    DECALQUE_CAIXA_1,  // "EXÉRCITO BRASILEIRO" (a placa dos caixotes, caixas.c)
    DECALQUE_CAIXA_2,  // "CHÃ DE ALEGRIA - PE"
    DECALQUE_UMIDADE,  // mancha de umidade com escorridos (parede)
    DECALQUE_ARRASTO,  // marcas de arrasto saindo do duto (chão)
    DECALQUE_MOLHADO,   // chão molhado: borrão irregular, com ilhas secas no meio (chão)
    DECALQUE_ESCORRIDO, // água que escorreu: um filete torto que alarga e empoça no fim (chão)
    DECALQUE_RESPINGO,  // respingos: gotas soltas em volta de uma mancha pequena (chão)
    DECALQUE_POCA,      // poça d'água parada, de borda nítida (chão, embaixo da goteira)
    DECALQUE_POEIRA,    // poeira e sujeira acumuladas no pé da parede (chão; v = 0 encostado nela)
    DECALQUE_VIDRO,     // a sujeira do vidro da contenção: poeira embaixo, manchas, marcas de pano, digitais
    DECALQUE_SANGUE,   // respingo de sangue (assets/texturas/sangue.png; sem o arquivo, não aparece)
    DECALQUE_TOTAL
} Decalque;

#define DECALQUES_ESTENCIL 5 // os primeiros DECALQUES_ESTENCIL do enum são texto

// Um pixel de material PBR: a cor (brilho e misturas sobre a paleta do material) e a superfície.
typedef struct {
    float brilho;     // multiplica a cor base (≈1 = a cor da paleta)
    float mistura;    // 0 = cor base, 1 = cor secundária
    float tijolo;     // 0..1: o tijolo aparecendo por baixo (por cima das outras duas cores)
    float altura;     // 0..1: relevo (0 = fundo: rejunte, rachadura, buraco; 1 = a face da peça)
    float rugosidade; // 0..1: 0 = espelho (esmalte, água), 1 = fosco (reboco, poeira)
    float metal;      // 0..1: 1 = metal nu (a cor vira a do reflexo)
} PixelMaterial;

// Número em [0, 1) a partir de um ponto inteiro da grade e da semente (sem rand()).
float materiais_hash(int x, int y, unsigned semente);
// Ruído de valor suave em [0, 1); repete a cada `periodo_x` × `periodo_y` unidades.
float materiais_ruido(float x, float y, int periodo_x, int periodo_y, unsigned semente);
// Soma de `oitavas` ruídos (cada um com o dobro da frequência) em [0, 1). u, v em [0, 1) cobrem a
// textura inteira, com `freq_x` × `freq_y` células na primeira oitava: o resultado repete em u e v.
float materiais_fbm(float u, float v, int freq_x, int freq_y, int oitavas, unsigned semente);

// Auxiliares das paredes (materiais.c e materiais_parede.c): smoothstep; rachadura fina e ramificada (1 na
// linha); escorridos verticais que descem do topo (v = 0); Voronoi periódico (f1, f2 em células; id 0..1).
float materiais_passo(float a, float b, float x);
float materiais_rachadura(float u, float v, int fx, int fy, unsigned semente);
float materiais_escorridos(float u, float v, int colunas, unsigned semente);
void materiais_voronoi(float u, float v, int nx, int ny, unsigned semente, float *f1, float *f2, float *id);

// As paredes e o teto (materiais_parede.c): azulejo esmaltado, tinta descascando sobre reboco e a laje.
PixelMaterial materiais_azulejo(float u, float v, float sujeira, unsigned semente);
PixelMaterial materiais_tinta(float u, float v, float sujeira, unsigned semente);
PixelMaterial materiais_laje(float u, float v, float sujeira, unsigned semente);

// O pixel (u, v) ∈ [0, 1)² de cada material; `sujeira` (0..1) dosa manchas, escorridos e desgaste.
// Nas paredes (concreto e azulejo), v = 0 é o teto e v = 1 é o chão.
PixelMaterial materiais_pixel(TipoMaterial m, float u, float v, float sujeira, unsigned semente);

// Alfa (0..1) das manchas que viram decalque (DECALQUE_UMIDADE, DECALQUE_ARRASTO); some nas bordas.
// No arrasto, u segue o comprimento (u = 1 na boca do duto) e v a largura.
float materiais_mancha(Decalque d, float u, float v, unsigned semente);

// Desgaste da tinta de estêncil (as placas): 0 = tinta sumiu, 1 = tinta inteira.
float materiais_desgaste(float u, float v, unsigned semente);

// UV em coordenadas de mundo para uma face da caixa: `eixo` é o eixo da normal (0 = x, 1 = y, 2 = z).
// Nas faces verticais, u segue a horizontal e v desce a partir de `topo` (o teto); no chão e no teto,
// u = x e v = z. `metros` = metros por repetição; `desloca` soma à UV (variação entre paredes).
// A mesma posição do mundo dá sempre a mesma UV: paredes vizinhas emendam e nada estica.
Vector2 materiais_uv(Vector3 ponto, int eixo, Vector2 metros, Vector2 desloca, float topo);

#endif
