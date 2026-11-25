// fita.h — a fita do avistamento: enquanto a intro datilografa a história, três fotos de assets/fita/
// passam por trás do texto como uma filmagem caseira de 1985 (tremida, zoom lento, OSD da câmera, falhas de
// VHS e o sinal caindo antes da última linha). Qual foto e quanto falha: regras_fita_tomada (regras.c).
#ifndef FITA_H
#define FITA_H

#include "estado.h"

void fita_carrega(Recursos *rec); // [REQ:RNF4] foto ausente: aquele trecho fica preto (nunca falha)
void fita_descarrega(Recursos *rec);
void fita_desenha(const Estado *e); // o fundo da intro datilografada (sempre cobre a tela inteira)

#endif
