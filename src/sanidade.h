// sanidade.h — a sanidade do Prof. Elias: barra fixa no topo da tela e grão que cresce com a criatura perto.
// As contas estão em regras_sanidade.c (puras); o grão é aplicado pelo shader do VHS (e->sanidade_grao).
#ifndef SANIDADE_H
#define SANIDADE_H

#include "estado.h"

// Um quadro: bônus dos conectivos recém-decifrados, queda/recuperação, a barra suavizada e a força do efeito
// na tela (1 − barra/100). Os bônus das tarefas e do recomeço vêm de jogo_aplica_evento →
// regras_sanidade_bonus.
void sanidade_atualiza(Estado *e, float dt);
// A barra no topo: "SANIDADE", a porcentagem, o pedaço recém-perdido e o alerta (vermelha e pulsando).
void sanidade_desenha(const Estado *e);

#endif
