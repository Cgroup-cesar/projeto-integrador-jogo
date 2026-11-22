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
        "NENHUM",
        "INICIAR",
        "INTRO_TERMINOU",
        "RODADA_CONCLUIDA",
        "VIDRO_ROMPEU",
        "SAIDA_CONCLUIDA",
        "PEGO",
        "TRANSMITIU_CONTRADICAO",
        "TRANSMITIU_SATISFAZIVEL",
        "VOLTAR_MENU",
        "PULAR_FASE",
        "IR_RESPOSTA_FINAL",
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
    case FASE_RESPOSTA_FINAL:
        return FASE_FIM_BOM;
    case FASE_FIM_RUIM:
    case FASE_FIM_BOM:
        return FASE_MENU;
    default:
        return (Fase)(fase + 1);
    }
}

// [REQ:PURA-regras_proxima_fase] a máquina de fases inteira: (fase, evento) → próxima fase
Fase regras_proxima_fase(Fase fase, EventoFase evento) {
    if (fase < 0 || fase >= FASE_TOTAL) return FASE_MENU;
    if (evento == EV_PULAR_FASE) return fase_seguinte(fase);
    if (evento == EV_VOLTAR_MENU) return FASE_MENU; // de qualquer fase: fim, pausa, inatividade
    if (evento == EV_IR_RESPOSTA_FINAL && fase >= FASE_INTRO && fase <= FASE_R3) return FASE_RESPOSTA_FINAL;
    switch (fase) {
    case FASE_MENU:
        return evento == EV_INICIAR ? FASE_INTRO : fase;
    case FASE_INTRO:
        return evento == EV_INTRO_TERMINOU ? FASE_R1 : fase;
    case FASE_R1:
        return evento == EV_RODADA_CONCLUIDA ? FASE_S1 : fase;
    case FASE_S1:
        return evento == EV_SAIDA_CONCLUIDA ? FASE_R2 : fase;
    case FASE_R2:
        return evento == EV_RODADA_CONCLUIDA ? FASE_S2 : fase;
    case FASE_S2:
        return evento == EV_SAIDA_CONCLUIDA ? FASE_R3 : fase;
    case FASE_R3:
        return evento == EV_RODADA_CONCLUIDA ? FASE_RESPOSTA_FINAL : fase;
    case FASE_RESPOSTA_FINAL:
        if (evento == EV_TRANSMITIU_CONTRADICAO) return FASE_FIM_BOM;
        if (evento == EV_TRANSMITIU_SATISFAZIVEL) return FASE_FIM_RUIM;
        return fase;
    default: // FIM_RUIM e FIM_BOM só saem com EV_VOLTAR_MENU (tratado acima)
        return fase;
    }
}

bool regras_reinicia_fase(Fase fase, EventoFase evento) {
    return (regras_fase_eh_rodada(fase) && evento == EV_VIDRO_ROMPEU) ||
           (regras_fase_eh_saida(fase) && evento == EV_PEGO);
}

int regras_roteiro_partida(bool final_bom, EventoFase *saida, int maximo) {
    const EventoFase roteiro[] = {
        EV_INICIAR,          EV_INTRO_TERMINOU,
        EV_RODADA_CONCLUIDA, EV_SAIDA_CONCLUIDA,
        EV_RODADA_CONCLUIDA, EV_SAIDA_CONCLUIDA,
        EV_RODADA_CONCLUIDA, final_bom ? EV_TRANSMITIU_CONTRADICAO : EV_TRANSMITIU_SATISFAZIVEL,
        EV_VOLTAR_MENU,
    };
    int n = (int)(sizeof(roteiro) / sizeof(roteiro[0]));
    if (n > maximo) n = maximo;
    for (int i = 0; i < n; i++)
        saida[i] = roteiro[i];
    return n;
}

EventoFase regras_evento_inatividade(bool modo_feira, Fase fase, float tempo_inativo) {
    bool jogando = fase != FASE_MENU && fase != FASE_FIM_RUIM && fase != FASE_FIM_BOM;
    return modo_feira && jogando && tempo_inativo > FEIRA_INATIVIDADE_S ? EV_VOLTAR_MENU : EV_NENHUM;
}

