// audio_sintese.h — todos os sons gerados em código (22050 Hz, mono). Se existir assets/audio/<nome>.ogg
// (ou .wav, .mp3), ele substitui o som sintetizado. O susto toca SOM_JUMPSCARE (arquivo "jumpscare").
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
    SOM_JUMPSCARE,           // só no susto (o apagão usa SOM_ESTRONDO)
    SOM_RESPIRACAO_OFEGANTE, // so toca ao sair do armario depois de muito tempo escondido
    SOM_GOTA,                // o pingo da goteira na poça (sala do gerador)
    SOM_CAMERA_MOTOR,        // o motor de uma câmera de segurança virando
    SOM_PORTA_ABRE,          // a porta de aço da sala de observação: a trava e o rangido
    SOM_PORTA_FECHA,         // o rangido e a batida no batente
    SOM_TOTAL
} IdSom;

void audio_inicia(Recursos *rec); // também define rec->susto_s (a duração do susto segue o som)
void audio_finaliza(Recursos *rec);

// Toca um som curto. volume 0..1; tom 1 = normal.
void audio_toca(const Estado *e, IdSom som, float volume, float tom);
// Som com lugar no mapa: volume pela distância e lado (esquerda/direita) por onde o jogador olha.
void audio_toca_em(const Estado *e, IdSom som, Vector3 fonte, float alcance, float tom);
// O mesmo, para um som que dura enquanto algo acontece: começa se parado; tocando, só acompanha o volume e
// o lado (pare com audio_para quando acabar). `ganho` multiplica o volume (1 = cheio).
void audio_mantem_em(const Estado *e, IdSom som, Vector3 fonte, float alcance, float tom, float ganho);
// O volume e o lado de um som em `fonte` agora (para quem toca o próprio Sound ou Music: tubulacao_som.c).
SomEspacial audio_espacial(const Estado *e, Vector3 fonte, float alcance);
float audio_pan(float pan); // −1 esquerda .. 1 direita, no formato da raylib em uso
void audio_para(const Estado *e, IdSom som);
void audio_silencia(const Estado *e); // para tudo (os 0,2 s de silêncio antes do jumpscare)

// Grava cada som sintetizado em <pasta>/<nome>.wav (para ouvir fora do jogo ou trocar por um .ogg).
void audio_exporta(const char *pasta);

// Laços de ambiência por fase, zumbido de lâmpada, alarme e respiração no armário.
void audio_atualiza(Estado *e, float dt);

#endif
