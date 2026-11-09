// audio_sintese.h — todos os sons gerados em código (22050 Hz, mono). Se existir assets/audio/<nome>.ogg,
// ele substitui o som sintetizado.
#ifndef AUDIO_SINTESE_H
#define AUDIO_SINTESE_H

#include "estado.h"

typedef enum {
    SOM_ZUMBIDO_LAMPADA,
    SOM_VOZ_PRECURSOR,
    SOM_VERDADEIRO,
    SOM_FALSO,
    SOM_BIP_INTERFONE,
    SOM_MAQUINA_ESCREVER,
    SOM_INTERRUPTOR,
    SOM_PASSO,
    SOM_PASSO_CRIATURA,
    SOM_VIDRO_ESTALO,
    SOM_ESTRONDO,
    SOM_ALARME,
    SOM_RESPIRACAO,
    SOM_AMBIENTE_SALA,
    SOM_AMBIENTE_SAIDA,
    SOM_PAPEL,
    SOM_ENERGIA,
    SOM_DISTORCAO,
    SOM_TOTAL
} IdSom;

void audio_inicia(Recursos *rec);
void audio_finaliza(Recursos *rec);

// Toca um som curto. volume 0..1; tom 1 = normal.
void audio_toca(const Estado *e, IdSom som, float volume, float tom);
void audio_para(const Estado *e, IdSom som);
void audio_silencia(const Estado *e); // para tudo (os 0,2 s de silêncio antes do jumpscare)

// Laços de ambiência por fase, zumbido de lâmpada, alarme e respiração no armário.
void audio_atualiza(Estado *e, float dt);

#endif
