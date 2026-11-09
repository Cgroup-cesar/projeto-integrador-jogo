// checagem.h — testes sem framework: CHECK conta, mostra o que falhou e segue.
#ifndef CHECAGEM_H
#define CHECAGEM_H

#include <stdio.h>

static int checagens_total = 0;
static int checagens_falhas = 0;

#define CHECK(condicao)                                                                                      \
    do {                                                                                                     \
        checagens_total++;                                                                                   \
        if (!(condicao)) {                                                                                   \
            checagens_falhas++;                                                                              \
            printf("  FALHOU  %s:%d  %s\n", __FILE__, __LINE__, #condicao);                                  \
        }                                                                                                    \
    } while (0)

// Imprime o resumo e devolve o código de saída do programa (0 = tudo certo).
static int resultado_final(const char *nome) {
    printf("%-12s %3d/%-3d checagens ok%s\n", nome, checagens_total - checagens_falhas, checagens_total,
           checagens_falhas == 0 ? "" : "  <-- FALHAS");
    return checagens_falhas == 0 ? 0 : 1;
}

#endif
