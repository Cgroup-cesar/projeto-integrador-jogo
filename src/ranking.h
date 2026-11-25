// ranking.h — o ranking e as avaliações no disco do estande. Só arquivo: quem ordena e converte é
// regras_ranking.c. No --smoke nada é lido nem gravado (o teste não suja o ranking da feira).
#ifndef RANKING_H
#define RANKING_H

#include "estado.h"

void ranking_carrega(Ranking *r);     // RANKING_ARQUIVO; sem arquivo, ranking vazio
bool ranking_salva(const Ranking *r); // grava num temporário e troca: um desligamento não corrompe
// Uma linha no AVALIACOES_ARQUIVO: data, resultado, placar e as estrelas (0 = não respondeu). Sem nome.
bool ranking_registra_avaliacao(const int *notas, int placar, bool vitoria);

#endif
