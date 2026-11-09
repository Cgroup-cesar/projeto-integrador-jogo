// logica.h — motor de lógica proposicional (puro: sem raylib.h, sem globais, sem efeitos colaterais).
// É o coração do jogo: as tabelas-verdade das rodadas, os painéis e a resposta final passam por aqui.
#ifndef LOGICA_H
#define LOGICA_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

typedef enum { OP_ATOMO, OP_NAO, OP_E, OP_OU, OP_IMPLICA, OP_SSE } OpLogico;

// Um nó da árvore da expressão. Os filhos são índices dentro do pool (−1 = sem filho).
typedef struct {
    OpLogico op;
    int atomo; // usado quando op == OP_ATOMO (0 = P ou A, 1 = Q ou B, 2 = C)
    int esq;
    int dir;
} NoLogico;

// Pool estático: os nós moram num vetor de tamanho fixo dentro da struct, sem malloc.
typedef struct {
    NoLogico nos[MAX_NOS_LOGICA];
    int usados;
} PoolLogico;

// Fichas: as "palavras" que o jogador encaixa na resposta final (e que as frases das rodadas usam).
typedef enum { FICHA_ATOMO, FICHA_NAO, FICHA_E, FICHA_OU, FICHA_SE_ENTAO, FICHA_SSE } TipoFicha;

typedef struct {
    TipoFicha tipo;
    int atomo; // só para FICHA_ATOMO
} Ficha;

typedef struct {
    bool ok;
    int posicao;         // índice da ficha onde o problema foi notado
    char mensagem[120];  // explicação amigável, em português
} ErroLogica;

#define LOGICA_MAX_ATOMOS 4

// ------------------------------------------------------------------ construção da árvore
void logica_pool_limpa(PoolLogico *pool);
int logica_atomo(PoolLogico *pool, int atomo);                       // devolve o índice do nó ou −1 (pool cheio)
int logica_nao(PoolLogico *pool, int filho);
int logica_binaria(PoolLogico *pool, OpLogico op, int esq, int dir);

// ------------------------------------------------------------------ avaliação
bool logica_aplica(OpLogico op, bool a, bool b); // a semântica de cada conectivo, num lugar só
bool logica_avalia(const PoolLogico *pool, int raiz, const bool *valores);
void logica_valores_da_linha(int linha, int n_atomos, bool *valores); // linha 2 com 2 átomos → A=1, B=0
int logica_tabela_verdade(const PoolLogico *pool, int raiz, int n_atomos, bool *saida); // devolve 2^n
bool logica_eh_tautologia(const PoolLogico *pool, int raiz, int n_atomos);
bool logica_eh_contradicao(const PoolLogico *pool, int raiz, int n_atomos);
bool logica_eh_satisfazivel(const PoolLogico *pool, int raiz, int n_atomos);

// ------------------------------------------------------------------ texto e parser
// Escreve a expressão com ¬ ∧ ∨ → ↔ (UTF-8) e só os parênteses necessários.
void logica_para_texto(const PoolLogico *pool, int raiz, const char *const *nomes, char *saida, size_t tamanho);

// Gramática: expr := termo (BINOP termo)*   termo := NAO termo | ATOMO
// Precedência ¬ > ∧ > ∨ > → > ↔; associatividade à esquerda, exceto → (à direita).
// Devolve a raiz ou −1; em caso de erro, `erro` explica o que faltou.
int logica_parse_fichas(PoolLogico *pool, const Ficha *fichas, int n, ErroLogica *erro);

#endif
