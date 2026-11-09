// circuito.h — painéis de circuito das saídas (Painel 1 no gerador, Painel 2 no Arquivo).
#ifndef CIRCUITO_H
#define CIRCUITO_H

#include "estado.h"

void circuito_inicia(Estado *e, int numero);
void circuito_atualiza(Estado *e, float dt);
void circuito_desenha(const Estado *e);    // o painel aberto, em 2D
void circuito_desenha_3d(const Estado *e); // LEDs e lâmpada de saída no gabinete

void circuito_piloto(const Estado *e, Entrada *in);

#endif
