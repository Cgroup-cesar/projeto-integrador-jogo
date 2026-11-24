// retorno.h — a volta à sala de observação depois de religar a energia, como uma cena sem corte: ao entrar
// na sala, o jogo assume o jogador, vira para a porta, fecha a porta e entra na sala; a rodada seguinte
// começa quando a folha bate, sem teleporte. No fim, o controle volta.
#ifndef RETORNO_H
#define RETORNO_H

#include "estado.h"

bool retorno_ativo(const Estado *e);
void retorno_entra_fase(Estado *e, Fase fase, bool reinicio); // a cena só atravessa a troca saída → rodada
void retorno_atualiza(Estado *e, float dt);                   // começa a cena e move o jogador por ela
void retorno_desenha(const Estado *e);                        // as faixas pretas de cinema

#endif
