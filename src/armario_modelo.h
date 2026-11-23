// armario_modelo.h — a aparência dos armários de esconderijo (assets/modelos/armario.glb, opcional). Só
// muda o desenho: posição, colisão, o "E — esconder-se" e o puzzle da respiração continuam em nivel.c,
// jogador.c e respiracao.c. Sem o arquivo, a caixa de metal de antes.
#ifndef ARMARIO_MODELO_H
#define ARMARIO_MODELO_H

#include "estado.h"

void armario_modelo_carrega(Recursos *rec); // [REQ:RNF4] sem o arquivo, só registra no log
void armario_modelo_descarrega(Recursos *rec);
// Desenha o armário com a porta virada para `frente` (unitário, no chão) e a frente rente à face da caixa
// de colisão (`centro` e `tam` são os dela). Devolve false sem o modelo.
bool armario_modelo_desenha(const Estado *e, Vector3 centro, Vector3 tam, Vector3 frente);

#endif
