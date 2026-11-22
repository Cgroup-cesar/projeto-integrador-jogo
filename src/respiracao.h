// respiracao.h — segurar a respiração no armário. Ao entrar, o Precursor para na porta; uma linha vermelha
// vai e volta numa barra e a pessoa aperta ESPAÇO quando ela passa sobre a faixa branca. 3 chances.
// Na 1ª vez da partida, um tutorial congela o mundo e mostra como faz.
#ifndef RESPIRACAO_H
#define RESPIRACAO_H

#include "estado.h"

void respiracao_comeca(Estado *e); // chamado ao entrar no armário (nas saídas, com a energia ainda fora)
void respiracao_atualiza(Estado *e, float dt);
void respiracao_desenha(const Estado *e);

bool respiracao_ativa(const Estado *e);           // tutorial, desafio ou alívio: o E não tira do armário
bool respiracao_congela_mundo(const Estado *e);   // tutorial: nada mais se mexe
bool respiracao_segura_criatura(const Estado *e); // tutorial e desafio: ele fica parado na porta

void respiracao_piloto(const Estado *e, Entrada *in); // --smoke: aperta ESPAÇO na hora certa

#endif
