// cenario.h — ambientação sem colisão: placas em estêncil, manchas, eletrodutos, grelhas, fiação e o halo
// das lâmpadas. Nada aqui entra em ColParedes nem muda a planta (a IA, as colisões e o --smoke não veem).
// Só desenho, a partir do estado: não há nada para atualizar por quadro.
#ifndef CENARIO_H
#define CENARIO_H

#include "estado.h"

// Dentro do shader de luz, depois de nivel_desenha (as paredes já estão no buffer de profundidade).
void cenario_desenha(const Estado *e);

// Halo das lâmpadas (soma de luz, sem escrever profundidade): chame cenario_halo entre comeca e termina,
// fora do shader de luz (junto dos emissivos).
void cenario_comeca_halos(void);
void cenario_halo(const Estado *e, Vector3 posicao, Color cor);
void cenario_termina_halos(void);

#endif
