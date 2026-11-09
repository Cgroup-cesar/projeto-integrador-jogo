// test_logica.c — motor de lógica proposicional: conectivos, classificações, parser e os dois painéis.
#include "checagem.h"
#include "logica.h"
#include "regras.h"
#include <string.h>

static const char *const NOMES[] = {"P", "Q", "R"};

// Monta "P op Q" num pool novo e devolve a raiz.
static int binaria(PoolLogico *pool, OpLogico op) {
    logica_pool_limpa(pool);
    return logica_binaria(pool, op, logica_atomo(pool, 0), logica_atomo(pool, 1));
}

static void confere_tabela(OpLogico op, bool l00, bool l01, bool l10, bool l11) {
    PoolLogico pool;
    bool t[4];
    int raiz = binaria(&pool, op);
    int linhas = logica_tabela_verdade(&pool, raiz, 2, t);
    CHECK(linhas == 4);
    CHECK(t[0] == l00 && t[1] == l01 && t[2] == l10 && t[3] == l11);
}

static void testa_conectivos(void) {
    PoolLogico pool;
    logica_pool_limpa(&pool);
    bool t[2];
    CHECK(logica_tabela_verdade(&pool, logica_nao(&pool, logica_atomo(&pool, 0)), 1, t) == 2);
    CHECK(t[0] == true && t[1] == false); // ¬P
    confere_tabela(OP_E, false, false, false, true);
    confere_tabela(OP_OU, false, true, true, true);
    confere_tabela(OP_IMPLICA, true, true, false, true);
    confere_tabela(OP_SSE, true, false, false, true);
}

static void testa_classificacoes(void) {
    PoolLogico pool;
    logica_pool_limpa(&pool);
    int p = logica_atomo(&pool, 0);
    int contradicao = logica_binaria(&pool, OP_E, p, logica_nao(&pool, logica_atomo(&pool, 0)));
    CHECK(logica_eh_contradicao(&pool, contradicao, 1));
    CHECK(!logica_eh_satisfazivel(&pool, contradicao, 1));

    int tautologia =
        logica_binaria(&pool, OP_OU, logica_atomo(&pool, 0), logica_nao(&pool, logica_atomo(&pool, 0)));
    CHECK(logica_eh_tautologia(&pool, tautologia, 1));

    int implica = logica_binaria(&pool, OP_IMPLICA, logica_atomo(&pool, 0), logica_atomo(&pool, 1));
    CHECK(logica_eh_satisfazivel(&pool, implica, 2));
    CHECK(!logica_eh_tautologia(&pool, implica, 2));
    CHECK(!logica_eh_contradicao(&pool, implica, 2));

    char texto[64];
    logica_para_texto(&pool, contradicao, NOMES, texto, sizeof(texto));
    CHECK(strcmp(texto, "P \xE2\x88\xA7 \xC2\xACP") == 0); // "P ∧ ¬P"
}

static int analisa(PoolLogico *pool, const Ficha *f, int n, ErroLogica *erro) {
    logica_pool_limpa(pool);
    return logica_parse_fichas(pool, f, n, erro);
}

static void testa_parser_precedencia(void) {
    PoolLogico pool;
    ErroLogica erro;
    const Ficha P = {FICHA_ATOMO, 0}, Q = {FICHA_ATOMO, 1};
    const Ficha NAO = {FICHA_NAO, 0}, E = {FICHA_E, 0}, OU = {FICHA_OU, 0}, SE = {FICHA_SE_ENTAO, 0};

    Ficha f1[] = {NAO, P, E, Q}; // (¬P) ∧ Q, não ¬(P ∧ Q)
    int r1 = analisa(&pool, f1, 4, &erro);
    CHECK(erro.ok && r1 >= 0 && pool.nos[r1].op == OP_E);

    Ficha f2[] = {P, OU, Q, E, P}; // P ∨ (Q ∧ P): o E pesa mais
    int r2 = analisa(&pool, f2, 5, &erro);
    CHECK(erro.ok && pool.nos[r2].op == OP_OU && pool.nos[pool.nos[r2].dir].op == OP_E);

    Ficha f3[] = {P, SE, Q, SE, P}; // → associa à direita: P → (Q → P), que é tautologia
    int r3 = analisa(&pool, f3, 5, &erro);
    CHECK(erro.ok && logica_eh_tautologia(&pool, r3, 2));

    Ficha f4[] = {P, E, Q, OU, P}; // (P ∧ Q) ∨ P: à esquerda
    int r4 = analisa(&pool, f4, 5, &erro);
    CHECK(erro.ok && pool.nos[r4].op == OP_OU && pool.nos[pool.nos[r4].esq].op == OP_E);

    const Ficha SSE = {FICHA_SSE, 0};
    Ficha f6[] = {P, OU, Q, SE, P}; // (P ∨ Q) → P: o OU pesa mais que o SE…ENTÃO
    int r6 = analisa(&pool, f6, 5, &erro);
    CHECK(erro.ok && pool.nos[r6].op == OP_IMPLICA && pool.nos[pool.nos[r6].esq].op == OP_OU);

    Ficha f7[] = {P, SE, Q, SSE, Q}; // (P → Q) ↔ Q: o SE…ENTÃO pesa mais que o SE E SÓ SE
    int r7 = analisa(&pool, f7, 5, &erro);
    CHECK(erro.ok && pool.nos[r7].op == OP_SSE && pool.nos[pool.nos[r7].esq].op == OP_IMPLICA);

    Ficha f5[] = {P, E, NAO, P}; // a resposta do final bom
    int r5 = analisa(&pool, f5, 4, &erro);
    CHECK(erro.ok && logica_eh_contradicao(&pool, r5, 2));
}