// ================================================================== resposta final
// [REQ:PURA-regras_avalia_transmissao] analisa a frase transmitida: inválida, contradição ou satisfazível
EventoFase regras_avalia_transmissao(const Ficha *fichas, int n, ErroLogica *erro) {
    PoolLogico pool;
    logica_pool_limpa(&pool);
    if (n > MAX_FICHAS_FRASE) {
        *erro = (ErroLogica){
            .ok = false, .posicao = MAX_FICHAS_FRASE, .mensagem = "No máximo 7 fichas por frase."};
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
bool regras_criatura_ve_jogador(Vector3 olho, Vector3 frente, Vector3 alvo, bool caminho_livre,
                                bool escondido) {
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
    case CRIATURA_PERSEGUE:
        return CRIATURA_PROCURA; // perdeu de vista: procura onde viu por último
    case CRIATURA_PATRULHA:
        return p.ouviu_ruido ? CRIATURA_INVESTIGA : CRIATURA_PATRULHA;
    case CRIATURA_INVESTIGA:
        return p.chegou_ao_destino ? CRIATURA_PROCURA : CRIATURA_INVESTIGA;
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

// [REQ:PURA-regras_postura_criatura] postura procedural: corcunda, balanço da passada e respiração
PosturaCriatura regras_postura_criatura(float velocidade, float passada, float t) {
    float ritmo = fminf(1.0f, fmaxf(0.0f, velocidade / CRIATURA_VEL_PERSEGUE)); // 0 parado, 1 perseguindo
    float andando = fminf(1.0f, ritmo * 3.0f); // o balanço já entra inteiro na patrulha
    float ciclo = passada * (2.0f * PI / CRIATURA_PASSADA_M);
    PosturaCriatura p;
    p.inclina_graus = CRIATURA_INCLINA_PARADO + (CRIATURA_INCLINA_CORRENDO - CRIATURA_INCLINA_PARADO) * ritmo;
    p.balanco_graus = sinf(ciclo) * CRIATURA_BALANCO_GRAUS * andando;
    p.sobe = fabsf(sinf(ciclo)) * CRIATURA_SOBE_M * andando;
    p.respira = 1.0f + sinf(t * 1.1f) * CRIATURA_RESPIRA * (1.0f - andando);
    return p;
}

// Número "sorteado" em [0, 1) a partir do ciclo: sem rand(), então o resultado é puro e repetível.
static float sorteio(int ciclo, unsigned semente) {
    unsigned h = (unsigned)ciclo * 2654435761u ^ semente;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (float)(h & 0xffffu) / 65536.0f;
}

// [REQ:PURA-regras_piscada_cela] lâmpada com defeito: piscada dupla em intervalos irregulares
float regras_piscada_cela(float t) {
    if (t < 0.0f) return 0.0f;
    int ciclo = (int)(t / PISCADA_INTERVALO_S);
    if (sorteio(ciclo, 0x9e37u) < PISCADA_PULA) return 0.0f; // às vezes ela não falha
    float s = t - ciclo * PISCADA_INTERVALO_S - sorteio(ciclo, 0x85ebu) * PISCADA_VARIACAO_S;
    if (s < 0.0f || s >= 0.30f) return 0.0f;
    if (s < 0.07f) return 1.0f;               // acende
    if (s < 0.13f) return 0.0f;               // apaga
    return 1.0f - (s - 0.13f) / 0.17f * 0.6f; // acende de novo, fraquejando (1,0 → 0,4)
}

// ================================================================== mixagem das trilhas
// [REQ:PURA-regras_volumes_trilhas] o volume de cada som longo sai só da situação do quadro
void regras_volumes_trilhas(SituacaoSom s, float v[TRILHA_TOTAL]) {
    for (int i = 0; i < TRILHA_TOTAL; i++) // padrão MAP: começa tudo em silêncio
        v[i] = 0.0f;
    if (s.silencio) return;
    v[TRILHA_MENU] = s.tela_inicial ? VOL_TRILHA_MENU : 0.0f;
    v[TRILHA_VOZ_PRECURSOR] = s.precursor_falando ? VOL_VOZ_PRECURSOR : 0.0f;
    v[TRILHA_VOZ_RADIO] = s.radio_falando ? VOL_VOZ_RADIO : 0.0f;
    v[TRILHA_PASSO_ANDAR] = s.andando && !s.correndo ? VOL_PASSO_ANDAR : 0.0f;
    v[TRILHA_PASSO_CORRER] = s.andando && s.correndo ? VOL_PASSO_CORRER : 0.0f;
    if (!s.criatura_solta || s.distancia_criatura >= CORACAO_LONGE_M) return;
    if (s.distancia_criatura < CORACAO_PERTO_M) {
        v[TRILHA_CORACAO_RAPIDO] = VOL_CORACAO;
        return;
    }
    float perto = (CORACAO_LONGE_M - s.distancia_criatura) / (CORACAO_LONGE_M - CORACAO_PERTO_M); // 0..1
    v[TRILHA_CORACAO_NORMAL] = VOL_CORACAO * (0.35f + 0.65f * perto);
}

// Sorteio em [−1, 1): uma posição do tremor.
static float tremida(int ciclo, unsigned semente) {
    return sorteio(ciclo, semente) * 2.0f - 1.0f;
}

// [REQ:PURA-regras_quadro_susto] o rosto pula até a câmera, freia colado nela e treme sem parar
QuadroSusto regras_quadro_susto(float t) {
    if (t < 0.0f) t = 0.0f;
    float x = fminf(1.0f, t / JUMPSCARE_PULO_S);
    float chegada = 1.0f - (1.0f - x) * (1.0f - x) * (1.0f - x); // sai rápido e freia colado na câmera
    int ciclo = (int)(t * JUMPSCARE_TREMOR_HZ);
    float forca = 0.35f + 0.65f * chegada; // no pulo já treme; colado, treme inteiro
    float depois = t - JUMPSCARE_PULO_S;
    QuadroSusto q;
    q.distancia = JUMPSCARE_LONGE_M + (JUMPSCARE_PERTO_M - JUMPSCARE_LONGE_M) * chegada;
    q.tremor_x = tremida(ciclo, 0x2f1du) * JUMPSCARE_TREMOR_M * forca;
    q.tremor_y = tremida(ciclo, 0x6b43u) * JUMPSCARE_TREMOR_M * forca;
    q.giro_graus = tremida(ciclo, 0xa7c5u) * JUMPSCARE_TREMOR_GRAUS * forca;
    q.clarao = depois < 0.0f ? 0.0f : fmaxf(0.0f, 1.0f - depois / JUMPSCARE_CLARAO_S);
    float ritmo = 2.0f * PI * JUMPSCARE_BOCA_HZ * t + 1.7f * sinf(2.0f * PI * 1.3f * t); // ritmo irregular
    q.boca = depois < 0.0f ? 1.0f : 0.5f + 0.5f * sinf(ritmo);
    return q;
}

// ================================================================== placar, vidas e interativos
// [REQ:FOLD-eventos] [REQ:PURA-regras_placar] o placar é a soma (fold) dos eventos; sem eventos, 0
int regras_placar(const EventoPlacar *eventos, int n) {
    int total = 0;
    // padrão FOLD
    for (int i = 0; i < n; i++)
        total += eventos[i].pontos;
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

// [REQ:FOLD-rachaduras] [REQ:PURA-regras_integridade_vidro] vidas = máximo − soma (fold) do dano das
// rachaduras
int regras_integridade_vidro(const Rachadura *rachaduras, int n, int vidas_max) {
    float dano = 0.0f;
    // padrão FOLD
    for (int i = 0; i < n; i++)
        dano += rachaduras[i].dano;
    int vidas = vidas_max - (int)floorf(dano + 0.01f);
    if (vidas < 0) return 0;
    return vidas > vidas_max ? vidas_max : vidas;
}

// [REQ:PURA-regras_evento_do_erro] o 3º erro (vidas = 0) rompe o vidro e recomeça a rodada
EventoFase regras_evento_do_erro(int vidas_restantes) {
    return vidas_restantes <= 0 ? EV_VIDRO_ROMPEU : EV_NENHUM;
}

bool regras_interativo_ativo(TipoInterativo tipo, int fase) {
    bool rodada = regras_fase_eh_rodada((Fase)fase) || fase == FASE_RESPOSTA_FINAL;
    bool saida = regras_fase_eh_saida((Fase)fase);
    switch (tipo) {
    case INTER_CONSOLE:
        return rodada;
    case INTER_PORTA:
        return rodada; // a porta da sala fica trancada nas fases de sala
    case INTER_PAINEL1:
        return fase == FASE_S1;
    case INTER_PAINEL2:
        return fase == FASE_S2;
    case INTER_DIARIO:
        return saida;
    case INTER_ARMARIO:
        return saida;
    }
    return false;
}

// ================================================================== circuitos dos painéis
void regras_circuito_painel1(Circuito *c) {
    *c = (Circuito){.n_entradas = 3, .n_portas = 3};
    c->portas[0] = (Porta){PORTA_NAO, 1, 0};          // ¬B
    c->portas[1] = (Porta){PORTA_E, 0, REF_PORTA(0)}; // A ∧ ¬B
    c->portas[2] = (Porta){PORTA_E, REF_PORTA(1), 2}; // (A ∧ ¬B) ∧ C
}

void regras_circuito_painel2(Circuito *c, TipoPorta p1, TipoPorta p2) {
    *c = (Circuito){.n_entradas = 2, .n_portas = 3};
    c->portas[0] = (Porta){p1, 0, 1};                       // A [?1] B
    c->portas[1] = (Porta){PORTA_NAO, 0, 0};                // ¬A
    c->portas[2] = (Porta){p2, REF_PORTA(0), REF_PORTA(1)}; // (...) [?2] (¬A)
}

// As portas do painel usam a mesma semântica do motor de lógica (OU-EXCLUSIVO = ¬(A ↔ B), NÃO-E = ¬(A ∧ B)).
static bool aplica_porta(TipoPorta tipo, bool a, bool b) {
    switch (tipo) {
    case PORTA_E:
        return logica_aplica(OP_E, a, b);
    case PORTA_OU:
        return logica_aplica(OP_OU, a, b);
    case PORTA_XOU:
        return !logica_aplica(OP_SSE, a, b);
    case PORTA_NE:
        return !logica_aplica(OP_E, a, b);
    case PORTA_NAO:
        return logica_aplica(OP_NAO, a, false);
    default:
        return false;
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
    // padrão FOLD: as portas são avaliadas em ordem, acumulando as saídas (cada uma alimenta as seguintes)
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
