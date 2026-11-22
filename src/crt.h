// crt.h — a cara de monitor de fósforo das telas de tarefa (analisador, painéis, manual): moldura com
// cantos, scanlines e varredura, caixas com legenda, título com franja de cor e opções em faixa, como a tela
// de início.
#ifndef CRT_H
#define CRT_H

#include "estado.h"

// Fundo, borda fina, scanlines e a faixa de varredura descendo. `cantos` desenha as quinas na cor da borda.
void crt_painel(const Estado *e, Rectangle r, Color borda, bool cantos);

// Caixa com legenda pequena no alto ("FRASE OBSERVADA"); devolve a área útil, abaixo da legenda.
Rectangle crt_caixa(const Estado *e, Rectangle r, const char *legenda);

// Título com franjas vermelha e ciana e um brilho em volta (o h1 do analisador).
void crt_titulo(const Estado *e, const char *texto, Vector2 pos, float tamanho, Color cor);

// Texto com o trecho entre parênteses no fim mais apagado: "O que significa...?  (setas + ENTER)".
void crt_pergunta(const Estado *e, const char *texto, Vector2 pos, float tamanho);

// Opção em faixa: a escolhida acende em degradê com barra na lateral e uma falha leve; `errou` deixa a
// faixa vermelha e tremendo; `seta` põe o ▶ na calha da esquerda (o texto começa depois dela).
void crt_opcao(const Estado *e, const char *texto, Rectangle r, float tamanho, bool ativa, bool errou,
               bool seta, Color cor);

#endif
