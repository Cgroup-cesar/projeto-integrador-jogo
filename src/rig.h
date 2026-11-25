// rig.h — o rig do modelo do Precursor (um .glb sem esqueleto): os pesos de cada vértice são calculados ao
// carregar, a partir dos ossos de esqueleto.c, e o shader de luz deforma a malha na GPU (skinning).
// Sem o modelo, o mesmo esqueleto anima o corpo procedural (cilindros entre as articulações).
#ifndef RIG_H
#define RIG_H

#include "estado.h"

// Depois do modelo e do shader de luz. Sem modelo (ou sem shader), deixa só a escala nominal.
void rig_carrega(Recursos *rec);
void rig_descarrega(Recursos *rec);

// Envia as matrizes da pose do quadro e liga o skinning; desliga depois do modelo. Fora de
// rlPushMatrix/rlPopMatrix: as duas enviam o lote pendente.
void rig_liga(const Estado *e);
void rig_desliga(const Estado *e);

// Sem o .glb: o corpo procedural sobre o mesmo esqueleto (desenhe dentro da transformação do corpo).
void rig_desenha_procedural(const Estado *e, Color cor);
// Um ponto do modelo em repouso (unidades do arquivo), preso a `osso`, no espaço do corpo desenhado (m).
Vector3 rig_ponto_no_corpo(const Estado *e, int osso, Vector3 repouso);

#endif
