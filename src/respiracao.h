// respiracao.h — segurar a respiração no armário. O armário esconde; se o Precursor vinha atrás de quem
// entrou, ele vai até lá e para na porta. Aí uma linha vermelha vai e volta numa barra e a pessoa aperta
// ESPAÇO quando ela passa sobre a faixa branca. 3 chances. Sem tutorial: a tecla ESPAÇO aparece embaixo
// da barra.
#ifndef RESPIRACAO_H
#define RESPIRACAO_H

#include "estado.h"

void respiracao_comeca(Estado *e); // ao entrar no armário: anota se ele vinha atrás (regras_armario_*)
void respiracao_atualiza(Estado *e, float dt);
void respiracao_desenha(const Estado *e);

bool respiracao_ativa(const Estado *e);           // desafio ou alívio: o E não tira do armário
bool respiracao_segura_criatura(const Estado *e); // desafio: ele fica parado na porta

void respiracao_piloto(const Estado *e, Entrada *in); // --smoke: aperta ESPAÇO na hora certa

#endif
