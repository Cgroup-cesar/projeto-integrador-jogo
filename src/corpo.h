// corpo.h — o corpo do americano, coberto por uma lona, no fim das marcas de arrasto do corredor sul
// (assets/modelos/corpo.glb, opcional). Examinar mostra quem era (FALAS_CORPO).
#ifndef CORPO_H
#define CORPO_H

#include "estado.h"

void corpo_carrega(Recursos *rec); // [REQ:RNF4] sem o arquivo, um volume escuro no mesmo lugar
void corpo_descarrega(Recursos *rec);
void corpo_desenha(const Estado *e);

#endif
