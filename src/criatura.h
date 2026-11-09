// criatura.h — o Precursor: IA nos corredores (A*), placeholder procedural, captura e jumpscare.
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
void criatura_desenha(const Estado *e);           // o corpo (geometria iluminada)
void criatura_desenha_jumpscare(const Estado *e); // o rosto na câmera, por cima da cena

// Pego: conta para o placar ("saída sem ser pego"), vem o susto e a saída recomeça.
void criatura_captura(Estado *e);

// Susto: 0,2 s de silêncio, depois até 1,2 s do rosto com a fenda aberta; no fim, aplica `depois`.
void criatura_susto(Estado *e, EventoFase depois);
void criatura_atualiza_susto(Estado *e, float dt);
bool criatura_em_susto(const Estado *e);

#endif
