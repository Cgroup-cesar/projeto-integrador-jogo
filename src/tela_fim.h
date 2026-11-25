// tela_fim.h — depois do texto do final: o desempenho (3 números) com a avaliação do jogo em estrelas, e o
// ranking da feira com a partida em destaque. A tabela do ranking também abre pelo menu.
#ifndef TELA_FIM_H
#define TELA_FIM_H

#include "estado.h"
#include "dialogo.h"

void tela_fim_atualiza(Estado *e, float dt);
void tela_fim_desenha(const Estado *e);
// O ranking em tela cheia. `destaque`: a posição da partida que acabou (−1: nenhuma); `chamada`: a frase
// de chamada embaixo.
void tela_fim_desenha_ranking(const Estado *e, int destaque, TextoId chamada);

#endif
