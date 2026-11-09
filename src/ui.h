// ui.h — fonte (com fallback), texto, menu, HUD, legendas e tela de fim.
#ifndef UI_H
#define UI_H

#include <stddef.h>
#include "estado.h"

// ------------------------------------------------------------------ fonte e texto
void ui_carrega_fonte(Recursos *rec);
void ui_descarrega_fonte(Recursos *rec);

// Copia `entrada` para `saida` trocando o que a fonte não sabe desenhar (á→a, ç→c, ∧→^...).
// Sem fonte externa, todo caractere fora do ASCII é trocado. Nunca aparece caractere quebrado.
void translitera(const char *entrada, char *saida, size_t tamanho, const Font *fonte);

// Procura um asset opcional em assets/ (na pasta atual ou ao lado do executável).
bool ui_caminho_asset(const char *relativo, char *saida, size_t tamanho);

// Escala da interface: 1.0 numa janela de 720 px de altura.
float ui_escala(void);

void ui_texto(const Estado *e, const char *texto, Vector2 pos, float tamanho, Color cor);
void ui_texto_centro(const Estado *e, const char *texto, float y, float tamanho, Color cor);
Vector2 ui_mede(const Estado *e, const char *texto, float tamanho);

// ------------------------------------------------------------------ telas
void ui_atualiza(Estado *e, float dt);
void ui_desenha(const Estado *e);

#endif
