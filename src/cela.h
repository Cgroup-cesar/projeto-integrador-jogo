// cela.h — o Precursor contido: no centro da cela, olhando o vidro.
// Sem desenho próprio: o corpo é criatura_desenha e a névoa da cela é feita no shader de luz (posfx.c).
#ifndef CELA_H
#define CELA_H

#include "estado.h"

void cela_atualiza(Estado *e, float dt); // chamado por criatura_atualiza enquanto ele está contido

#endif
