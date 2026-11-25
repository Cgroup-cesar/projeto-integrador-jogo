// entulho.h — a sujeira do chão: poeira acumulada no pé das paredes, cacos de reboco caídos, lascas de
// piso e papéis largados. As posições saem do mapa (nivel.c) e de um sorteio fixo, célula por célula:
// mudou o mapa, o entulho acompanha sozinho. Sem colisão: a IA, as colisões e o --smoke não veem.
#ifndef ENTULHO_H
#define ENTULHO_H

#include "estado.h"

// Dentro do shader de luz, depois de nivel_desenha (cenario_desenha chama, antes das manchas d'água).
// Fora de rlPushMatrix/rlPopMatrix (envia o lote pendente).
void entulho_desenha(const Estado *e);

#endif
