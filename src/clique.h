// clique.h — o mouse nas telas de interface. Ao desenhar, cada tela registra onde estão as opções que ela
// mostrou (clique_area); no quadro seguinte, o atualiza da tela pergunta o que está sob o ponteiro. Assim o
// clique acerta exatamente o que está na tela, sem repetir as contas de layout, e desenhar continua sem mudar
// o Estado (as áreas são um subproduto do desenho, como os pixels, e vivem só neste módulo).
#ifndef CLIQUE_H
#define CLIQUE_H

#include "estado.h"

typedef enum {
    CLIQUE_MENU,     // INICIAR, RANKING, SAIR
    CLIQUE_PAUSA,    // CONTINUAR, VOLTAR AO MENU
    CLIQUE_GENERO,   // professor (0) ou professora (1)
    CLIQUE_OPCAO,    // conectivos e traduções no terminal
    CLIQUE_LUZ,      // as luzes A (0) e B (1) do terminal
    CLIQUE_CHAVE,    // as chaves A, B e C do painel 1
    CLIQUE_ENCAIXE,  // os encaixes do painel 2 (0 e 1)
    CLIQUE_FICHA,    // as fichas da resposta final
    CLIQUE_PERGUNTA, // a avaliação: cada pergunta
    CLIQUE_ESTRELA,  // a avaliação: pergunta * AVALIACAO_MAX + estrela
    CLIQUE_TECLA,    // uma tecla da barra do rodapé (TeclaClique)
    CLIQUE_GRUPOS
} GrupoClique;

// As teclas da barra do rodapé que um clique aperta.
typedef enum {
    TECLA_ESC,
    TECLA_ENTER,
    TECLA_H,
    TECLA_1,
    TECLA_2,
    TECLA_3,
    TECLA_E,
    TECLA_F,
    TECLA_ESPACO,
    TECLA_NENHUMA
} TeclaClique;

void clique_limpa(void);                                        // no começo de cada desenho
void clique_area(GrupoClique grupo, int indice, Rectangle area); // ao desenhar uma opção clicável

// O índice da área do grupo sob o ponteiro (−1 = nenhuma). Só vale com o cursor à mostra.
int clique_sob_ponteiro(const Entrada *in, GrupoClique grupo);
bool clique_sobre_algo(const Entrada *in); // para trocar a seta pela mãozinha

// A regra das listas de opções: passar o ponteiro por cima escolhe (move o cursor); clicar escolhe e
// confirma. Devolve true no clique.
bool clique_escolhe(const Entrada *in, GrupoClique grupo, int *cursor);

// Um clique numa tecla da barra do rodapé vale como apertar a tecla (chamado logo depois de ler a entrada).
void clique_aperta_teclas(Entrada *in);

#endif
