// texturas.h — as texturas do mapa: geradas UMA vez (procedurais, com override opcional em
// assets/texturas/), liberadas ao sair, e as primitivas que desenham com elas (caixa, quadro e halo).
#ifndef TEXTURAS_H
#define TEXTURAS_H

#include "estado.h"

// Como a UV de uma caixa é calculada.
typedef struct {
    Vector2 metros;  // metros por repetição (u, v); ignorado em `local`
    Vector2 desloca; // soma à UV de mundo: variação entre peças iguais
    bool local;      // true: cada face vai de 0 a 1 (porta); false: UV de mundo (paredes, chão, teto)
} ModoUV;

// Chame depois de ui_carrega_fonte (as placas usam a fonte) e antes do primeiro quadro.
void texturas_carrega(Recursos *rec);
void texturas_descarrega(Recursos *rec);

// O modo de UV de cada material (metros da config.h); `variacao` desloca a UV (ex.: o índice da parede).
ModoUV texturas_modo(TipoMaterial m, int variacao);

// Caixa com textura nas 6 faces (mesma ordem de vértices de DrawCubeV). Sem textura, cai em DrawCubeV.
// Desenhe fora de rlPushMatrix: as coordenadas já são de mundo.
void texturas_caixa(Texture2D textura, Vector3 centro, Vector3 tamanho, ModoUV uv, Color tinta);
// Quadro com textura (decalque): `direita` e `cima` são os semieixos inteiros (a normal é direita × cima).
void texturas_quadro(Texture2D textura, Vector3 centro, Vector3 direita, Vector3 cima, Color tinta);

#endif
