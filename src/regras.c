// regras.c — regras do jogo como funções puras. Não inclui raylib.h: só raymath.h, que é só de cabeçalho.
#include "regras.h"
#include <math.h>
#include <string.h>

// ================================================================== fases
const char *regras_nome_fase(Fase fase) {
    static const char *const nomes[FASE_TOTAL] = {
        "MENU", "INTRO", "R1", "S1", "R2", "S2", "R3", "RESPOSTA_FINAL", "FIM_RUIM", "FIM_BOM",
    };
    if (fase < 0 || fase >= FASE_TOTAL) return "?";
    return nomes[fase];
}

const char *regras_nome_evento(EventoFase evento) {
    static const char *const nomes[EV_TOTAL] = {
        "NENHUM",       "INICIAR",         "INTRO_TERMINOU", "RODADA_CONCLUIDA",        "VIDRO_ROMPEU",
        "SAIDA_CONCLUIDA", "PEGO",         "TRANSMITIU_CONTRADICAO", "TRANSMITIU_SATISFAZIVEL",
        "VOLTAR_MENU",  "PULAR_FASE",      "IR_RESPOSTA_FINAL",
    };
    if (evento < 0 || evento >= EV_TOTAL) return "?";
    return nomes[evento];
}

bool regras_fase_eh_rodada(Fase fase) {
    return fase == FASE_R1 || fase == FASE_R2 || fase == FASE_R3;
}

bool regras_fase_eh_saida(Fase fase) {
    return fase == FASE_S1 || fase == FASE_S2;
}

int regras_numero_rodada(Fase fase) {
    return fase == FASE_R1 ? 1 : fase == FASE_R2 ? 2 : fase == FASE_R3 ? 3 : 0;
}

// Debug (F2): a fase seguinte na ordem da partida, sem precisar resolver nada.
static Fase fase_seguinte(Fase fase) {
    switch (fase) {
    case FASE_RESPOSTA_FINAL: return FASE_FIM_BOM;
    case FASE_FIM_RUIM:
    case FASE_FIM_BOM: return FASE_MENU;
    default: return (Fase)(fase + 1);
    }
}

// [REQ:PURA-regras_proxima_fase] a máquina de fases inteira: (fase, evento) → próxima fase
Fase regras_proxima_fase(Fase fase, EventoFase evento) {
    if (fase < 0 || fase >= FASE_TOTAL) return FASE_MENU;
    if (evento == EV_PULAR_FASE) return fase_seguinte(fase);
    if (evento == EV_IR_RESPOSTA_FINAL && fase >= FASE_INTRO && fase <= FASE_R3) return FASE_RESPOSTA_FINAL;
    switch (fase) {
    case FASE_MENU: return evento == EV_INICIAR ? FASE_INTRO : fase;
    case FASE_INTRO: return evento == EV_INTRO_TERMINOU ? FASE_R1 : fase;
    case FASE_R1: return evento == EV_RODADA_CONCLUIDA ? FASE_S1 : fase;
    case FASE_S1: return evento == EV_SAIDA_CONCLUIDA ? FASE_R2 : fase;
    case FASE_R2: return evento == EV_RODADA_CONCLUIDA ? FASE_S2 : fase;
    case FASE_S2: return evento == EV_SAIDA_CONCLUIDA ? FASE_R3 : fase;
    case FASE_R3: return evento == EV_RODADA_CONCLUIDA ? FASE_RESPOSTA_FINAL : fase;
    case FASE_RESPOSTA_FINAL:
        if (evento == EV_TRANSMITIU_CONTRADICAO) return FASE_FIM_BOM;
        if (evento == EV_TRANSMITIU_SATISFAZIVEL) return FASE_FIM_RUIM;
        return fase;
    case FASE_FIM_RUIM:
    case FASE_FIM_BOM: return evento == EV_VOLTAR_MENU ? FASE_MENU : fase;
    default: return FASE_MENU;
    }
}

bool regras_reinicia_fase(Fase fase, EventoFase evento) {
    return (regras_fase_eh_rodada(fase) && evento == EV_VIDRO_ROMPEU) ||
           (regras_fase_eh_saida(fase) && evento == EV_PEGO);
}

int regras_roteiro_partida(bool final_bom, EventoFase *saida, int maximo) {
    const EventoFase roteiro[] = {
        EV_INICIAR,          EV_INTRO_TERMINOU,   EV_RODADA_CONCLUIDA,
        EV_SAIDA_CONCLUIDA,  EV_RODADA_CONCLUIDA, EV_SAIDA_CONCLUIDA,
        EV_RODADA_CONCLUIDA, final_bom ? EV_TRANSMITIU_CONTRADICAO : EV_TRANSMITIU_SATISFAZIVEL,
        EV_VOLTAR_MENU,
    };
    int n = (int)(sizeof(roteiro) / sizeof(roteiro[0]));
    if (n > maximo) n = maximo;
    for (int i = 0; i < n; i++) saida[i] = roteiro[i];
    return n;
}

