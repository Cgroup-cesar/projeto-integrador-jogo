// controle.h — o controle (Xbox ou qualquer um que a raylib reconheça) somado ao teclado e ao mouse.
// Ele só preenche a mesma Entrada do quadro: os módulos do jogo não sabem de onde veio a ação.
//
//   analógico esquerdo  andar (e, nos menus, vira as setas)     A      interagir · confirmar · respirar
//   analógico direito   olhar                                   B      voltar (sai do terminal, do painel...)
//   LB, LT ou L3        correr                                  Y      lanterna · LUZ B no terminal
//   direcional          setas                                   X      LUZ A no terminal · chave A no painel
//   View                ajuda                                   Start  pausa
#ifndef CONTROLE_H
#define CONTROLE_H

#include "estado.h"

// Lê o primeiro controle ligado e soma na Entrada já preenchida pelo teclado e pelo mouse (chame depois
// deles: usa in->alguma para saber se o teclado foi mexido neste quadro). Também decide in->controle, que
// fica ligado até alguém voltar a usar o teclado ou o mouse.
void controle_le(Entrada *in);

#endif
