// trilhas.h — os sons longos de assets/audio, tocados por streaming (Music): trilha do menu, voz do
// Precursor, voz do rádio, coração e passos. Os sons curtos (cliques, bipes, estalos) continuam em
// audio_sintese.
#ifndef TRILHAS_H
#define TRILHAS_H

#include "estado.h"

// Carrega o que existir em assets/audio (depois de audio_inicia). Arquivo ausente: a trilha fica muda e,
// onde havia um som sintetizado equivalente (passos, voz), ele continua valendo.
void trilhas_carrega(Recursos *rec);
void trilhas_descarrega(Recursos *rec);
bool trilhas_tem(const Estado *e, IdTrilha trilha);

// A cada quadro: monta a situação, pergunta os volumes a regras_volumes_trilhas e faz o fade até eles.
void trilhas_atualiza(Estado *e, float dt);

#endif
