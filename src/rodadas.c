// rodadas.c — as 3 rodadas na sala: decifrar o conectivo pela tabela-verdade e traduzir a frase.
// Toda resposta do Precursor sai de logica_avalia sobre a frase observada, nunca de uma tabela fixa.
#include "rodadas.h"
#include "criatura.h"
#include "audio_sintese.h"
#include "dialogo.h"
#include "ui.h"
#include <math.h>
#include <string.h>

// ================================================================== dados das rodadas
static const DadosRodada RODADAS[3] = {
    { // R1 (tutorial; só a luz A; 2 linhas)
        .teste = {{{GLIFO_NAO, GLIFO_LUZ_A}, 2}},
        .n_colunas = 1, .n_luzes = 1, .conectivo_certo = {0},
        .traducao = {{GLIFO_NAO, GLIFO_NOS, GLIFO_PERIGO}, 3}, .traducao_certa = 1,
    },
    { // R2 (luzes A e B; duas frases observadas ao mesmo tempo)
        .teste = {{{GLIFO_LUZ_A, GLIFO_E, GLIFO_LUZ_B}, 3}, {{GLIFO_LUZ_A, GLIFO_OU, GLIFO_LUZ_B}, 3}},
        .n_colunas = 2, .n_luzes = 2, .conectivo_certo = {1, 2},
        .traducao = {{GLIFO_VOCE, GLIFO_E, GLIFO_NOS, GLIFO_FALAR}, 4}, .traducao_certa = 2,
    },
    { // R3
        .teste = {{{GLIFO_LUZ_A, GLIFO_SE_ENTAO, GLIFO_LUZ_B}, 3}},
        .n_colunas = 1, .n_luzes = 2, .conectivo_certo = {3},
        .traducao = {{GLIFO_VOCE, GLIFO_FALAR, GLIFO_SE_ENTAO, GLIFO_PORTA, GLIFO_ABRIR}, 5}, .traducao_certa = 0,
    },
};

const DadosRodada *rodadas_dados(int numero) {
    if (numero < 1 || numero > 3) numero = 1;
    return &RODADAS[numero - 1];
}

int rodadas_linhas(const DadosRodada *d) {
    return 1 << d->n_luzes;
}

int rodadas_linha_atual(const Estado *e) {
    const DadosRodada *d = rodadas_dados(e->rod.numero);
    return d->n_luzes == 1 ? (e->rod.luz_a ? 1 : 0) : (e->rod.luz_a ? 2 : 0) + (e->rod.luz_b ? 1 : 0);
}

// ================================================================== avaliação pelo motor de lógica
static bool glifo_para_ficha(IdGlifo g, Ficha *f) {
    switch (g) {
    case GLIFO_LUZ_A: *f = (Ficha){FICHA_ATOMO, 0}; return true;
    case GLIFO_LUZ_B: *f = (Ficha){FICHA_ATOMO, 1}; return true;
    case GLIFO_NAO: *f = (Ficha){FICHA_NAO, 0}; return true;
    case GLIFO_E: *f = (Ficha){FICHA_E, 0}; return true;
    case GLIFO_OU: *f = (Ficha){FICHA_OU, 0}; return true;
    case GLIFO_SE_ENTAO: *f = (Ficha){FICHA_SE_ENTAO, 0}; return true;
    case GLIFO_SSE: *f = (Ficha){FICHA_SSE, 0}; return true;
    default: return false;
    }
}

// Converte os glifos em fichas, monta a árvore e avalia com as luzes atuais.
static bool avalia_frase(const FraseGlifos *frase, bool luz_a, bool luz_b) {
    Ficha fichas[MAX_GLIFOS_FRASE];
    int n = 0;
    for (int i = 0; i < frase->n; i++)
        if (glifo_para_ficha(frase->glifos[i], &fichas[n])) n++;
    PoolLogico pool;
    logica_pool_limpa(&pool);
    ErroLogica erro;
    int raiz = logica_parse_fichas(&pool, fichas, n, &erro);
    const bool valores[2] = {luz_a, luz_b};
    return raiz >= 0 && logica_avalia(&pool, raiz, valores);
}

static bool tabela_completa(const Estado *e) {
    const DadosRodada *d = rodadas_dados(e->rod.numero);
    for (int c = 0; c < d->n_colunas; c++)
        for (int l = 0; l < rodadas_linhas(d); l++)
            if (!e->rod.linha_vista[c][l]) return false;
    return true;
}

