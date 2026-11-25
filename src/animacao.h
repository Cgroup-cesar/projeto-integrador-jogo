// animacao.h — liga o jogo à animação do Precursor: lê o Estado (fase, IA, cela, lanterna, armário, bueiro,
// tubulação) e pede a pose a regras_anim_passo (puro). Não muda a IA, os gatilhos nem as colisões: só a
// apresentação.
#ifndef ANIMACAO_H
#define ANIMACAO_H

#include "estado.h"

void animacao_atualiza(Estado *e, float dt); // todo quadro, mesmo com a IA parada (porta do armário)

Vector3 animacao_pos(const Estado *e);  // onde o corpo é desenhado (alcança devagar os pulos curtos)
float animacao_yaw(const Estado *e);    // para onde ele aponta (o da IA, ou o do caminho até a tubulação)
bool animacao_visivel(const Estado *e); // inclui as mãos saindo do bueiro antes de ele ficar ativo

#endif
