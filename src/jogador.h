// jogador.h — câmera em 1ª pessoa, movimento com colisão, fôlego, armário e interação.
#ifndef JOGADOR_H
#define JOGADOR_H

#include "estado.h"

void jogador_teleporta(Estado *e, Vector3 pos, float yaw);
void jogador_vai_ao_console(Estado *e); // posiciona na frente do console, olhando para o vidro
Vector3 jogador_olho(const Estado *e);
Vector3 jogador_direcao(const Estado *e);

void jogador_atualiza(Estado *e, float dt);
void jogador_atualiza_camera(Estado *e);
void jogador_desenha(const Estado *e); // por cima da cena: as frestas do armário

#endif
