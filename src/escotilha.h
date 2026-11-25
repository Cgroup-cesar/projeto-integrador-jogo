// escotilha.h — a escotilha no teto da sala de observação e a escada até ela, ao lado dos caixotes. Só
// cenário: o jogador não sobe; a tampa abre na cena do final bom (cena_final.c).
#ifndef ESCOTILHA_H
#define ESCOTILHA_H

#include "estado.h"

void escotilha_adiciona_paredes(ColParedes *paredes); // a colisão da escada (o jogador não sobe)
void escotilha_desenha_teto(const Estado *e);         // a laje do teto com o vão e o poço acima dele
void escotilha_desenha(const Estado *e);              // a escada, o aro e a tampa (aberta só no final bom)

#endif
