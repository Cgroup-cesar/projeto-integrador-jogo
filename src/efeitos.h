// efeitos.h — os passes de tela da cena HDR e a sombra da lanterna (ver docs/graficos.md):
//   sombra   o mapa de distâncias da lanterna (antes da cena), usado pelo shader de luz e pela névoa
//   SSAO     oclusão ambiente a ½ da resolução, lida da profundidade da cena
//   volume   névoa volumétrica (raymarch) a ½ da resolução: as luminárias e o feixe da lanterna na poeira
//   bloom    os brilhos acima de 1 espalhados (5 níveis)
//   compõe   cena × AO + névoa + bloom → exposição → ACES → gradação de cor → vinheta e grão → o quadro LDR
// Cada efeito liga e desliga pela qualidade (qualidade.c). Os recursos de GPU são deste módulo.
#ifndef EFEITOS_H
#define EFEITOS_H

#include "estado.h"

void efeitos_carrega(Recursos *rec); // depois de posfx_inicia (usa o shader de luz e o tamanho do alvo)
void efeitos_descarrega(void);
void efeitos_redimensiona(const Recursos *rec); // o alvo mudou de tamanho (janela, escala)

// As luzes e a câmera do quadro (a névoa refaz a luz e os passes de tela reconstroem a posição).
void efeitos_guarda_luzes(const Vector3 *pos, const Vector3 *cor, const Vector4 *cone, const float *queda,
                          int n, int lanterna);
void efeitos_guarda_camera(void); // dentro do BeginMode3D da câmera principal

// A sombra da lanterna, ANTES da cena (fecha e reabre o alvo HDR). Devolve false se a sombra está desligada.
// Entre as duas: desenhe a geometria que projeta sombra (o mesmo shader de luz, em modo sombra).
bool efeitos_comeca_sombra(const Estado *e, Vector3 pos, Vector3 direcao);
void efeitos_termina_sombra(const Estado *e);
void efeitos_sem_sombra(const Estado *e); // lanterna apagada: o shader de luz não lê o mapa

// A cena HDR (já fechada) → os passes → o quadro LDR (fica aberto para o que vem por cima). false = sem
// efeitos (quem chama faz o tone mapping simples).
bool efeitos_compoe(const Estado *e);

#endif
