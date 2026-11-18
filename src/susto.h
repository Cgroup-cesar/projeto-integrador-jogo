// susto.h — o jumpscare: silêncio, o grito e o rosto do Precursor pulando até a câmera, tremendo.
#ifndef SUSTO_H
#define SUSTO_H

#include "estado.h"

// Começa o susto: JUMPSCARE_SILENCIO_S de silêncio, depois o grito e o rosto por susto_duracao(e)
// segundos; no fim, aplica `depois` (EV_PEGO, EV_VIDRO_ROMPEU...). Ignorado se já houver um susto.
void susto_inicia(Estado *e, EventoFase depois);
bool susto_ativo(const Estado *e); // no silêncio ou com o rosto na tela

// Quanto o rosto fica na tela: com assets/audio/jumpscare.*, até o fim do trecho alto do som
// (JUMPSCARE_AUDIO_S); com o som sintetizado ou sem áudio, JUMPSCARE_S.
float susto_duracao(const Estado *e);

// A boca: separa os vértices da mandíbula do modelo (uma vez, depois de criatura_carrega_modelo). No susto,
// susto_atualiza gira a mandíbula para abrir e fechar; fora dele, o modelo volta ao original.
void susto_carrega_boca(Recursos *rec);
void susto_descarrega_boca(Recursos *rec);

void susto_atualiza(Estado *e, float dt);
bool susto_rosto_na_tela(const Estado *e); // depois do silêncio; enquanto isso o corpo some da cena

// Desenho em dois passos: susto_prepara (antes do render target da cena) desenha o rosto 3D numa
// textura própria; susto_desenha (dentro dele) escurece o cenário e cola o rosto por cima.
void susto_prepara(const Estado *e);
void susto_desenha(const Estado *e);

#endif
