// caixas.h — pilhas de caixotes militares de madeira encostadas nas paredes das salas
// (assets/modelos/caixas.glb, opcional). Sem o modelo, entram caixas simples no mesmo lugar e tamanho.
#ifndef CAIXAS_H
#define CAIXAS_H

#include "estado.h"

void caixas_carrega(Recursos *rec); // [REQ:RNF4] sem o arquivo, só registra no log (o placeholder entra)
void caixas_descarrega(Recursos *rec);
void caixas_adiciona_paredes(ColParedes *paredes); // a colisão de cada pilha (vale com ou sem o modelo)
void caixas_desenha(const Estado *e);

#endif
