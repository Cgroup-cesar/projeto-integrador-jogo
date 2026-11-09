// funcional.h — map, filter e fold genéricos (void*, tamanho do elemento e ponteiro de função).
// São os três padrões dos slides escritos uma vez só. Onde a versão tipada deixa o código mais claro
// para quem está começando, ela continua existindo (ver colecoes.c e regras.c).
#ifndef FUNCIONAL_H
#define FUNCIONAL_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef void (*FxMapFn)(void *elemento, void *contexto);
typedef bool (*FxFiltroFn)(const void *elemento, void *contexto);
typedef void (*FxFoldFn)(void *acumulador, const void *elemento, void *contexto);

// Aplica `f` a cada elemento, alterando-o no lugar.
static inline void fx_map(void *vetor, int n, size_t tamanho, FxMapFn f, void *contexto) {
    unsigned char *base = (unsigned char *)vetor;
    // padrão MAP
    for (int i = 0; i < n; i++)
        f(base + (size_t)i * tamanho, contexto);
}

// Mantém só os elementos em que `manter` devolve true, compactando no próprio vetor (v[j++] = v[i]).
// Devolve o novo tamanho. Não aloca nada.
static inline int fx_filter_inplace(void *vetor, int n, size_t tamanho, FxFiltroFn manter, void *contexto) {
    unsigned char *base = (unsigned char *)vetor;
    int j = 0;
    // padrão FILTER
    for (int i = 0; i < n; i++) {
        unsigned char *elemento = base + (size_t)i * tamanho;
        if (!manter(elemento, contexto)) continue;
        if (j != i) memcpy(base + (size_t)j * tamanho, elemento, tamanho);
        j++;
    }
    return j;
}

// Reduz o vetor a um valor: `f` combina o acumulador com cada elemento.
static inline void fx_fold(const void *vetor, int n, size_t tamanho, void *acumulador, FxFoldFn f,
                           void *contexto) {
    const unsigned char *base = (const unsigned char *)vetor;
    // padrão FOLD
    for (int i = 0; i < n; i++)
        f(acumulador, base + (size_t)i * tamanho, contexto);
}

#endif
