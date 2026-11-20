// tela_nome.h — entrada do nome de quem joga, entre o menu e a intro. O nome fica só em memória.
#ifndef TELA_NOME_H
#define TELA_NOME_H

#include "estado.h"

void tela_nome_atualiza(Estado *e, float dt);
void tela_nome_desenha(const Estado *e);

#endif
