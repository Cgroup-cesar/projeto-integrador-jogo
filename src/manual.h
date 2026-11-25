// manual.h — o "manual" que o terminal e os painéis compartilham: a lâmpada, o ✓/✗, a mini-tabela e o cartão
// de um conectivo (ou porta). A mesma linguagem visual nos dois lugares: a porta E do circuito é o E da
// frase. Só desenha; quem calcula os valores é quem chama (logica.c / regras.c).
#ifndef MANUAL_H
#define MANUAL_H

#include "estado.h"

void manual_lampada(Vector2 c, float r, bool acesa);             // luz acesa (âmbar) ou apagada
void manual_marca(Vector2 c, float r, bool verdade, float alfa); // ✓ verde ou ✗ vermelho
// Lâmpadas (A, e B se houver) e o resultado de cada linha: ✓, ✗ ou "?" (valor < 0: ainda não visto).
void manual_mini_tabela(const Estado *e, Vector2 pos, bool unario, const int *valores, float s);
// Um cartão: nome, o que faz em palavras e a mini-tabela presa à borda direita. `destaque`: borda âmbar.
void manual_cartao(const Estado *e, Rectangle b, const char *nome, const char *descricao, bool unario,
                   const int *valores, bool destaque, float s);
// O selo "[H] AJUDA" / "[H] FECHAR AJUDA", alinhado à direita a partir de (direita, topo).
void manual_selo_ajuda(const Estado *e, float direita, float topo, float s);

#endif
