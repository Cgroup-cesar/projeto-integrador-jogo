// quadro.h — o quadro de investigação na parede sul da sala de observação (assets/modelos/quadro.glb,
// opcional): fichas, digitais, a página do jornal Rede Bola com a foto do avistamento e um relatório do SNI,
// ligados por linhas vermelhas. Só cenário; sem o arquivo, a parede fica vazia.
#ifndef QUADRO_H
#define QUADRO_H

#include "estado.h"

void quadro_carrega(Recursos *rec);
void quadro_descarrega(Recursos *rec);
void quadro_desenha(const Estado *e);

#endif