// ================================================================== resposta final
// [REQ:PURA-regras_avalia_transmissao] analisa a frase transmitida: inválida, contradição ou satisfazível
EventoFase regras_avalia_transmissao(const Ficha *fichas, int n, ErroLogica *erro) {
    PoolLogico pool;
    logica_pool_limpa(&pool);
    if (n > MAX_FICHAS_FRASE) {
        *erro = (ErroLogica){.ok = false, .posicao = MAX_FICHAS_FRASE, .mensagem = "No máximo 7 fichas por frase."};
        return EV_NENHUM;
    }
    int raiz = logica_parse_fichas(&pool, fichas, n, erro);
    if (raiz < 0) return EV_NENHUM;
    if (logica_eh_contradicao(&pool, raiz, 2)) return EV_TRANSMITIU_CONTRADICAO;
    return EV_TRANSMITIU_SATISFAZIVEL;
}

// ================================================================== movimento e o Precursor
Vector3 regras_frente(float yaw) {
    return (Vector3){sinf(yaw), 0.0f, cosf(yaw)};
}

// [REQ:PURA-regras_nova_posicao] posição seguinte a partir da entrada; o movimento é multiplicado por dt
Vector3 regras_nova_posicao(Vector3 pos, float yaw, Vector2 mover, float velocidade, float dt) {
    Vector3 frente = regras_frente(yaw);
    Vector3 direita = {-frente.z, 0.0f, frente.x}; // frente × cima
    Vector3 direcao = Vector3Add(Vector3Scale(frente, mover.y), Vector3Scale(direita, mover.x));
    float tamanho = Vector3Length(direcao);
    if (tamanho < 0.0001f) return pos;
    // Andar na diagonal não é mais rápido: a direção é normalizada antes de multiplicar.
    return Vector3Add(pos, Vector3Scale(direcao, velocidade * dt / tamanho));
}

// [REQ:PURA-regras_criatura_ve_jogador] cone de 70° e 14 m; o raycast contra as paredes chega pronto
bool regras_criatura_ve_jogador(Vector3 olho, Vector3 frente, Vector3 alvo, bool caminho_livre, bool escondido) {
    if (escondido || !caminho_livre) return false;
    Vector3 para_alvo = Vector3Subtract(alvo, olho);
    float distancia = Vector3Length(para_alvo);
    if (distancia > CRIATURA_VISAO_DIST) return false;
    if (distancia < 0.0001f) return true;
    float cosseno = Vector3DotProduct(Vector3Normalize(frente), para_alvo) / distancia;
    return cosseno >= cosf(CRIATURA_VISAO_GRAUS * 0.5f * DEG2RAD);
}

// [REQ:PURA-regras_ruido_percebido] o ruído enfraquece com a distância e some no raio dele
float regras_ruido_percebido(Ruido ruido, Vector3 ouvinte) {
    if (ruido.raio <= 0.0f || ruido.intensidade <= 0.0f) return 0.0f;
    float distancia = Vector3Distance(ruido.pos, ouvinte);
    if (distancia >= ruido.raio) return 0.0f;
    return ruido.intensidade * (1.0f - distancia / ruido.raio);
}

// [REQ:PURA-regras_proximo_estado_criatura] PATRULHA → INVESTIGA → PERSEGUE → PROCURA → PATRULHA
EstadoCriatura regras_proximo_estado_criatura(EstadoCriatura atual, PercepcaoCriatura p) {
    if (p.ve_jogador) return CRIATURA_PERSEGUE;
    switch (atual) {
    case CRIATURA_PERSEGUE: return CRIATURA_PROCURA; // perdeu de vista: procura onde viu por último
    case CRIATURA_PATRULHA: return p.ouviu_ruido ? CRIATURA_INVESTIGA : CRIATURA_PATRULHA;
    case CRIATURA_INVESTIGA: return p.chegou_ao_destino ? CRIATURA_PROCURA : CRIATURA_INVESTIGA;
    case CRIATURA_PROCURA:
        if (p.ouviu_ruido) return CRIATURA_INVESTIGA;
        return p.tempo_no_estado >= CRIATURA_PROCURA_S ? CRIATURA_PATRULHA : CRIATURA_PROCURA;
    }
    return CRIATURA_PATRULHA;
}

const char *regras_nome_estado_criatura(EstadoCriatura estado) {
    static const char *const nomes[] = {"PATRULHA", "INVESTIGA", "PERSEGUE", "PROCURA"};
    if (estado < 0 || estado > CRIATURA_PROCURA) return "?";
    return nomes[estado];
}

// ================================================================== placar, vidas e interativos
// [REQ:FOLD-eventos] [REQ:PURA-regras_placar] o placar é a soma (fold) dos eventos; sem eventos, 0
int regras_placar(const EventoPlacar *eventos, int n) {
    int total = 0;
    // padrão FOLD
    for (int i = 0; i < n; i++) total += eventos[i].pontos;
    return total;
}

