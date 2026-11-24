// botoes.h — os ícones dos botões do controle de Xbox (assets/botoes/*.png, brancos com transparência), para
// a barra de teclas mostrar o botão desenhado em vez do nome. Cada ícone é opcional: sem o arquivo, quem
// desenha cai na caixinha com o texto.
#ifndef BOTOES_H
#define BOTOES_H

#include "estado.h"

void botoes_carrega(Recursos *rec); // [REQ:RNF4] ícone ausente: aquele botão volta a ser texto (nunca falha)
void botoes_descarrega(Recursos *rec);

// Desenha o ícone do rótulo ("A", "LB", "VIEW", "SETAS_V"...) com a altura dada, a partir de `pos` (canto
// superior esquerdo), tingido com `cor`. Devolve a largura usada, ou 0 se não houver ícone para ele.
float botoes_desenha(const Estado *e, const char *rotulo, Vector2 pos, float altura, Color cor);

#endif
