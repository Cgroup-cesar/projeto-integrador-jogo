// painel_modelo.h — o quadro elétrico dos painéis das saídas (o gerador), em aço pintado: corpo, porta com
// dobradiças e puxador, venezianas, placa de perigo e os eletrodutos até o teto. Só desenho: a colisão e a
// interação continuam as da caixa de 1,2 × 1,8 × 0,5 m (nivel.c, circuito.c); os LEDs são de circuito.c.
#ifndef PAINEL_MODELO_H
#define PAINEL_MODELO_H

#include "estado.h"

// `base`: o centro da pegada no chão (x, z do painel). A frente olha para +z, como os LEDs de circuito.c.
void painel_modelo_desenha(const Estado *e, Vector3 base, int variacao);

#endif
