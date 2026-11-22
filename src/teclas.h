// teclas.h — a barra de teclas: uma faixa no rodapé, sempre no mesmo lugar, que mostra só as teclas que
// funcionam agora (desenhadas como teclas) e um minimapa do teclado com elas acesas. Pensada para a feira: a
// cabine é escura, e a pessoa precisa achar a tecla sem enxergar o teclado.
#ifndef TECLAS_H
#define TECLAS_H

#include "estado.h"

void teclas_desenha(const Estado *e);
void teclas_desenha_tela_cheia(const Estado *e); // por cima do menu e das telas depois do final

#endif
