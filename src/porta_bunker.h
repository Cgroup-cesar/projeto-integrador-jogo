// porta_bunker.h — o modelo da porta de bunker da sala de observação (assets/modelos/porta_bunker.glb,
// opcional). Quanto a folha está aberta vem de porta_sala.c; ela abre para o corredor.
#ifndef PORTA_BUNKER_H
#define PORTA_BUNKER_H

#include "estado.h"

void porta_bunker_carrega(Recursos *rec); // separa o batente da folha (são peças soltas na mesma malha)
void porta_bunker_descarrega(Recursos *rec);
// Desenha batente e folha. Devolve false sem o modelo (nivel.c desenha a porta de correr de antes).
bool porta_bunker_desenha(const Estado *e);

#endif
