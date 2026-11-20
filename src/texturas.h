// texturas.h — as texturas do mapa: geradas UMA vez (procedurais, com override opcional em
// assets/texturas/), liberadas ao sair, e as primitivas que desenham com elas (caixa, quadro e halo).
#ifndef TEXTURAS_H
#define TEXTURAS_H

#include "estado.h"

// Chame depois de ui_carrega_fonte (as placas usam a fonte) e antes do primeiro quadro.
void texturas_carrega(Recursos *rec);
void texturas_descarrega(Recursos *rec);

#endif
