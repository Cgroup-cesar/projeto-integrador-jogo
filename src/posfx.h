// posfx.h — render target em resolução interna reduzida, filtro VHS e (a partir do M2) luz e névoa.
#ifndef POSFX_H
#define POSFX_H

#include "estado.h"

void posfx_inicia(Recursos *rec);
void posfx_finaliza(Recursos *rec);

// Recria o render target quando a janela muda de tamanho.
void posfx_atualiza(Estado *e, float dt);

// Tudo que for desenhado entre estas duas chamadas vai para o render target (e recebe o VHS).
void posfx_inicia_cena(const Estado *e);
void posfx_termina_cena(const Estado *e);

// Liga o shader de luz e névoa com as luzes do quadro (até LUZES_SHADER). Desenhe a geometria iluminada entre
// as duas. cones[i]: xyz = direção, w = cosseno da meia-abertura; w < -1 = luz pontual comum.
// quedas[i]: quanto a luz enfraquece com a distância (1 = lâmpada comum; menor = vai mais longe).
void posfx_comeca_luz(const Estado *e, const Vector3 *posicoes, const Vector3 *cores, const Vector4 *cones,
                      const float *quedas, int n, float ambiente);
void posfx_termina_luz(const Estado *e);

// Depois de comeca_luz: troca o ponto de vista da névoa e da contraluz (padrão: a câmera do jogador).
void posfx_olho(const Estado *e, Vector3 olho);

// Entre comeca_luz e termina_luz: contraluz nas bordas do que for desenhado a seguir (intensidade 0 desliga).
// Chame fora de rlPushMatrix/rlPopMatrix (envia o lote pendente com a transformação atual).
void posfx_contraluz(const Estado *e, Color cor, float intensidade);

// Amplia o render target para a janela inteira, com o filtro VHS se estiver ligado.
void posfx_desenha(const Estado *e);

#endif
