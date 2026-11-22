// regras_ranking.c — o ranking da feira, sem raylib e sem arquivo: ordenar e converter linha <-> partida.
// Quem lê e grava o arquivo é ranking.c; aqui só as regras (por isso dá para testar tudo).
#include "regras.h"
#include <stdio.h>
#include <string.h>

// A partida `a` fica na frente de `b`? Maior placar; empate: menos tempo. Empate total: não (quem já
// estava fica na frente).
static bool na_frente(const EntradaRanking *a, const EntradaRanking *b) {
    if (a->placar != b->placar) return a->placar > b->placar;
    return a->segundos < b->segundos;
}

// [REQ:PURA-regras_ranking_insere] acha o lugar da partida e empurra as de baixo (a última sai se encher)
int regras_ranking_insere(Ranking *r, const EntradaRanking *nova) {
    int pos = 0;
    while (pos < r->n && !na_frente(nova, &r->itens[pos])) // padrão FILTER: a primeira que ela supera
        pos++;
    if (pos >= RANKING_MAX) return -1;
    int ultimo = r->n < RANKING_MAX ? r->n : RANKING_MAX - 1;
    for (int i = ultimo; i > pos; i--) // padrão MAP: cada uma de baixo desce uma posição
        r->itens[i] = r->itens[i - 1];
    r->itens[pos] = *nova;
    if (r->n < RANKING_MAX) r->n++;
    return pos;
}

// [REQ:PURA-regras_ranking_le_linha] uma linha estragada é recusada, não derruba o ranking inteiro
bool regras_ranking_le_linha(const char *linha, EntradaRanking *saida) {
    int placar, segundos, vitoria, feminino, lidos = 0;
    if (sscanf(linha, "%d;%d;%d;%d;%n", &placar, &segundos, &vitoria, &feminino, &lidos) != 4 || lidos == 0)
        return false;
    const char *nome = linha + lidos;
    size_t n = strcspn(nome, "\r\n");
    if (n == 0 || n >= NOME_MAX_BYTES || memchr(nome, ';', n) != NULL || segundos < 0) return false;
    *saida = (EntradaRanking){placar, segundos, vitoria != 0, feminino != 0, {0}};
    memcpy(saida->nome, nome, n);
    return true;
}

int regras_ranking_escreve_linha(const EntradaRanking *e, char *saida, size_t tamanho) {
    int n = snprintf(saida, tamanho, "%d;%d;%d;%d;%s\n", e->placar, e->segundos, e->vitoria ? 1 : 0,
                     e->feminino ? 1 : 0, e->nome);
    return (n < 0 || (size_t)n >= tamanho) ? -1 : n;
}
