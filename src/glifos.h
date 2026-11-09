// glifos.h — a língua do Precursor: glifos de linhas numa grade 5×5 (coordenadas 0..4, y para baixo).
#ifndef GLIFOS_H
#define GLIFOS_H

#include "estado.h"

typedef enum {
    GLIFO_NAO,
    GLIFO_E,
    GLIFO_OU,
    GLIFO_SE_ENTAO,
    GLIFO_SSE,
    GLIFO_LUZ_A,
    GLIFO_LUZ_B,
    GLIFO_NOS,
    GLIFO_VOCE,
    GLIFO_PERIGO,
    GLIFO_FALAR,
    GLIFO_PORTA,
    GLIFO_ABRIR,
    GLIFO_TOTAL
} IdGlifo;

#define MAX_GLIFOS_FRASE 6

typedef struct {
    IdGlifo glifos[MAX_GLIFOS_FRASE];
    int n;
} FraseGlifos;

void desenha_glifo(IdGlifo id, Vector2 pos, float tamanho, Color cor, float espessura);
void desenha_frase_glifos(const Estado *e, const FraseGlifos *frase, Vector2 pos, float tamanho, float espessura);

// Conectivos ainda desconhecidos em âmbar; decifrados (e palavras) em verde-fósforo.
int glifo_conectivo(IdGlifo id); // 0 NÃO, 1 E, 2 OU, 3 SE…ENTÃO; −1 se não for conectivo
Color glifo_cor(const Estado *e, IdGlifo id);

void glifos_atualiza(Estado *e, float dt); // fagulhas no vidro enquanto ele fala
void glifos_desenha(const Estado *e);      // a frase atual projetada no vidro, tremendo levemente

#endif