static void testa_parser_erros(void) {
    PoolLogico pool;
    ErroLogica erro;
    const Ficha P = {FICHA_ATOMO, 0}, Q = {FICHA_ATOMO, 1}, NAO = {FICHA_NAO, 0}, E = {FICHA_E, 0};

    CHECK(analisa(&pool, NULL, 0, &erro) < 0 && !erro.ok && erro.mensagem[0] != '\0'); // vazia
    Ficha f1[] = {E, P};
    CHECK(analisa(&pool, f1, 2, &erro) < 0 && !erro.ok && erro.posicao == 0); // começa com conectivo
    Ficha f2[] = {P, E};
    CHECK(analisa(&pool, f2, 2, &erro) < 0 && !erro.ok && erro.posicao == 2); // termina cedo
    Ficha f3[] = {P, Q};
    CHECK(analisa(&pool, f3, 2, &erro) < 0 && !erro.ok && erro.posicao == 1); // falta conectivo
    Ficha f4[] = {NAO};
    CHECK(analisa(&pool, f4, 1, &erro) < 0 && !erro.ok);
    Ficha f5[] = {P, E, E, Q};
    CHECK(analisa(&pool, f5, 4, &erro) < 0 && !erro.ok && erro.posicao == 2);
}

// Painel 2: (A [?1] B) [?2] (¬A) com alvo 00→1, 01→1, 10→0, 11→1. Há exatamente duas soluções.
static void testa_painel2(void) {
    const bool alvo[4] = {true, true, false, true};
    int solucoes = 0;
    bool e_ou = false, e_xou = false;
    for (int p1 = PORTA_E; p1 <= PORTA_NE; p1++) {
        for (int p2 = PORTA_E; p2 <= PORTA_NE; p2++) {
            Circuito c;
            regras_circuito_painel2(&c, (TipoPorta)p1, (TipoPorta)p2);
            if (!regras_circuito_confere_tabela(&c, alvo)) continue;
            solucoes++;
            e_ou = e_ou || (p1 == PORTA_E && p2 == PORTA_OU);
            e_xou = e_xou || (p1 == PORTA_E && p2 == PORTA_XOU);
        }
    }
    CHECK(solucoes == 2);
    CHECK(e_ou && e_xou);
}

// Painel 1: (A ∧ ¬B) ∧ C acende só com A=1, B=0, C=1.
static void testa_painel1(void) {
    Circuito c;
    regras_circuito_painel1(&c);
    int acesas = 0;
    bool unica_certa = false;
    for (int linha = 0; linha < 8; linha++) {
        bool entradas[3], leds[MAX_PORTAS];
        logica_valores_da_linha(linha, 3, entradas);
        if (!regras_avaliar_circuito(&c, entradas, leds)) continue;
        acesas++;
        unica_certa = entradas[0] && !entradas[1] && entradas[2] && leds[0] && leds[1];
    }
    CHECK(acesas == 1);
    CHECK(unica_certa);
}

static void testa_pool_cheio(void) {
    PoolLogico pool;
    logica_pool_limpa(&pool);
    int ultimo = 0;
    for (int i = 0; i < MAX_NOS_LOGICA + 5; i++)
        ultimo = logica_atomo(&pool, 0);
    CHECK(ultimo == -1);
    CHECK(pool.usados == MAX_NOS_LOGICA);
    CHECK(logica_nao(&pool, -1) == -1);
}

int main(void) {
    testa_conectivos();
    testa_classificacoes();
    testa_parser_precedencia();
    testa_parser_erros();
    testa_painel1();
    testa_painel2();
    testa_pool_cheio();
    return resultado_final("test_logica");
}
