// mesa_bilhete.h — a mesa do Arquivo onde fica o bilhete do Varela verdadeiro: a escrivaninha de metal
// (assets/modelos/mesa.glb), as folhas (assets/modelos/papeis.glb), a lâmpada âmbar que pisca sobre ela e o
// brilho que respira no bilhete até ele ser lido. Os dois modelos são opcionais.
#ifndef MESA_BILHETE_H
#define MESA_BILHETE_H

#include "estado.h"

void mesa_bilhete_carrega(Recursos *rec); // [REQ:RNF4] sem os arquivos, a mesa e o papel simples de antes
void mesa_bilhete_descarrega(Recursos *rec);
void mesa_bilhete_desenha(const Estado *e);        // a mesa e os papéis (dentro da luz do jogo)
void mesa_bilhete_desenha_brilho(const Estado *e); // o halo da lâmpada e do bilhete (entre os halos)
float mesa_bilhete_piscada(const Estado *e);       // força da lâmpada agora (0..1), a mesma na luz e no halo

#endif
