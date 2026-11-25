// jogador.h — câmera em 1ª pessoa, movimento com colisão, fôlego, armário e interação.
#ifndef JOGADOR_H
#define JOGADOR_H

#include "estado.h"

void jogador_teleporta(Estado *e, Vector3 pos, float yaw);
void jogador_vai_ao_console(Estado *e); // um passo até a frente do console, virando para o vidro
void jogador_poe_no_console(Estado *e); // o mesmo, na hora (piloto automático e atalhos de debug)
void jogador_sai_do_console(Estado *e); // um passo para trás, sem virar (só se está na frente dele)
bool jogador_na_sala(const Estado *e);  // dentro da sala de observação (onde as rodadas acontecem)
Vector3 jogador_olho(const Estado *e);
Vector3 jogador_direcao(const Estado *e);

void jogador_atualiza(Estado *e, float dt);
void jogador_passos(Estado *e, bool andando, float dt); // balanço da câmera, som e barulho dos passos
void jogador_atualiza_camera(Estado *e);
void jogador_desenha(const Estado *e); // por cima da cena: as frestas do armário
// O quanto a porta do armário cobre a tela agora (0 = nada, 1 = as frestas inteiras).
float jogador_porta_armario(const Estado *e);
// O quanto as folhas da porta do armário do jogador (jog.armario) estão abertas (0..1): abrem e fecham
// quando ele entra e sai.
float jogador_porta_aberta(const Estado *e);

#endif
