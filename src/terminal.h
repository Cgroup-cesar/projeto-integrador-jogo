// terminal.h — a interface do ANALISADOR SEMÂNTICO SNI-85 (rodadas e resposta final).
#ifndef TERMINAL_H
#define TERMINAL_H

#include "estado.h"

void terminal_atualiza(Estado *e, float dt);
void terminal_desenha(const Estado *e);

// Moldura e cabeçalho comuns (a resposta final reaproveita). Devolve a área interna.
Rectangle terminal_moldura(const Estado *e, const char *cabecalho);

#endif
