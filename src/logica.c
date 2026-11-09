// logica.c — motor de lógica proposicional. Não inclui raylib.h: roda nos testes sem janela.
#include "logica.h"
#include <stdio.h>
#include <string.h>

// ------------------------------------------------------------------ construção da árvore
void logica_pool_limpa(PoolLogico *pool) {
    pool->usados = 0;
}

static int novo_no(PoolLogico *pool, OpLogico op, int atomo, int esq, int dir) {
    if (pool->usados >= MAX_NOS_LOGICA) return -1; // pool cheio: quem chamou trata o −1
    int i = pool->usados++;
    pool->nos[i] = (NoLogico){op, atomo, esq, dir};
    return i;
}

int logica_atomo(PoolLogico *pool, int atomo) {
    return novo_no(pool, OP_ATOMO, atomo, -1, -1);
}

int logica_nao(PoolLogico *pool, int filho) {
    if (filho < 0) return -1;
    return novo_no(pool, OP_NAO, 0, filho, -1);
}

int logica_binaria(PoolLogico *pool, OpLogico op, int esq, int dir) {
    if (esq < 0 || dir < 0) return -1;
    return novo_no(pool, op, 0, esq, dir);
}

// ------------------------------------------------------------------ avaliação
// [REQ:PURA-logica_aplica] a semântica dos 5 conectivos num lugar só (os painéis também usam)
bool logica_aplica(OpLogico op, bool a, bool b) {
    switch (op) {
    case OP_NAO: return !a;
    case OP_E: return a && b;
    case OP_OU: return a || b;
    case OP_IMPLICA: return !a || b;
    case OP_SSE: return a == b;
    case OP_ATOMO: return a;
    }
    return false;
}

static bool no_valido(const PoolLogico *pool, int i) {
    return i >= 0 && i < pool->usados;
}

// [REQ:PURA-logica_avalia] avalia a árvore para uma valoração dos átomos (recursiva, sem efeitos)
bool logica_avalia(const PoolLogico *pool, int raiz, const bool *valores) {
    if (!no_valido(pool, raiz)) return false;
    const NoLogico *no = &pool->nos[raiz];
    if (no->op == OP_ATOMO) return valores[no->atomo];
    bool a = logica_avalia(pool, no->esq, valores);
    if (no->op == OP_NAO) return logica_aplica(OP_NAO, a, false);
    bool b = logica_avalia(pool, no->dir, valores);
    return logica_aplica(no->op, a, b);
}

void logica_valores_da_linha(int linha, int n_atomos, bool *valores) {
    for (int i = 0; i < n_atomos; i++) valores[i] = ((linha >> (n_atomos - 1 - i)) & 1) != 0;
}

// [REQ:PURA-logica_tabela_verdade] gera a tabela-verdade inteira; as rodadas e os painéis leem daqui
int logica_tabela_verdade(const PoolLogico *pool, int raiz, int n_atomos, bool *saida) {
    if (n_atomos < 0 || n_atomos > LOGICA_MAX_ATOMOS) return 0;
    int linhas = 1 << n_atomos;
    bool valores[LOGICA_MAX_ATOMOS] = {false};
    for (int linha = 0; linha < linhas; linha++) {
        logica_valores_da_linha(linha, n_atomos, valores);
        saida[linha] = logica_avalia(pool, raiz, valores);
    }
    return linhas;
}

// Quantas linhas da tabela dão verdadeiro? As três classificações abaixo saem daqui.
static int conta_verdadeiras(const PoolLogico *pool, int raiz, int n_atomos) {
    bool tabela[1 << LOGICA_MAX_ATOMOS];
    int linhas = logica_tabela_verdade(pool, raiz, n_atomos, tabela);
    int verdadeiras = 0;
    // padrão FOLD: soma as linhas verdadeiras
    for (int i = 0; i < linhas; i++) verdadeiras += tabela[i] ? 1 : 0;
    return verdadeiras;
}

bool logica_eh_tautologia(const PoolLogico *pool, int raiz, int n_atomos) {
    return conta_verdadeiras(pool, raiz, n_atomos) == (1 << n_atomos);
}

// [REQ:PURA-logica_eh_contradicao] a chave do final bom: nenhuma linha pode ser verdadeira
bool logica_eh_contradicao(const PoolLogico *pool, int raiz, int n_atomos) {
    return conta_verdadeiras(pool, raiz, n_atomos) == 0;
}

bool logica_eh_satisfazivel(const PoolLogico *pool, int raiz, int n_atomos) {
    return conta_verdadeiras(pool, raiz, n_atomos) > 0;
}

// ------------------------------------------------------------------ texto
static int precedencia(OpLogico op) {
    switch (op) {
    case OP_ATOMO: return 6;
    case OP_NAO: return 5;
    case OP_E: return 4;
    case OP_OU: return 3;
    case OP_IMPLICA: return 2;
    case OP_SSE: return 1;
    }
    return 0;
}

static const char *simbolo(OpLogico op) {
    switch (op) {
    case OP_E: return " \xE2\x88\xA7 ";       // ∧
    case OP_OU: return " \xE2\x88\xA8 ";      // ∨
    case OP_IMPLICA: return " \xE2\x86\x92 "; // →
    case OP_SSE: return " \xE2\x86\x94 ";     // ↔
    default: return "";
    }
}