// ================================================================== início
void rodadas_inicia(Estado *e, int numero) {
    memset(&e->rod, 0, sizeof(e->rod));
    e->rod.numero = numero;
    e->rod.etapa = ETAPA_DECIFRAR;
}

static void avisa(Rodada *r, TextoId texto) {
    r->aviso = dialogo_texto(texto);
    r->aviso_t = 1.8f;
}

// ================================================================== erros, rachaduras e vidas
static void cria_rachaduras(Estado *e) {
    int quantas = GetRandomValue(1, 3);
    Vector2 origem = {GetRandomValue(25, 75) / 100.0f, GetRandomValue(18, 62) / 100.0f};
    for (int i = 0; i < quantas; i++) {
        Rachadura r = {
            .origem = origem,
            .angulo = GetRandomValue(0, 628) / 100.0f,
            .comprimento = 0.0f,
            .comprimento_alvo = GetRandomValue(16, 34) / 100.0f,
            .dano = 1.0f / quantas, // a soma do dano de um erro é 1 vida
            .galhos = GetRandomValue(2, 4),
            .semente = (unsigned)GetRandomValue(1, 1 << 20),
        };
        col_rachaduras_adiciona(&e->rachaduras, r);
    }
}

static void registra_erro(Estado *e) {
    e->erros_na_fase++;
    e->erros_totais++;
    col_eventos_adiciona(&e->eventos, (EventoPlacar){PONTO_ERRO, PONTOS_ERRO, e->fase});
    cria_rachaduras(e);
    e->vidas = regras_integridade_vidro(e->rachaduras.itens, e->rachaduras.n, VIDAS_VIDRO);
    e->falha_luz_t = 0.9f;
    avisa(&e->rod, TXT_TERMINAL_ERRO);
    audio_toca(e, SOM_VIDRO_ESTALO, 1.0f, 0.9f + GetRandomValue(0, 20) / 100.0f);
    if (e->vidas > 0) return;
    // 3º erro: o vidro rompe, vem o jumpscare e a rodada recomeça com o vidro restaurado.
    e->rod.rompendo = true;
    criatura_susto(e, EV_VIDRO_ROMPEU);
}

// ================================================================== decifrar
static void comeca_resposta(Estado *e) {
    Rodada *r = &e->rod;
    r->resposta_linha = rodadas_linha_atual(e);
    r->resposta_coluna = 0;
    r->resposta_t = RESPOSTA_PRECURSOR_S;
    e->cri.fala_t = RESPOSTA_PRECURSOR_S + 0.6f;
}

// Quando o tempo da resposta acaba, ele responde à frase da coluna atual (e à próxima, na R2).
static void termina_resposta(Estado *e) {
    Rodada *r = &e->rod;
    const DadosRodada *d = rodadas_dados(r->numero);
    int linha = r->resposta_linha;
    bool a = d->n_luzes == 1 ? (linha & 1) != 0 : (linha & 2) != 0;
    bool b = d->n_luzes == 2 && (linha & 1) != 0;
    bool valor = avalia_frase(&d->teste[r->resposta_coluna], a, b);
    r->linha_vista[r->resposta_coluna][linha] = true;
    r->linha_valor[r->resposta_coluna][linha] = valor;
    r->resposta_valor = valor;
    audio_toca(e, valor ? SOM_VERDADEIRO : SOM_FALSO, 0.9f, 1.0f);
    if (++r->resposta_coluna < d->n_colunas) {
        r->resposta_t = 0.7f;
        e->cri.fala_t = 1.2f;
    }
    if (r->resposta_coluna >= d->n_colunas && tabela_completa(e)) {
        r->etapa = ETAPA_ESCOLHER_CONECTIVO;
        r->cursor = 0;
    }
}

static void atualiza_decifrar(Estado *e, float dt) {
    Rodada *r = &e->rod;
    const DadosRodada *d = rodadas_dados(r->numero);
    if (r->resposta_t > 0.0f) {
        r->resposta_t -= dt;
        if (r->resposta_t <= 0.0f) termina_resposta(e);
        return;
    }
    if (e->in.tecla1) {
        r->luz_a = !r->luz_a;
        audio_toca(e, SOM_INTERRUPTOR, 0.8f, 1.0f);
    }
    if (e->in.tecla2 && d->n_luzes == 2) {
        r->luz_b = !r->luz_b;
        audio_toca(e, SOM_INTERRUPTOR, 0.8f, 1.1f);
    }
    // A cada combinação nova, ele responde e a linha da tabela se preenche sozinha.
    if (!r->linha_vista[0][rodadas_linha_atual(e)]) comeca_resposta(e);
}

