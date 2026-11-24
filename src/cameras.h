// cameras.h — câmeras de segurança nas salas e nos corredores (assets/modelos/camera.glb, opcional): presas
// no alto das paredes por um braço, com o LED vermelho piscando. As que enxergam o jogador viram para ele,
// com o som do motor. Só cenário: não mudam nada na partida.
#ifndef CAMERAS_H
#define CAMERAS_H

#include "estado.h"

void cameras_carrega(Recursos *rec); // [REQ:RNF4] sem o arquivo, não há câmeras (só registra no log)
void cameras_descarrega(Recursos *rec);
// As que enxergam o jogador viram para ele (com o som do motor); as outras ficam paradas sem gastar nada.
void cameras_atualiza(Estado *e, float dt);
void cameras_desenha(const Estado *e);

#endif
