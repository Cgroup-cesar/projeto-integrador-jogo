// rodadas.h — dados e fluxo das 3 rodadas na sala de observação (decifrar e traduzir).
#ifndef RODADAS_H
#define RODADAS_H

#include "estado.h"
#include "glifos.h"

#define MAX_COLUNAS 2

// O que cada rodada pede. As tabelas-verdade NÃO estão aqui: saem de logica_avalia.
typedef struct {
    FraseGlifos teste[MAX_COLUNAS]; // frase(s) observadas (a R2 tem duas, em duas colunas)
    int n_colunas;
    int n_luzes; // a R1 usa só a luz A
    int conectivo_certo[MAX_COLUNAS];
    FraseGlifos traducao; // o que ele "diz" na etapa de traduzir
    int traducao_certa;   // índice da opção certa (ordem de dialogo.c)
} DadosRodada;

const DadosRodada *rodadas_dados(int numero);
int rodadas_linhas(const DadosRodada *d); // 2 ou 4
int rodadas_linha_atual(const Estado *e);

void rodadas_inicia(Estado *e, int numero);
void rodadas_atualiza(Estado *e, float dt);
void rodadas_desenha(const Estado *e); // as rachaduras do vidro, por cima da cena

// As frases que o Precursor está mostrando agora (o vidro projeta). Devolve quantas.
int rodadas_frases_atuais(const Estado *e, FraseGlifos *frases, int maximo);

// Piloto automático (--smoke): devolve as teclas que resolvem a rodada, errando de propósito
// até o número de erros pedido (para o smoke passar também pelo caminho do erro).
void rodadas_piloto(const Estado *e, Entrada *in, int erros_desejados);

#endif
