// goteira.h — o cano pingando na sala do gerador: a gota que incha, cai e abre ondas na poça, e o pingo
// (som no espaço, como os passos do Precursor). A poça e as manchas de chão molhado ficam em cenario.c.
// Sem colisão: a IA, as colisões e o --smoke não veem.
#ifndef GOTEIRA_H
#define GOTEIRA_H

#include "estado.h"

// Onde a gota cai (tirado do mapa de nivel.c): rente à parede leste da sala do gerador (x = 44), na
// linha 2 (z de 4 a 6), entre as caixas (canto nordeste) e a porta; longe do painel 1 e do armário.
#define GOTEIRA_X 43.55f
#define GOTEIRA_Z 5.6f

// Toca o pingo (no espaço) a cada gota que chega na poça. Só nas saídas, quando dá para chegar lá.
void goteira_atualiza(Estado *e, float dt);

// Dentro do shader de luz, depois das manchas do chão (cenario_desenha chama): cano, gota e ondas.
void goteira_desenha(const Estado *e);

#endif
