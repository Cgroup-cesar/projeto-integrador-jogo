// logica.h — motor de lógica proposicional (puro: sem raylib.h, sem globais).
#ifndef LOGICA_H
#define LOGICA_H

#include <stdbool.h>
#include "config.h"

typedef enum { OP_ATOMO, OP_NAO, OP_E, OP_OU, OP_IMPLICA, OP_SSE } OpLogico;

// Um nó da árvore da expressão. Os filhos são índices dentro do pool (−1 = sem filho).
typedef struct {
    OpLogico op;
    int atomo; // usado quando op == OP_ATOMO (0 = P/A, 1 = Q/B, 2 = C)
    int esq;
    int dir;
} NoLogico;

// Pool estático: os nós moram num vetor de tamanho fixo, sem malloc.
typedef struct {
    NoLogico nos[MAX_NOS_LOGICA];
    int usados;
} PoolLogico;

void logica_pool_limpa(PoolLogico *pool);

#endif
