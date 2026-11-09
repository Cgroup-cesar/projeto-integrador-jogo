// jogo.h — aplica regras_proxima_fase e despacha atualiza/desenha para o módulo de cada fase.
#ifndef JOGO_H
#define JOGO_H

#include "estado.h"

void jogo_inicia(Estado *e);
void jogo_finaliza(Estado *e);

// Aplica um evento: a decisão é de regras_proxima_fase (pura); aqui só se prepara a fase nova.
void jogo_aplica_evento(Estado *e, EventoFase evento);
void jogo_reinicia_fase(Estado *e);
void jogo_vai_para_fase(Estado *e, Fase fase); // debug (--fase R2): começa direto numa fase

void jogo_atualiza(Estado *e, float dt);
void jogo_desenha_cena(const Estado *e);      // 3D e efeitos de tela (dentro do render target, com VHS)
void jogo_desenha_interface(const Estado *e); // terminal, painéis, HUD, legendas (resolução da janela)

#endif
