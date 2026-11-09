// compositor.h — a resposta final por fichas e os dois finais.
#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "estado.h"

void compositor_inicia(Estado *e);
void compositor_atualiza(Estado *e, float dt);
void compositor_desenha(const Estado *e);

void compositor_piloto(const Estado *e, Entrada *in, bool final_bom);

// Cinemática de cada final (FIM_RUIM e FIM_BOM) até a tela de placar.
void compositor_inicia_fim(Estado *e);
void compositor_atualiza_fim(Estado *e, float dt);
void compositor_desenha_fim(const Estado *e); // por cima da cena (dentro do render target)

#endif
