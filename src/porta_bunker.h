// porta_bunker.h — a porta de bunker da sala de observação (assets/modelos/porta_bunker.glb, opcional):
// fechada enquanto o jogador trabalha no console e abrindo para o corredor quando a saída começa.
#ifndef PORTA_BUNKER_H
#define PORTA_BUNKER_H

#include "estado.h"

void porta_bunker_carrega(Recursos *rec); // separa o batente da folha (são peças soltas na mesma malha)
void porta_bunker_descarrega(Recursos *rec);
// Desenha batente e folha. Devolve false sem o modelo (nivel.c desenha a porta de correr de antes).
bool porta_bunker_desenha(const Estado *e);
float porta_bunker_abertura(const Estado *e); // 0 = fechada, 1 = aberta (encostada na parede do corredor)

#endif
