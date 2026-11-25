// console_modelo.h — o computador de época sobre a mesa do console (assets/modelos/terminal.glb, opcional).
// Sem o arquivo, a mesa continua com a tela verde de antes (nivel.c).
#ifndef CONSOLE_MODELO_H
#define CONSOLE_MODELO_H

#include "estado.h"

void console_modelo_carrega(Recursos *rec); // [REQ:RNF4] sem o arquivo, só registra no log
void console_modelo_descarrega(Recursos *rec);
// Desenha o terminal em cima da mesa, virado para quem está no console. Devolve false sem o modelo.
bool console_modelo_desenha(const Estado *e);

#endif
