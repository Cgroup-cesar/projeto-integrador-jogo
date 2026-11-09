// rodadas.h — dados e fluxo das 3 rodadas na sala de observação (decifrar e traduzir).
#ifndef RODADAS_H
#define RODADAS_H

#include "estado.h"
#include "glifos.h"

void rodadas_inicia(Estado *e, int numero);
void rodadas_atualiza(Estado *e, float dt);
void rodadas_desenha(const Estado *e);           // efeitos 3D da rodada
void rodadas_desenha_rachaduras(const Estado *e); // rachaduras do vidro, por cima da cena

// A frase que o Precursor está "dizendo" agora (o vidro e o terminal mostram). false = nenhuma.
bool rodadas_frase_atual(const Estado *e, FraseGlifos *frase);

// Piloto automático (--smoke): devolve as teclas que resolvem a rodada.
void rodadas_piloto(const Estado *e, Entrada *in);

#endif
