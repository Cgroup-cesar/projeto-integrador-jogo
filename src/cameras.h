// cameras.h — câmeras de segurança nas salas e nos corredores (assets/modelos/camera.glb, opcional): presas
// no alto das paredes, apontando para onde o jogador passa, com o LED vermelho piscando. Só cenário.
#ifndef CAMERAS_H
#define CAMERAS_H

#include "estado.h"

void cameras_carrega(Recursos *rec); // [REQ:RNF4] sem o arquivo, não há câmeras (só registra no log)
void cameras_descarrega(Recursos *rec);
void cameras_desenha(const Estado *e);

#endif
