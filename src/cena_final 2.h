// cena_final.h — o final bom como cena: a saída pelo corredor até a lanterna do Varela e a fita do Exército
// dando a volta na base, lá fora (assets/modelos/base.glb, opcional). A coreografia é de regras_cena.c.
#ifndef CENA_FINAL_H
#define CENA_FINAL_H

#include "estado.h"

void cena_final_carrega(Recursos *rec);
void cena_final_descarrega(Recursos *rec);
void cena_final_inicia(Estado *e);                         // as poses da caminhada (no começo do final)
void cena_final_atualiza(Estado *e, float antes, float t); // move o jogador e abre a porta
bool cena_final_dentro(const Estado *e);                   // a caminhada: as luzes vêm de cena_final_luzes
bool cena_final_fora(const Estado *e);                     // a fita lá fora
int cena_final_luzes(const Estado *e, Vector3 *pos, Vector3 *cor, Vector4 *cone, int *ids);
void cena_final_desenha_emissivos(const Estado *e); // os painéis acendendo e a lente da lanterna
bool cena_final_desenha_fora(const Estado *e);      // a base (com as próprias luzes); false = não é a vez
void cena_final_desenha(const Estado *e);           // faixas, a gravação, o branco e o preto (no quadro)

#endif
