// bancada.h — ferramentas de bancada para comparar o visual e medir o desempenho, só pela linha de comando:
//   --olhar x,z,yaw,pitch   põe o jogador nesse ponto e olhando para lá, ao entrar na fase (capturas)
//   --lanterna              entra com a lanterna acesa
//   --energia               entra com a energia já religada (as lâmpadas do corredor acesas)
//   --medir <s>             mede o tempo de quadro por <s> segundos, imprime a média e o pior 1% e fecha
// Sem essas opções, nada aqui muda o jogo.
#ifndef BANCADA_H
#define BANCADA_H

#include "estado.h"

// Lê as opções da bancada (as outras opções do jogo passam direto). Devolve true se reconheceu argv[*i]
// (e avança *i quando a opção leva valor).
bool bancada_le_argumento(int argc, char **argv, int *i);

// Depois de entrar na primeira fase: aplica --olhar, --lanterna e --energia.
void bancada_aplica(Estado *e);

// A cada quadro, com o tempo real do quadro. Devolve true quando a medição terminou (hora de fechar).
bool bancada_quadro(float tempo_real);

#endif
