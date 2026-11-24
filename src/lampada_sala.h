// lampada_sala.h — a lâmpada pendurada da sala de observação, em cima de onde o jogador começa
// (assets/modelos/lampada.glb, opcional). Sem o arquivo, uma lâmpada simples de fio, cúpula e bulbo.
#ifndef LAMPADA_SALA_H
#define LAMPADA_SALA_H

#include "estado.h"

void lampada_sala_carrega(Recursos *rec); // [REQ:RNF4] sem o arquivo, só registra no log
void lampada_sala_descarrega(Recursos *rec);

// Onde fica o bulbo: é dali que sai a luz da sala (nas rodadas e nas saídas).
Vector3 lampada_sala_bulbo(void);

// A lâmpada (o modelo ou a simples), com a luz do jogo.
void lampada_sala_desenha(const Estado *e);
// O bulbo aceso, por dentro da gaiola. `forca` 0 = apagado.
void lampada_sala_desenha_bulbo(Color cor, float forca);

#endif
