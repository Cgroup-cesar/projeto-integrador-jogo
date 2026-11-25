// porta_sala.h — a porta da sala de observação. Nas rodadas, trancada. Nas saídas, começa fechada e o
// jogador a abre e fecha com E (com som); a criatura, chegando nela fechada, abre com um tranco. A colisão
// segue a folha: quase fechada, ela fecha o vão.
#ifndef PORTA_SALA_H
#define PORTA_SALA_H

#include "estado.h"

void porta_sala_entra_fase(Estado *e); // fechada (e sólida) no começo de toda fase
void porta_sala_interage(Estado *e);   // E na porta: trancada nas rodadas; abre ou fecha nas saídas
void porta_sala_fecha(Estado *e);      // a volta à sala (retorno.c) fecha a porta
void porta_sala_atualiza(Estado *e, float dt);
float porta_sala_abertura(const Estado *e); // 0 = fechada, 1 = aberta, já com a aceleração da folha
void porta_sala_desenha(const Estado *e);   // pilares e porta de bunker (sem o modelo, a de correr)

#endif
