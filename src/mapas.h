// mapas.h — os mapas PBR de cada material do mapa, gerados em C a partir das funções puras de materiais.c:
// cor (albedo), normal (tirada da altura) e ORM (R = oclusão das cavidades, G = rugosidade, B = metal).
// Só roda no carregamento; quem sobe para a GPU e libera é texturas.c.
#ifndef MAPAS_H
#define MAPAS_H

#include "estado.h"

typedef struct {
    Image cor, normal, orm;
} MapasMaterial;

// Gera os três mapas do material (largura e altura em pixels: potências de 2). As bordas emendam: a normal e
// a oclusão leem os vizinhos dando a volta na textura.
MapasMaterial mapas_gera(TipoMaterial m, int largura, int altura);

// Só a normal e o ORM de uma superfície lisa (quando a cor vem de assets/texturas/ sem _normal nem _orm).
MapasMaterial mapas_lisos(float rugosidade);

// Quantos metros a textura cobre na horizontal (a força do relevo sai na mesma escala em todo material).
float mapas_metros(TipoMaterial m);

#endif
