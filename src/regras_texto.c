// regras_texto.c — as regras puras de texto: marcadores do roteiro ({NOME}, {PROF}, {x|y}), a edição do nome
// digitado e a legenda com defeito do rádio falso. Separado de regras.c para os módulos ficarem com ~500
// linhas; as declarações continuam em regras.h. Como regras.c, não inclui raylib.h e é testado sem janela.
#include "regras.h"
#include <string.h>

// ================================================================== textos do roteiro (nome e gênero)
// Quantos bytes tem a letra UTF-8 que começa em `p` (1 a 4), sem passar do '\0'.
static size_t bytes_da_letra(const char *p) {
    unsigned char c = (unsigned char)p[0];
    size_t esperado = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
    size_t k = 1;
    while (k < esperado && p[k] != '\0')
        k++;
    return k;
}

// Anexa `n` bytes à saída só se couberem inteiros (com o '\0' no fim). Devolve false quando não cabem.
static bool anexa(char *saida, size_t tamanho, size_t *j, const char *trecho, size_t n) {
    if (*j + n + 1 > tamanho) return false;
    memcpy(saida + *j, trecho, n);
    *j += n;
    return true;
}

// Anexa um trecho letra por letra: se faltar espaço, corta entre duas letras, nunca no meio de um acento.
static bool anexa_letras(char *saida, size_t tamanho, size_t *j, const char *trecho, size_t n) {
    size_t i = 0;
    while (i < n) {
        size_t b = bytes_da_letra(trecho + i);
        if (b > n - i) b = n - i;
        if (!anexa(saida, tamanho, j, trecho + i, b)) return false;
        i += b;
    }
    return true;
}

// O que entra no lugar de "{conteudo}" (n bytes, sem as chaves): a escolha "x|y" pelo gênero ou o valor do
// marcador com essa chave. NULL quando o marcador é desconhecido.
static const char *valor_do_marcador(const char *conteudo, size_t n, const Marcador *marcadores, int total,
                                     bool feminino, size_t *tam_valor) {
    const char *barra = memchr(conteudo, '|', n);
    if (barra != NULL) {
        size_t antes = (size_t)(barra - conteudo);
        *tam_valor = feminino ? n - antes - 1 : antes;
        return feminino ? barra + 1 : conteudo;
    }
    const char *achado = NULL;
    for (int i = 0; i < total; i++) // padrão FOLD: acha o valor do marcador com essa chave
        if (strlen(marcadores[i].chave) == n && strncmp(marcadores[i].chave, conteudo, n) == 0)
            achado = marcadores[i].valor;
    if (achado != NULL) *tam_valor = strlen(achado);
    return achado;
}

// [REQ:PURA-regras_monta_texto] troca {NOME}, {PROF}... e {x|y} nas falas: o roteiro fica em dialogo.c
void regras_monta_texto(const char *modelo, const Marcador *marcadores, int n, bool feminino, char *saida,
                        size_t tamanho) {
    if (saida == NULL || tamanho == 0) return;
    size_t j = 0;
    bool cabe = true;
    const char *p = modelo != NULL ? modelo : "";
    while (*p != '\0' && cabe) {
        const char *fecha = *p == '{' ? strchr(p, '}') : NULL;
        size_t tam_valor = 0;
        const char *valor =
            fecha ? valor_do_marcador(p + 1, (size_t)(fecha - p - 1), marcadores, n, feminino, &tam_valor)
                  : NULL;
        if (valor != NULL) {
            cabe = anexa_letras(saida, tamanho, &j, valor, tam_valor);
            p = fecha + 1;
        } else { // texto comum (ou marcador desconhecido): copia uma letra
            size_t b = bytes_da_letra(p);
            cabe = anexa(saida, tamanho, &j, p, b);
            p += b;
        }
    }
    saida[j] = '\0';
}

// Letras aceitas no nome: A–Z, a–z, as acentuadas do Latin-1 (de À a ÿ, menos × e ÷), espaço, hífen e
// apóstrofo. A fonte do jogo tem todo o Latin-1, então tudo o que entra aqui aparece na tela.
static bool aceita_no_nome(int codigo) {
    bool ascii = (codigo >= 'A' && codigo <= 'Z') || (codigo >= 'a' && codigo <= 'z') || codigo == ' ' ||
                 codigo == '-' || codigo == '\'';
    return ascii || (codigo >= 0xC0 && codigo <= 0xFF && codigo != 0xD7 && codigo != 0xF7);
}

