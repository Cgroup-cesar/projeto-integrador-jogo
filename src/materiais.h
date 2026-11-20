// materiais.h — o "desenho" das texturas procedurais como funções puras (sem raylib.h, sem I/O).
// Cada material devolve, por pixel, um brilho (multiplica a cor base) e uma mistura (0..1) com a cor
// secundária (umidade, tinta descascada, ferrugem...). Quem transforma isso em cor e em textura é texturas.c.
// Tudo é determinístico (mesma semente → mesma imagem) e contínuo nas bordas (a textura repete sem emenda).
#ifndef MATERIAIS_H
#define MATERIAIS_H

#include <stdbool.h>
#include "raymath.h"

// Número em [0, 1) a partir de um ponto inteiro da grade e da semente (sem rand()).
float materiais_hash(int x, int y, unsigned semente);
// Ruído de valor suave em [0, 1); repete a cada `periodo_x` × `periodo_y` unidades.
float materiais_ruido(float x, float y, int periodo_x, int periodo_y, unsigned semente);
// Soma de `oitavas` ruídos (cada um com o dobro da frequência) em [0, 1). u, v em [0, 1) cobrem a
// textura inteira, com `freq_x` × `freq_y` células na primeira oitava: o resultado repete em u e v.
float materiais_fbm(float u, float v, int freq_x, int freq_y, int oitavas, unsigned semente);

#endif