// ================================================================== escolher o conectivo e traduzir
static bool move_cursor(Rodada *r, const Entrada *in, int opcoes) {
    if (in->cima) r->cursor = (r->cursor + opcoes - 1) % opcoes;
    if (in->baixo) r->cursor = (r->cursor + 1) % opcoes;
    return in->confirmar;
}

static void atualiza_escolha_conectivo(Estado *e) {
    Rodada *r = &e->rod;
    const DadosRodada *d = rodadas_dados(r->numero);
    if (!move_cursor(r, &e->in, CONECTIVOS_ESCOLHA)) return;
    if (r->cursor != d->conectivo_certo[r->coluna]) {
        registra_erro(e);
        return;
    }
    e->conectivo_decifrado[d->conectivo_certo[r->coluna]] = true;
    avisa(r, TXT_TERMINAL_CERTO);
    audio_toca(e, SOM_VERDADEIRO, 0.6f, 1.25f);
    r->cursor = 0;
    if (++r->coluna < d->n_colunas) return;
    r->etapa = ETAPA_TRADUZIR;
    audio_toca(e, SOM_VOZ_PRECURSOR, 1.0f, 1.0f);
}

static void atualiza_traducao(Estado *e) {
    Rodada *r = &e->rod;
    e->cri.fala_t = fmaxf(e->cri.fala_t, 0.5f); // enquanto espera a tradução, ele continua "dizendo" a frase
    if (!move_cursor(r, &e->in, 3)) return;
    if (r->cursor != rodadas_dados(r->numero)->traducao_certa) {
        registra_erro(e);
        return;
    }
    avisa(r, TXT_TERMINAL_CERTO);
    r->etapa = ETAPA_CONCLUIDA;
    r->concluida_t = 0.0f;
    e->cri.fala_t = 0.0f;
    ui_fala(e, r->numero == 1 ? FALAS_APOS_R1 : r->numero == 2 ? FALAS_APOS_R2 : FALAS_APOS_R3);
    if (r->numero == 3) {
        e->alarme_t = 6.0f;
        audio_toca(e, SOM_ALARME, 0.8f, 1.0f);
    }
}

// ================================================================== depois de concluir: a cena entre as fases
// R1: ele dá um passo em direção ao vidro, as luzes piscam e vem o apagão. R2: apagão maior.
// R3: alarme, ele encosta a mão no vidro e o terminal mostra RESPONDA.
static void atualiza_conclusao(Estado *e, float dt) {
    Rodada *r = &e->rod;
    float antes = r->concluida_t;
    r->concluida_t += dt;
    if (r->numero == 1) e->cri.avanco_vidro = fminf(1.0f, r->concluida_t / 2.5f);
    if (r->numero == 3 && r->concluida_t > 1.5f) e->cri.mao_no_vidro = true;
    float apagao_em = r->numero == 3 ? 99.0f : 3.2f;
    float fim = r->numero == 1 ? 5.0f : r->numero == 2 ? 6.0f : 5.0f;
    if (antes < 2.2f && r->concluida_t >= 2.2f && r->numero != 3) e->falha_luz_t = 1.0f;
    if (antes < apagao_em && r->concluida_t >= apagao_em) {
        e->apagao_t = r->numero == 1 ? 1.8f : 2.8f;
        audio_toca(e, SOM_ESTRONDO, 0.5f, 0.6f);
    }
    if (r->concluida_t >= fim) e->evento = EV_RODADA_CONCLUIDA;
}

// ================================================================== atualiza
void rodadas_atualiza(Estado *e, float dt) {
    Rodada *r = &e->rod;
    r->aviso_t = fmaxf(0.0f, r->aviso_t - dt);
    col_rachaduras_atualiza(&e->rachaduras, dt);
    if (r->rompendo || e->jumpscare_t > 0.0f || e->silencio_t > 0.0f) return;
    if (r->etapa == ETAPA_CONCLUIDA) {
        atualiza_conclusao(e, dt);
        return;
    }
    if (e->jog.modo != MODO_TERMINAL) return;
    if (e->in.voltar) {
        e->jog.modo = MODO_ANDANDO;
        return;
    }
    switch (r->etapa) {
    case ETAPA_DECIFRAR: atualiza_decifrar(e, dt); break;
    case ETAPA_ESCOLHER_CONECTIVO: atualiza_escolha_conectivo(e); break;
    case ETAPA_TRADUZIR: atualiza_traducao(e); break;
    default: break;
    }
}

