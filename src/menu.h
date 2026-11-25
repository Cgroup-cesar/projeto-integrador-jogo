// menu.h — a tela de início: o título à esquerda, o Precursor visível atrás do vidro, as opções em faixas
// (INICIAR, RANKING, SAIR) e o 1º lugar do ranking como convite. A tabela do ranking abre por cima.
#ifndef MENU_H
#define MENU_H

#include "estado.h"

void menu_atualiza(Estado *e);
void menu_desenha(const Estado *e);

#endif
