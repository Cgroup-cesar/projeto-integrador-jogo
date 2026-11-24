// chave.h — a porta trancada do Arquivo (a sala do segundo gerador) e a chave dela, no chaveiro da sala de
// observação, ao lado do quadro. A porta fica trancada nas duas saídas; a chave só aparece na S2 (a segunda
// queda de energia), e um aviso diz onde ela está.
#ifndef CHAVE_H
#define CHAVE_H

#include "estado.h"
#include "regras.h"

// Ao entrar numa fase (depois de prepara_mundo): zera a chave no começo de cada saída (no recomeço da S2,
// depois de ser pego, a chave e a porta continuam como estavam), tranca a porta e mostra o aviso da S2.
void chave_entra_fase(Estado *e, Fase fase, bool reinicio);
void chave_atualiza(Estado *e, float dt);

void chave_pega(Estado *e);        // E no chaveiro
void chave_tenta_porta(Estado *e); // E na porta do Arquivo

Vector3 chave_chaveiro_centro(void);
void chave_desenha_chaveiro(const Estado *e); // o chaveiro e a chave (cena 3D)
void chave_desenha_porta(const Estado *e);    // a porta do Arquivo, fechada ou deslizando (cena 3D)
void chave_desenha_brilho(const Estado *e);   // o brilho da chave e o LED da porta (entre os halos)
void chave_desenha_aviso(const Estado *e);    // o aviso na tela (interface)

#endif
