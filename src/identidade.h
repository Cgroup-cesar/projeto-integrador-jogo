// identidade.h — quem joga: "Professor ou professora?" e o nome, perguntados no começo da intro.
// O que o jogador escolhe vai para Estado.ident e entra nas falas pelos marcadores {NOME}, {PROF} e {x|y}.
#ifndef IDENTIDADE_H
#define IDENTIDADE_H

#include "estado.h"

// Menu e partida nova: professor e nome zerado (privacidade na feira; o campo começa limpo).
void identidade_reinicia(Estado *e);
// Sem nome (por exemplo, começando direto numa fase com --fase), usa o nome padrão de dialogo.c.
void identidade_garante_nome(Estado *e);

// Etapas INTRO_GENERO e INTRO_NOME: setas, letras, Backspace e ENTER. No modo feira, segue sozinho.
void identidade_atualiza(Estado *e, float dt);
void identidade_desenha(const Estado *e);

#endif
