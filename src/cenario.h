// cenario.h — ambientação sem colisão: placas em estêncil, manchas, eletrodutos, grelhas, fiação e o halo
// das lâmpadas. Nada aqui entra em ColParedes nem muda a planta (a IA, as colisões e o --smoke não veem).
// Só desenho, a partir do estado: não há nada para atualizar por quadro.
#ifndef CENARIO_H
#define CENARIO_H

#include "estado.h"

// Dentro do shader de luz, depois de nivel_desenha (as paredes já estão no buffer de profundidade).
void cenario_desenha(const Estado *e);

#endif
