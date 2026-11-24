// tubulacao_som.h — os sons da tubulação por onde o Precursor vai da cela ao corredor: a tampa ao entrar na
// tubulação da cela e ao sair pelo duto do corredor, e os passos dele por dentro dos canos, que andam em
// linha reta de uma boca à outra (som espacial). Só som: lê o Estado e não muda o jogo.
#ifndef TUBULACAO_SOM_H
#define TUBULACAO_SOM_H

#include "estado.h"

// Depois de audio_inicia. Sem os arquivos de assets/audio, os sons ficam mudos (só um TraceLog).
void tubulacao_som_carrega(Recursos *rec);
void tubulacao_som_descarrega(Recursos *rec);

void tubulacao_som_atualiza(Estado *e, float dt); // todo quadro, também na pausa (os passos param)

#endif