// [REQ:FILTER-eventos] pontos de uma fase: filter (compactação numa cópia) + fold
int regras_placar_da_fase(const EventoPlacar *eventos, int n, int fase) {
    EventoPlacar copia[MAX_EVENTOS];
    if (n > MAX_EVENTOS) n = MAX_EVENTOS;
    memcpy(copia, eventos, sizeof(EventoPlacar) * (size_t)n);
    int j = 0;
    // padrão FILTER
    for (int i = 0; i < n; i++)
        if (copia[i].fase == fase) copia[j++] = copia[i];
    return regras_placar(copia, j);
}

int regras_bonus_tempo(float tempo_total) {
    float sobra = TEMPO_ALVO_PARTIDA_S - tempo_total;
    return sobra > 0.0f ? (int)(sobra * 0.5f) : 0;
}

// [REQ:FOLD-rachaduras] [REQ:PURA-regras_integridade_vidro] vidas = máximo − soma (fold) do dano das rachaduras
int regras_integridade_vidro(const Rachadura *rachaduras, int n, int vidas_max) {
    float dano = 0.0f;
    // padrão FOLD
    for (int i = 0; i < n; i++) dano += rachaduras[i].dano;
    int vidas = vidas_max - (int)floorf(dano + 0.01f);
    if (vidas < 0) return 0;
    return vidas > vidas_max ? vidas_max : vidas;
}

bool regras_interativo_ativo(TipoInterativo tipo, int fase) {
    bool rodada = regras_fase_eh_rodada((Fase)fase) || fase == FASE_RESPOSTA_FINAL;
    bool saida = regras_fase_eh_saida((Fase)fase);
    switch (tipo) {
    case INTER_CONSOLE: return rodada;
    case INTER_PORTA: return rodada; // a porta da sala fica trancada nas fases de sala
    case INTER_PAINEL1: return fase == FASE_S1;
    case INTER_PAINEL2: return fase == FASE_S2;
    case INTER_DIARIO: return saida;
    case INTER_ARMARIO: return saida;
    }
    return false;
}

// ================================================================== circuitos dos painéis
void regras_circuito_painel1(Circuito *c) {
    *c = (Circuito){.n_entradas = 3, .n_portas = 3};
    c->portas[0] = (Porta){PORTA_NAO, 1, 0};                       // ¬B
    c->portas[1] = (Porta){PORTA_E, 0, REF_PORTA(0)};              // A ∧ ¬B
    c->portas[2] = (Porta){PORTA_E, REF_PORTA(1), 2};              // (A ∧ ¬B) ∧ C
}

void regras_circuito_painel2(Circuito *c, TipoPorta p1, TipoPorta p2) {
    *c = (Circuito){.n_entradas = 2, .n_portas = 3};
    c->portas[0] = (Porta){p1, 0, 1};                              // A [?1] B
    c->portas[1] = (Porta){PORTA_NAO, 0, 0};                       // ¬A
    c->portas[2] = (Porta){p2, REF_PORTA(0), REF_PORTA(1)};        // (...) [?2] (¬A)
}

// As portas do painel usam a mesma semântica do motor de lógica (OU-EXCLUSIVO = ¬(A ↔ B), NÃO-E = ¬(A ∧ B)).
static bool aplica_porta(TipoPorta tipo, bool a, bool b) {
    switch (tipo) {
    case PORTA_E: return logica_aplica(OP_E, a, b);
    case PORTA_OU: return logica_aplica(OP_OU, a, b);
    case PORTA_XOU: return !logica_aplica(OP_SSE, a, b);
    case PORTA_NE: return !logica_aplica(OP_E, a, b);
    case PORTA_NAO: return logica_aplica(OP_NAO, a, false);
    default: return false;
    }
}

static bool valor_da_referencia(int ref, const bool *entradas, const bool *portas, int portas_prontas) {
    if (ref < REF_PORTA(0)) return entradas[ref];
    int i = ref - REF_PORTA(0);
    return i < portas_prontas ? portas[i] : false;
}

// [REQ:PURA-regras_avaliar_circuito] avalia porta a porta (cada LED) e devolve a lâmpada de saída
bool regras_avaliar_circuito(const Circuito *c, const bool *entradas, bool *leds) {
    bool portas[MAX_PORTAS] = {false};
    for (int i = 0; i < c->n_portas && i < MAX_PORTAS; i++) {
        bool a = valor_da_referencia(c->portas[i].a, entradas, portas, i);
        bool b = valor_da_referencia(c->portas[i].b, entradas, portas, i);
        portas[i] = aplica_porta(c->portas[i].tipo, a, b);
        if (leds != NULL) leds[i] = portas[i];
    }
    return c->n_portas > 0 ? portas[c->n_portas - 1] : false;
}

bool regras_circuito_confere_tabela(const Circuito *c, const bool *alvo) {
    bool entradas[LOGICA_MAX_ATOMOS] = {false};
    int linhas = 1 << c->n_entradas;
    for (int linha = 0; linha < linhas; linha++) {
        logica_valores_da_linha(linha, c->n_entradas, entradas);
        if (regras_avaliar_circuito(c, entradas, NULL) != alvo[linha]) return false;
    }
    return true;
}
