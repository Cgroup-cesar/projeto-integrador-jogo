// lanterna.h — a lanterna do Prof. Elias: F liga e desliga (sem bateria), a luz em cone saindo dos olhos e o
// modelo na mão, no canto inferior direito.
#ifndef LANTERNA_H
#define LANTERNA_H

#include "estado.h"

// Modelo opcional (assets/modelos/lanterna.glb). Sem ele, entra uma lanterna feita de cilindros.
void lanterna_carrega_modelo(Recursos *rec);
void lanterna_descarrega_modelo(Recursos *rec);

void lanterna_reinicia(Estado *e); // partida nova: começa apagada
void lanterna_atualiza(Estado *e, float dt);

// A luz do quadro para o shader (a 5ª, depois das 4 do cenário). Devolve false com a lanterna apagada.
bool lanterna_luz(const Estado *e, Vector3 *pos, Vector3 *cor, Vector4 *cone, float *queda);

void lanterna_desenha(const Estado *e);        // o modelo na mão (dentro do shader de luz)
void lanterna_desenha_brilho(const Estado *e); // a lente acesa (fora do shader: brilha sozinha)

#endif
