// criatura.h — o Precursor: IA nos corredores (A*), placeholder procedural, captura e jumpscare.
#ifndef CRIATURA_H
#define CRIATURA_H

#include "estado.h"

void criatura_contem(Estado *e);       // fases de sala: dentro da cela
void criatura_inicia_saida(Estado *e); // saídas: surge do duto depois de alguns segundos
void criatura_atualiza(Estado *e, float dt);
void criatura_desenha(const Estado *e);           // o corpo (geometria iluminada)
void criatura_desenha_jumpscare(const Estado *e); // o rosto na câmera, por cima da cena

#endif
