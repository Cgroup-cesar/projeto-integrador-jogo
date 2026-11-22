// criatura.h — o Precursor: IA nos corredores (A*), placeholder procedural e captura (o susto: susto.h).
#ifndef CRIATURA_H
#define CRIATURA_H

#include "estado.h"

// Modelo opcional (assets/modelos/precursor.glb). Sem ele, o placeholder procedural entra sem erro.
void criatura_carrega_modelo(Recursos *rec);
void criatura_descarrega_modelo(Recursos *rec);

void criatura_contem(Estado *e);              // fases de sala: dentro da cela
void criatura_inicia_saida(Estado *e);        // saídas: surge do duto depois de alguns segundos
void criatura_puxa_para_contencao(Estado *e); // painel resolvido: some numa distorção
void criatura_atualiza(Estado *e, float dt);
void criatura_desenha(const Estado *e); // o corpo (geometria iluminada)

// Pego: conta para o placar ("saída sem ser pego"), vem o susto e a saída recomeça.
void criatura_captura(Estado *e);
// O armário: ele aparece parado na porta (de frente para ela), chega mais perto a cada erro e, quando a
// pessoa acerta a respiração, desiste e vai patrulhar o ponto mais longe dali.
void criatura_na_porta_do_armario(Estado *e);
void criatura_chega_mais_perto(Estado *e);
void criatura_desiste(Estado *e);

#endif
