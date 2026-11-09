// posfx.h — render target em resolução interna reduzida, filtro VHS e (a partir do M2) luz e névoa.
#ifndef POSFX_H
#define POSFX_H

#include "estado.h"

void posfx_inicia(Recursos *rec);
void posfx_finaliza(Recursos *rec);

// Recria o render target quando a janela muda de tamanho; liga/desliga o VHS (F1).
void posfx_atualiza(Estado *e, float dt);

// Tudo que for desenhado entre estas duas chamadas vai para o render target (e recebe o VHS).
void posfx_inicia_cena(const Estado *e);
void posfx_termina_cena(const Estado *e);

// Liga o shader de luz e névoa com as luzes do quadro (até 4). Desenhe a geometria iluminada entre as duas.
void posfx_comeca_luz(const Estado *e, const Vector3 *posicoes, const Vector3 *cores, int n, float ambiente);
void posfx_termina_luz(const Estado *e);

// Amplia o render target para a janela inteira, com o filtro VHS se estiver ligado.
void posfx_desenha(const Estado *e);

#endif