static int conta_letras(const char *texto) {
    int n = 0;
    for (const char *p = texto; *p != '\0'; p++) // padrão FOLD: conta os bytes que começam uma letra
        if (((unsigned char)*p & 0xC0) != 0x80) n++;
    return n;
}

// Primeira letra de cada palavra em maiúscula: "inês" vira "Inês" (o nome aparece como "Professora Inês").
// No Latin-1, as minúsculas acentuadas (à a þ, menos ÷) ficam 0x20 acima das maiúsculas, como no ASCII.
static int maiuscula(int codigo) {
    if (codigo >= 'a' && codigo <= 'z') return codigo - 0x20;
    if (codigo >= 0xE0 && codigo <= 0xFE && codigo != 0xF7) return codigo - 0x20;
    return codigo;
}

// [REQ:PURA-regras_nome_edita] edita o nome digitado no início (letras com acento, Backspace e limite)
bool regras_nome_edita(char *nome, size_t tamanho, int max_letras, int codigo) {
    size_t n = strlen(nome);
    if (codigo == NOME_APAGAR) {
        if (n == 0) return false;
        do { // volta até o primeiro byte da última letra (uma letra acentuada tem 2 bytes)
            n--;
        } while (n > 0 && ((unsigned char)nome[n] & 0xC0) == 0x80);
        nome[n] = '\0';
        return true;
    }
    bool espaco_sobrando = codigo == ' ' && (n == 0 || nome[n - 1] == ' ');
    if (!aceita_no_nome(codigo) || espaco_sobrando || conta_letras(nome) >= max_letras) return false;
    if (n == 0 || nome[n - 1] == ' ') codigo = maiuscula(codigo); // começo de palavra
    char utf8[2] = {(char)codigo, 0};
    size_t b = 1;
    if (codigo >= 0x80) { // de U+0080 a U+07FF, o UTF-8 usa 2 bytes: 110xxxxx 10xxxxxx
        utf8[0] = (char)(0xC0 | (codigo >> 6));
        utf8[1] = (char)(0x80 | (codigo & 0x3F));
        b = 2;
    }
    if (n + b + 1 > tamanho) return false;
    memcpy(nome + n, utf8, b);
    nome[n + b] = '\0';
    return true;
}

// ================================================================== legenda com defeito (rádio falso)
// Número em [0, 1) a partir de dois inteiros: um hash, não rand(), para a função ser pura e repetível.
static float sorteio_texto(int a, int b) {
    unsigned h = (unsigned)a * 2654435761u ^ (unsigned)b * 2246822519u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (float)(h & 0xffffu) / 65536.0f;
}

// [REQ:PURA-regras_falha_texto] a legenda do rádio falso falha: letras viram símbolos por um instante
bool regras_falha_texto(const char *texto, float t, char *saida, size_t tamanho) {
    static const char simbolos[] = "#%&*/<>?_|";
    if (saida == NULL || tamanho == 0) return false;
    int ciclo = t < 0.0f ? 0 : (int)(t / FALHA_INTERVALO_S);
    float s = t - ciclo * FALHA_INTERVALO_S - sorteio_texto(ciclo, 7) * (FALHA_INTERVALO_S - FALHA_DURACAO_S);
    bool falhando = t >= 0.0f && s >= 0.0f && s < FALHA_DURACAO_S;
    size_t j = 0;
    for (int i = 0; texto != NULL && texto[i] != '\0' && j + 1 < tamanho;
         i++) { // padrão MAP: letra por letra
        unsigned char c = (unsigned char)texto[i];
        bool letra = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        bool troca = falhando && letra && sorteio_texto(ciclo * 131 + i, 13) < FALHA_LETRAS;
        saida[j++] = troca ? simbolos[(int)(sorteio_texto(ciclo, i) * 10.0f) % 10] : texto[i];
    }
    saida[j] = '\0';
    return falhando;
}