static void anexa(char *saida, size_t tamanho, const char *texto) {
    size_t usado = strlen(saida);
    if (usado + 1 < tamanho) snprintf(saida + usado, tamanho - usado, "%s", texto);
}

static void escreve_no(const PoolLogico *pool, int i, const char *const *nomes, char *saida, size_t tamanho);

// Escreve o filho entre parênteses só quando a precedência (ou a associatividade) exige.
static void escreve_filho(const PoolLogico *pool, int filho, OpLogico pai, bool lado_direito, const char *const *nomes,
                          char *saida, size_t tamanho) {
    int pf = precedencia(pool->nos[filho].op);
    int pp = precedencia(pai);
    bool direita_assoc = pai == OP_IMPLICA;
    bool parenteses = pf < pp || (pf == pp && pai != OP_NAO && (lado_direito != direita_assoc));
    if (parenteses) anexa(saida, tamanho, "(");
    escreve_no(pool, filho, nomes, saida, tamanho);
    if (parenteses) anexa(saida, tamanho, ")");
}

static void escreve_no(const PoolLogico *pool, int i, const char *const *nomes, char *saida, size_t tamanho) {
    if (!no_valido(pool, i)) return;
    const NoLogico *no = &pool->nos[i];
    if (no->op == OP_ATOMO) {
        anexa(saida, tamanho, nomes[no->atomo]);
    } else if (no->op == OP_NAO) {
        anexa(saida, tamanho, "\xC2\xAC"); // ¬
        escreve_filho(pool, no->esq, OP_NAO, false, nomes, saida, tamanho);
    } else {
        escreve_filho(pool, no->esq, no->op, false, nomes, saida, tamanho);
        anexa(saida, tamanho, simbolo(no->op));
        escreve_filho(pool, no->dir, no->op, true, nomes, saida, tamanho);
    }
}

void logica_para_texto(const PoolLogico *pool, int raiz, const char *const *nomes, char *saida, size_t tamanho) {
    if (tamanho == 0) return;
    saida[0] = '\0';
    escreve_no(pool, raiz, nomes, saida, tamanho);
}

// ------------------------------------------------------------------ parser (precedência por escalada)
typedef struct {
    PoolLogico *pool;
    const Ficha *fichas;
    int n;
    int pos;
    ErroLogica *erro;
} Parser;

static int falha(Parser *p, int posicao, const char *mensagem) {
    if (p->erro->ok) { // guarda só o primeiro erro, que é o que explica o problema
        p->erro->ok = false;
        p->erro->posicao = posicao;
        snprintf(p->erro->mensagem, sizeof(p->erro->mensagem), "%s", mensagem);
    }
    return -1;
}

static bool eh_binaria(TipoFicha t) {
    return t == FICHA_E || t == FICHA_OU || t == FICHA_SE_ENTAO || t == FICHA_SSE;
}

static OpLogico op_da_ficha(TipoFicha t) {
    switch (t) {
    case FICHA_E: return OP_E;
    case FICHA_OU: return OP_OU;
    case FICHA_SE_ENTAO: return OP_IMPLICA;
    case FICHA_SSE: return OP_SSE;
    default: return OP_ATOMO;
    }
}

static int termo(Parser *p) {
    if (p->pos >= p->n) return falha(p, p->pos, "A frase terminou cedo: depois de um conectivo precisa vir uma palavra.");
    Ficha f = p->fichas[p->pos];
    if (f.tipo == FICHA_NAO) {
        p->pos++;
        int filho = termo(p);
        return filho < 0 ? -1 : logica_nao(p->pool, filho);
    }
    if (f.tipo == FICHA_ATOMO) {
        p->pos++;
        return logica_atomo(p->pool, f.atomo);
    }
    return falha(p, p->pos, "Um conectivo não pode vir aqui: antes dele precisa vir uma palavra.");
}

static int expressao(Parser *p, int precedencia_minima) {
    int esq = termo(p);
    while (esq >= 0 && p->pos < p->n && eh_binaria(p->fichas[p->pos].tipo)) {
        OpLogico op = op_da_ficha(p->fichas[p->pos].tipo);
        if (precedencia(op) < precedencia_minima) break;
        p->pos++;
        int proxima = op == OP_IMPLICA ? precedencia(op) : precedencia(op) + 1; // → associa à direita
        int dir = expressao(p, proxima);
        esq = dir < 0 ? -1 : logica_binaria(p->pool, op, esq, dir);
    }
    return esq;
}

// [REQ:PURA-logica_parse_fichas] fichas → árvore, com mensagem amigável quando a frase não fecha
int logica_parse_fichas(PoolLogico *pool, const Ficha *fichas, int n, ErroLogica *erro) {
    *erro = (ErroLogica){.ok = true, .posicao = -1, .mensagem = ""};
    Parser p = {pool, fichas, n, 0, erro};
    if (n <= 0) return falha(&p, 0, "A frase está vazia: escolha pelo menos uma palavra.");
    int raiz = expressao(&p, 0);
    if (raiz >= 0 && p.pos < n) return falha(&p, p.pos, "Faltou um conectivo entre duas palavras.");
    if (raiz < 0 && erro->ok) return falha(&p, p.pos, "A frase é longa demais para o analisador.");
    return raiz;
}