// ================================================================== o que o vidro mostra
int rodadas_frases_atuais(const Estado *e, FraseGlifos *frases, int maximo) {
    if (!regras_fase_eh_rodada(e->fase) || maximo <= 0) return 0;
    const Rodada *r = &e->rod;
    const DadosRodada *d = rodadas_dados(r->numero);
    if (r->etapa == ETAPA_TRADUZIR) {
        frases[0] = d->traducao;
        return 1;
    }
    if (r->etapa == ETAPA_CONCLUIDA) return 0;
    int n = d->n_colunas < maximo ? d->n_colunas : maximo;
    for (int i = 0; i < n; i++) frases[i] = d->teste[i];
    return n;
}

// ================================================================== rachaduras (por cima da cena)
static unsigned proximo(unsigned *semente) {
    *semente = *semente * 1103515245u + 12345u;
    return (*semente >> 16) & 0x7fff;
}

// Um galho é uma linha quebrada; os pontos saem de uma semente fixa, então ele não "pisca".
static void desenha_galho(Vector2 inicio, float angulo, float comprimento, unsigned semente, float esp, Color cor) {
    Vector2 p = inicio;
    int passos = 6;
    for (int i = 0; i < passos; i++) {
        angulo += ((int)(proximo(&semente) % 100) - 50) / 160.0f;
        Vector2 q = {p.x + cosf(angulo) * comprimento / passos, p.y + sinf(angulo) * comprimento / passos};
        DrawLineEx(p, q, esp, cor);
        p = q;
    }
}

void rodadas_desenha(const Estado *e) {
    float w = (float)e->rec.alvo_largura, h = (float)e->rec.alvo_altura;
    Color cor = Fade((Color){0xd8, 0xee, 0xff, 255}, 0.75f);
    for (int i = 0; i < e->rachaduras.n; i++) {
        const Rachadura *r = &e->rachaduras.itens[i];
        Vector2 origem = {r->origem.x * w, r->origem.y * h};
        float comprimento = r->comprimento * h;
        unsigned semente = r->semente;
        desenha_galho(origem, r->angulo, comprimento, semente, 1.6f, cor);
        for (int g = 0; g < r->galhos; g++) {
            float ang = r->angulo + ((int)(proximo(&semente) % 200) - 100) / 90.0f;
            desenha_galho(origem, ang, comprimento * 0.55f, semente + g, 1.0f, Fade(cor, 0.6f));
        }
    }
}

// ================================================================== piloto automático (--smoke)
static void piloto_decifrar(const Estado *e, Entrada *in) {
    const Rodada *r = &e->rod;
    const DadosRodada *d = rodadas_dados(r->numero);
    if (r->resposta_t > 0.0f) return;
    for (int l = 0; l < rodadas_linhas(d); l++) {
        if (r->linha_vista[0][l]) continue;
        bool a = d->n_luzes == 1 ? (l & 1) != 0 : (l & 2) != 0;
        bool b = d->n_luzes == 2 && (l & 1) != 0;
        if (r->luz_a != a) in->tecla1 = true;
        else if (r->luz_b != b) in->tecla2 = true;
        return;
    }
}

static void piloto_escolhe(const Rodada *r, Entrada *in, int alvo) {
    if (r->cursor < alvo) in->baixo = true;
    else if (r->cursor > alvo) in->cima = true;
    else in->confirmar = true;
}

void rodadas_piloto(const Estado *e, Entrada *in, int erros_desejados) {
    const Rodada *r = &e->rod;
    const DadosRodada *d = rodadas_dados(r->numero);
    bool errar = e->erros_na_fase < erros_desejados;
    switch (r->etapa) {
    case ETAPA_DECIFRAR: piloto_decifrar(e, in); break;
    case ETAPA_ESCOLHER_CONECTIVO: {
        int certo = d->conectivo_certo[r->coluna];
        piloto_escolhe(r, in, errar ? (certo + 1) % CONECTIVOS_ESCOLHA : certo);
        break;
    }
    case ETAPA_TRADUZIR: piloto_escolhe(r, in, errar ? (d->traducao_certa + 1) % 3 : d->traducao_certa); break;
    default: break;
    }
}
