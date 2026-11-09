// nivel.h — mapa ASCII → geometria, paredes, interativos e luzes.
#ifndef NIVEL_H
#define NIVEL_H

#include "estado.h"

#define MAX_LUZES_SHADER 4

void nivel_carrega(Nivel *n);

// Paredes do mapa (corridas de '#' viram uma caixa só), vidro e móveis.
void nivel_constroi_paredes(const Nivel *n, ColParedes *paredes);
void nivel_tranca_porta(const Nivel *n, ColParedes *paredes); // fases de sala: a porta da sala fica trancada
void nivel_constroi_interativos(const Nivel *n, ColInterativos *interativos);

// Coordenadas: célula (coluna, linha) ↔ mundo (1 célula = 2 m).
Vector3 nivel_centro_celula(int coluna, int linha);
bool nivel_celula_de(Vector3 pos, int *coluna, int *linha);
char nivel_celula(const Nivel *n, int coluna, int linha);
bool nivel_caminhavel(const Nivel *n, int coluna, int linha); // para o A* da criatura
bool nivel_na_sala_de_observacao(const Nivel *n, Vector3 pos);

// Até 4 luzes pontuais para o shader, escolhidas conforme a fase e a posição do jogador.
int nivel_luzes(const Estado *e, Vector3 *posicoes, Vector3 *cores);

void nivel_atualiza(Estado *e, float dt);
void nivel_desenha(const Estado *e);           // geometria iluminada (dentro do shader de luz)
void nivel_desenha_emissivos(const Estado *e); // lâmpadas, telas e LEDs (sem luz, brilham sozinhos)
void nivel_desenha_vidro(const Estado *e);     // o vidro da contenção, transparente, por último

#endif
