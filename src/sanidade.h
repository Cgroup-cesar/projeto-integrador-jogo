// sanidade.h — a sanidade do Prof. Elias: cai com a criatura perto, volta devagar e volta muito nas tarefas.
// Não aparece no HUD: vira grão na tela (o shader do VHS lê e->sanidade_grao). Regras em regras_sanidade.c.
#ifndef SANIDADE_H
#define SANIDADE_H

#include "estado.h"

// Um quadro: bônus dos conectivos recém-decifrados, queda/recuperação e o grão suavizado. Os bônus das
// tarefas e do recomeço vêm dos eventos de fase (jogo_aplica_evento → regras_sanidade_bonus).
void sanidade_atualiza(Estado *e, float dt);
// Nada a desenhar aqui: o grão é aplicado pelo shader do VHS (vhs_comeca), por baixo da interface.
void sanidade_desenha(const Estado *e);

#endif
