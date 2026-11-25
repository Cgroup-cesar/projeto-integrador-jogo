// falha.h — a "fita velha" reaproveitável: o chuvisco de TV por cima da cena e a falha de uma opção escolhida
// (balanço, tremida logo depois da troca e cópias vermelha e ciana). Só desenha; tudo sai do relógio.
#ifndef FALHA_H
#define FALHA_H

#include "estado.h"

// Sorteio repetível (o mesmo número dá sempre o mesmo resultado), de 0 a 1.
float falha_sorteio(int n);

// Chuvisco aditivo (só clareia: a cena continua à vista). `faixas` liga as faixas claras que descem.
void falha_chuvisco(const Estado *e, float alfa, float pico, bool faixas);

// Quanto a opção escolhida se desloca agora, em px; `intensidade` 1 é a do menu. Em *forca, de 0 a 1,
// quanto a falha está forte neste quadro (para falha_texto).
float falha_deslocamento(const Estado *e, float intensidade, float *forca);

// O texto da opção com a falha: cópias vermelha e ciana e uma fatia que escorrega dentro de `faixa`.
void falha_texto(const Estado *e, const char *texto, Vector2 pos, float tamanho, Color cor, float forca,
                 Rectangle faixa);

// Texto de OSD da fita (PLAY, REC, a data): branco com franjas vermelha e ciana, como um VHS velho.
void falha_osd(const Estado *e, const char *texto, Vector2 pos, float tamanho);
#endif
