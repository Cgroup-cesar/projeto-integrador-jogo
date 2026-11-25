// teclas.h — as teclas que funcionam agora, desenhadas como teclas. Na partida, um "B Comandos" pequeno no
// canto inferior esquerdo abre e fecha a lista; no menu e nas telas depois do final, uma barra no rodapé.
#ifndef TECLAS_H
#define TECLAS_H

#include "estado.h"

void teclas_atualiza(Estado *e, float dt); // B abre e fecha a lista de comandos
void teclas_desenha(const Estado *e);
void teclas_desenha_tela_cheia(const Estado *e); // por cima do menu e das telas depois do final

#endif
