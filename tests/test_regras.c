// test_regras.c — regras puras e coleções, incluindo as bordas: lista vazia, vetor cheio, ruído esgotado,
// criatura sem ruídos e placar sem eventos. Roda sem abrir janela.
#include "raylib.h"
#include "checagem.h"
#include "regras.h"
#include "colecoes.h"
#include "materiais.h"
#include <math.h>
#include <string.h>

static bool perto(float a, float b) {
    return fabsf(a - b) < 0.001f;
}

// [REQ:PURA-regras_nova_posicao] testa a posição nova: frente, parado, diagonal e proporcional ao dt
static void testa_movimento(void) {
    Vector3 origem = {0, 0, 0};
    Vector3 frente = regras_nova_posicao(origem, 0.0f, (Vector2){0, 1}, 2.5f, 0.1f);
    CHECK(perto(frente.z, 0.25f) && perto(frente.x, 0.0f));
    Vector3 parado = regras_nova_posicao(origem, 0.0f, (Vector2){0, 0}, 2.5f, 0.1f);
    CHECK(perto(parado.x, 0) && perto(parado.z, 0));
    Vector3 diagonal = regras_nova_posicao(origem, 0.3f, (Vector2){1, 1}, 2.0f, 0.5f);
    CHECK(perto(Vector3Length(diagonal), 1.0f)); // diagonal não é mais rápida
    Vector3 meio_dt = regras_nova_posicao(origem, 0.0f, (Vector2){0, 1}, 2.5f, 0.05f);
    CHECK(perto(meio_dt.z, 0.125f)); // movimento proporcional ao dt
}

// [REQ:PURA-regras_criatura_ve_jogador] testa o cone, a distância, a parede no meio e o esconderijo
static void testa_visao(void) {
    Vector3 olho = {0, 1.6f, 0}, frente = {0, 0, 1};
    CHECK(regras_criatura_ve_jogador(olho, frente, (Vector3){0, 1.6f, 5}, true, false));
    CHECK(!regras_criatura_ve_jogador(olho, frente, (Vector3){0, 1.6f, -5}, true, false)); // atrás
    CHECK(!regras_criatura_ve_jogador(olho, frente, (Vector3){0, 1.6f, 20}, true, false)); // longe demais
    CHECK(!regras_criatura_ve_jogador(olho, frente, (Vector3){5, 1.6f, 1}, true, false));  // fora do cone
    CHECK(!regras_criatura_ve_jogador(olho, frente, (Vector3){0, 1.6f, 5}, false, false)); // parede no meio
    CHECK(!regras_criatura_ve_jogador(olho, frente, (Vector3){0, 1.6f, 5}, true, true));   // escondido
    CHECK(regras_criatura_ve_jogador(olho, frente, (Vector3){2, 1.6f, 5}, true, false));   // dentro dos 35°
}

// [REQ:PURA-regras_ruido_percebido] testa a atenuação pela distância e o ruído esgotado
static void testa_ruido_percebido(void) {
    Ruido r = {{0, 0, 0}, 10.0f, 1.0f, RUIDO_CORRIDA};
    CHECK(perto(regras_ruido_percebido(r, (Vector3){0, 0, 0}), 1.0f));
    CHECK(perto(regras_ruido_percebido(r, (Vector3){5, 0, 0}), 0.5f));
    CHECK(perto(regras_ruido_percebido(r, (Vector3){12, 0, 0}), 0.0f));
    r.intensidade = 0.0f;
    CHECK(perto(regras_ruido_percebido(r, (Vector3){1, 0, 0}), 0.0f)); // esgotado
}

// [REQ:PURA-regras_proximo_estado_criatura] testa as transições PATRULHA, INVESTIGA, PERSEGUE e PROCURA
static void testa_estados_criatura(void) {
    PercepcaoCriatura nada = {0};
    PercepcaoCriatura viu = {.ve_jogador = true};
    PercepcaoCriatura ouviu = {.ouviu_ruido = true};
    PercepcaoCriatura chegou = {.chegou_ao_destino = true};
    PercepcaoCriatura cansou = {.tempo_no_estado = CRIATURA_PROCURA_S};
    CHECK(regras_proximo_estado_criatura(CRIATURA_PATRULHA, nada) == CRIATURA_PATRULHA);
    CHECK(regras_proximo_estado_criatura(CRIATURA_PATRULHA, ouviu) == CRIATURA_INVESTIGA);
    CHECK(regras_proximo_estado_criatura(CRIATURA_INVESTIGA, viu) == CRIATURA_PERSEGUE);
    CHECK(regras_proximo_estado_criatura(CRIATURA_INVESTIGA, chegou) == CRIATURA_PROCURA);
    CHECK(regras_proximo_estado_criatura(CRIATURA_PERSEGUE, nada) == CRIATURA_PROCURA);
    CHECK(regras_proximo_estado_criatura(CRIATURA_PROCURA, nada) == CRIATURA_PROCURA);
    CHECK(regras_proximo_estado_criatura(CRIATURA_PROCURA, cansou) == CRIATURA_PATRULHA);
    CHECK(regras_proximo_estado_criatura(CRIATURA_PROCURA, ouviu) == CRIATURA_INVESTIGA);
}

// [REQ:PURA-regras_postura_criatura] testa a postura parado, andando e perseguindo
static void testa_postura_criatura(void) {
    PosturaCriatura parado = regras_postura_criatura(0.0f, 0.4f, 1.0f);
    CHECK(perto(parado.balanco_graus, 0.0f) && perto(parado.sobe, 0.0f)); // parado não balança
    CHECK(perto(parado.inclina_graus, CRIATURA_INCLINA_PARADO));
    CHECK(fabsf(parado.respira - 1.0f) <= CRIATURA_RESPIRA + 0.0001f); // respira só um pouco
    float quarto = CRIATURA_PASSADA_M / 4.0f;                          // pico do balanço
    PosturaCriatura anda = regras_postura_criatura(CRIATURA_VEL_PATRULHA, quarto, 1.0f);
    CHECK(perto(anda.balanco_graus, CRIATURA_BALANCO_GRAUS) && perto(anda.sobe, CRIATURA_SOBE_M));
    PosturaCriatura persegue = regras_postura_criatura(CRIATURA_VEL_PERSEGUE, quarto, 1.0f);
    CHECK(persegue.inclina_graus > anda.inclina_graus); // mais rápido, mais curvado
    CHECK(perto(persegue.inclina_graus, CRIATURA_INCLINA_CORRENDO));
    CHECK(perto(persegue.respira, 1.0f)); // andando não respira
    PosturaCriatura rapido = regras_postura_criatura(99.0f, quarto, 1.0f);
    CHECK(perto(rapido.inclina_graus, CRIATURA_INCLINA_CORRENDO)); // não passa do limite
}

// [REQ:PURA-regras_piscada_cela] testa a lâmpada com defeito: rara, curta, irregular e repetível
static void testa_piscada_cela(void) {
    int acendeu = 0, quadros_acesos = 0, quadros = 0;
    bool na_faixa = true;
    float antes = 0.0f;
    for (int i = 0; i < 6000; i++, quadros++) { // 60 s a cada 0,01 s
        float v = regras_piscada_cela(i * 0.01f);
        if (v < 0.0f || v > 1.0f) na_faixa = false;
        if (v > 0.0f && antes <= 0.0f) acendeu++; // cada piscada dupla acende duas vezes
        if (v > 0.0f) quadros_acesos++;
        antes = v;
    }
    CHECK(na_faixa);
    CHECK(acendeu / 2 >= 12 && acendeu / 2 <= 22);  // ~3 s em média: entre 12 e 22 piscadas em 60 s
    CHECK(quadros_acesos < quadros / 10);           // acesa menos de 10% do tempo
    CHECK(perto(regras_piscada_cela(-1.0f), 0.0f)); // tempo negativo: apagada
    CHECK(regras_piscada_cela(12.34f) == regras_piscada_cela(12.34f)); // repetível
}

// [REQ:PURA-regras_quadro_susto] testa o pulo até a câmera, o tremor e o clarão
static void testa_quadro_susto(void) {
    QuadroSusto inicio = regras_quadro_susto(0.0f);
    CHECK(perto(inicio.distancia, JUMPSCARE_LONGE_M) && perto(inicio.clarao, 0.0f)); // começa longe
    QuadroSusto chegou = regras_quadro_susto(JUMPSCARE_PULO_S);
    CHECK(perto(chegou.distancia, JUMPSCARE_PERTO_M) && perto(chegou.clarao, 1.0f)); // colado, com clarão
    CHECK(perto(regras_quadro_susto(-1.0f).distancia, JUMPSCARE_LONGE_M));           // tempo negativo
    bool so_aproxima = true, no_limite = true;
    int mudou = 0;
    QuadroSusto antes = inicio;
    for (int i = 1; i <= 200; i++) { // 2 s a cada 0,01 s
        QuadroSusto q = regras_quadro_susto(i * 0.01f);
        if (q.distancia > antes.distancia + 0.0001f) so_aproxima = false;
        if (fabsf(q.tremor_x) > JUMPSCARE_TREMOR_M || fabsf(q.tremor_y) > JUMPSCARE_TREMOR_M ||
            fabsf(q.giro_graus) > JUMPSCARE_TREMOR_GRAUS)
            no_limite = false;
        if (!perto(q.tremor_x, antes.tremor_x)) mudou++;
        antes = q;
    }
    CHECK(so_aproxima && no_limite);
    CHECK(perto(antes.distancia, JUMPSCARE_PERTO_M) && perto(antes.clarao, 0.0f)); // fica colado, sem clarão
    CHECK(mudou >= (int)(2.0f * JUMPSCARE_TREMOR_HZ) - 4); // treme rápido: ~40 posições novas por segundo
    CHECK(regras_quadro_susto(0.77f).tremor_y == regras_quadro_susto(0.77f).tremor_y); // repetível
    CHECK(perto(regras_quadro_susto(JUMPSCARE_PULO_S * 0.5f).boca, 1.0f)); // pula de boca escancarada
    float menor = 1.0f, maior = 0.0f;
    for (int i = 0; i < 100; i++) { // 1 s depois do pulo: a boca abre e fecha
        float b = regras_quadro_susto(JUMPSCARE_PULO_S + i * 0.01f).boca;
        menor = fminf(menor, b);
        maior = fmaxf(maior, b);
    }
    CHECK(menor >= 0.0f && maior <= 1.0f && menor < 0.15f && maior > 0.85f);
}

// [REQ:PURA-regras_placar] testa o placar sem eventos, o total e o placar por fase
static void testa_placar(void) {
    CHECK(regras_placar(NULL, 0) == 0); // placar sem eventos
    EventoPlacar ev[] = {
        {PONTO_RODADA_SEM_ERRO, 100, FASE_R1}, {PONTO_ERRO, -25, FASE_R2},
        {PONTO_SAIDA_LIMPA, 50, FASE_S1},      {PONTO_ERRO, -25, FASE_R2},
        {PONTO_FINAL_BOM, 200, FASE_FIM_BOM},
    };
    CHECK(regras_placar(ev, 5) == 300);
    CHECK(regras_placar_da_fase(ev, 5, FASE_R2) == -50);
    CHECK(regras_placar_da_fase(ev, 5, FASE_S2) == 0);
    CHECK(regras_bonus_tempo(TEMPO_ALVO_PARTIDA_S + 10) == 0);
    CHECK(regras_bonus_tempo(TEMPO_ALVO_PARTIDA_S - 100) == 50);
}

// [REQ:PURA-regras_evento_do_erro] testa que só o 3º erro rompe o vidro
static void testa_evento_do_erro(void) {
    CHECK(regras_evento_do_erro(2) == EV_NENHUM);
    CHECK(regras_evento_do_erro(1) == EV_NENHUM);
    CHECK(regras_evento_do_erro(0) == EV_VIDRO_ROMPEU);
}

// [REQ:PURA-regras_integridade_vidro] testa as vidas depois de 0, 1, 2 e 3 erros
static void testa_vidro(void) {
    CHECK(regras_integridade_vidro(NULL, 0, VIDAS_VIDRO) == 3); // sem rachaduras
    Rachadura r[6] = {0};
    r[0].dano = 0.5f;
    r[1].dano = 0.5f; // 1º erro em duas rachaduras
    CHECK(regras_integridade_vidro(r, 2, VIDAS_VIDRO) == 2);
    r[2].dano = 1.0f / 3.0f;
    r[3].dano = 1.0f / 3.0f;
    r[4].dano = 1.0f / 3.0f; // 2º erro em três rachaduras
    CHECK(regras_integridade_vidro(r, 5, VIDAS_VIDRO) == 1);
    r[5].dano = 1.0f;
    CHECK(regras_integridade_vidro(r, 6, VIDAS_VIDRO) == 0); // 3º erro: o vidro rompe
}

static void testa_colecao_ruidos(void) {
    ColRuidos c = {0};
    CHECK(col_ruidos_mais_forte(&c, (Vector3){0, 0, 0}, NULL) == -1); // criatura sem ruídos (lista vazia)
    col_ruidos_atualiza(&c, 0.1f);                                    // lista vazia não quebra
    CHECK(c.n == 0);
    for (int i = 0; i < MAX_RUIDOS + 3; i++)
        col_ruidos_adiciona(&c, (Ruido){{(float)i, 0, 0}, 3, 1, RUIDO_PASSO});
    CHECK(c.n == MAX_RUIDOS); // vetor cheio: descarta sem travar
    CHECK(!col_ruidos_adiciona(&c, (Ruido){{0, 0, 0}, 3, 1, RUIDO_PASSO}));

    ColRuidos d = {0};
    col_ruidos_adiciona(&d, (Ruido){{0, 0, 0}, 3.0f, 1.0f, RUIDO_PASSO});
    col_ruidos_adiciona(&d, (Ruido){{8, 0, 0}, 10.0f, 1.0f, RUIDO_CORRIDA});
    float intensidade = 0;
    CHECK(col_ruidos_mais_forte(&d, (Vector3){7, 0, 0}, &intensidade) == 1); // o da corrida, mais perto
    CHECK(intensidade > 0.8f);
    col_ruidos_atualiza(&d, 2.0f); // ruído esgotado sai do vetor
    CHECK(d.n == 0);
}

static void testa_colecao_eventos(void) {
    ColEventos c = {0};
    col_eventos_acumula(&c, (EventoPlacar){PONTO_ERRO, -25, FASE_R2});
    col_eventos_acumula(&c, (EventoPlacar){PONTO_ERRO, -25, FASE_R2}); // mesmo tipo e fase: soma no último
    CHECK(c.n == 1 && c.itens[0].pontos == -50);
    col_eventos_acumula(&c, (EventoPlacar){PONTO_ERRO, -25, FASE_R3}); // fase nova: evento novo
    CHECK(c.n == 2);
    for (int i = 0; i < MAX_EVENTOS + 5; i++)
        col_eventos_adiciona(&c, (EventoPlacar){PONTO_RODADA_SEM_ERRO, 1, FASE_R1});
    CHECK(c.n == MAX_EVENTOS); // vetor cheio: descarta sem travar
    CHECK(regras_placar(c.itens, c.n) == -75 + (MAX_EVENTOS - 2));
}

static void testa_colecao_particulas(void) {
    ColParticulas c = {0};
    col_particulas_adiciona(&c, (Particula){{0, 1, 0}, {0, 0, 0}, 1.0f, 1.0f, PARTICULA_POEIRA});
    col_particulas_adiciona(&c,
                            (Particula){{0, 1, 0}, {100, 0, 0}, 5.0f, 5.0f, PARTICULA_FAGULHA}); // vai sair
    col_particulas_adiciona(&c,
                            (Particula){{0, 1, 0}, {0, 0, 0}, 0.05f, 1.0f, PARTICULA_POEIRA}); // vai morrer
    col_particulas_atualiza(&c, 0.1f, (Vector3){-5, -5, -5}, (Vector3){5, 5, 5});
    CHECK(c.n == 1);
    CHECK(c.itens[0].vida < 1.0f);
}

static void testa_colecao_rachaduras(void) {
    ColRachaduras c = {0};
    col_rachaduras_adiciona(&c, (Rachadura){.comprimento = 0, .comprimento_alvo = 0.3f, .dano = 1});
    col_rachaduras_atualiza(&c, 0.05f);
    CHECK(c.itens[0].comprimento > 0.0f && c.itens[0].comprimento < 0.3f);
    for (int i = 0; i < 40; i++)
        col_rachaduras_atualiza(&c, 0.05f);
    CHECK(perto(c.itens[0].comprimento, 0.3f));
}

static void testa_colecao_paredes(void) {
    ColParedes c = {0};
    CHECK(!col_paredes_colide(&c, (Vector3){0, 0, 0}, 0.3f, 1.8f)); // sem paredes
    col_paredes_adiciona(&c, (Parede){{2, 0, -1}, {4, 3, 1}, PAREDE_MURO});
    CHECK(!col_paredes_colide(&c, (Vector3){0, 0, 0}, 0.3f, 1.8f));
    CHECK(col_paredes_colide(&c, (Vector3){1.8f, 0, 0}, 0.3f, 1.8f));
    CHECK(perto(col_paredes_distancia_raio(&c, (Vector3){0, 1, 0}, (Vector3){1, 0, 0}, 14.0f), 2.0f));
    CHECK(perto(col_paredes_distancia_raio(&c, (Vector3){0, 1, 0}, (Vector3){-1, 0, 0}, 14.0f), 14.0f));
}

static void testa_colecao_interativos(void) {
    ColInterativos c = {0};
    col_interativos_adiciona(&c, (Interativo){INTER_CONSOLE, {0, 1, 2}, {0.5f, 0.5f, 0.3f}, {0, 0, -1}});
    col_interativos_adiciona(&c, (Interativo){INTER_ARMARIO, {0, 1, -2}, {0.5f, 1, 0.3f}, {0, 0, 1}});
    col_interativos_adiciona(&c, (Interativo){INTER_PAINEL1, {3, 1, 0}, {0.5f, 0.5f, 0.1f}, {-1, 0, 0}});
    CHECK(col_interativos_na_mira(&c, (Vector3){0, 1, 0}, (Vector3){0, 0, 1}, 3.0f) == 0);
    CHECK(col_interativos_na_mira(&c, (Vector3){0, 1, 0}, (Vector3){0, 0, -1}, 3.0f) == 1);
    CHECK(col_interativos_na_mira(&c, (Vector3){0, 1, 0}, (Vector3){0, 1, 0}, 3.0f) ==
          -1);                                // olhando pro teto
    col_interativos_filtra_fase(&c, FASE_S1); // na saída 1: armário e painel 1
    CHECK(c.n == 2 && c.itens[0].tipo == INTER_ARMARIO && c.itens[1].tipo == INTER_PAINEL1);
    col_interativos_filtra_fase(&c, FASE_MENU);
    CHECK(c.n == 0);
}

// ================================================================== nome de quem joga
// [REQ:PURA-regras_nome] testa a normalização: vazio, espaços, inválidos, acentos, limite e UTF-8 inteiro
static void testa_nome_normaliza(void) {
    char s[NOME_CAPACIDADE];
    CHECK(regras_nome_normaliza("", s, sizeof(s)) == 0 && s[0] == '\0');
    CHECK(regras_nome_normaliza("     ", s, sizeof(s)) == 0 && s[0] == '\0');
    CHECK(regras_nome_normaliza(NULL, s, sizeof(s)) == 0 && s[0] == '\0');
    CHECK(regras_nome_normaliza("  Ana   Maria  ", s, sizeof(s)) == 9 && strcmp(s, "Ana Maria") == 0);
    // acentos, hífen e apóstrofo valem: "João D'Ávila-Sá" tem 15 letras e 18 bytes
    CHECK(regras_nome_normaliza("Jo\xC3\xA3o D'\xC3\x81vila-S\xC3\xA1", s, sizeof(s)) == 15);
    CHECK(strcmp(s, "Jo\xC3\xA3o D'\xC3\x81vila-S\xC3\xA1") == 0);
    CHECK(regras_nome_normaliza("R2-D2", s, sizeof(s)) == 5 && strcmp(s, "R2-D2") == 0);
    // inválidos somem: pontuação, tabulação, emoji (4 bytes), × e ÷, UTF-8 quebrado
    CHECK(regras_nome_normaliza("Ana!@#$%<>", s, sizeof(s)) == 3 && strcmp(s, "Ana") == 0);
    CHECK(regras_nome_normaliza("a\tb", s, sizeof(s)) == 2 && strcmp(s, "ab") == 0);
    CHECK(regras_nome_normaliza("Z\xC3\xA9\xF0\x9F\x98\x80", s, sizeof(s)) == 2);
    CHECK(strcmp(s, "Z\xC3\xA9") == 0);
    CHECK(regras_nome_normaliza("2\xC3\x97"
                                "2",
                                s, sizeof(s)) == 2 &&
          strcmp(s, "22") == 0);
    CHECK(regras_nome_normaliza("A\xC3", s, sizeof(s)) == 1 && strcmp(s, "A") == 0);
    CHECK(regras_nome_normaliza("\xC3(B", s, sizeof(s)) == 1 && strcmp(s, "B") == 0);
    CHECK(regras_nome_normaliza("!!! ", s, sizeof(s)) == 0 && s[0] == '\0');
}

static void testa_nome_limite(void) {
    char s[NOME_CAPACIDADE];
    CHECK(regras_nome_normaliza("ABCDEFGHIJKLMNOP", s, sizeof(s)) == NOME_MAX_CHARS); // no limite
    CHECK(strcmp(s, "ABCDEFGHIJKLMNOP") == 0);
    CHECK(regras_nome_normaliza("ABCDEFGHIJKLMNOPQRS", s, sizeof(s)) == NOME_MAX_CHARS); // acima: corta
    CHECK(strcmp(s, "ABCDEFGHIJKLMNOP") == 0);
    CHECK(regras_nome_normaliza("ABCDEFGHIJKLMNO PQ", s, sizeof(s)) == 15); // não termina em espaço
    CHECK(strcmp(s, "ABCDEFGHIJKLMNO") == 0);
    // 17 letras de 2 bytes: ficam 16 letras inteiras (32 bytes), nenhuma partida ao meio
    const char *acentos = "\xC3\x81\xC3\x89\xC3\x8D\xC3\x93\xC3\x9A\xC3\x82\xC3\x8A\xC3\x94"
                          "\xC3\x83\xC3\x95\xC3\x87\xC3\x80\xC3\x9C\xC3\x91\xC3\xA1\xC3\xA9\xC3\xAD";
    CHECK(regras_nome_normaliza(acentos, s, sizeof(s)) == NOME_MAX_CHARS);
    CHECK(strlen(s) == 32 && strncmp(s, acentos, 32) == 0);
    CHECK(regras_nome_conta_letras(s) == NOME_MAX_CHARS);
    // cap pequeno: só cabe o que cabe inteiro
    char p[4];
    CHECK(regras_nome_normaliza("\xC3\x81\xC3\x89\xC3\x8D", p, sizeof(p)) == 1 && strcmp(p, "\xC3\x81") == 0);
    CHECK(regras_nome_normaliza("Ana", p, 3) == 2 && strcmp(p, "An") == 0);
    CHECK(regras_nome_normaliza("\xC3\x89", p, 2) == 0 && p[0] == '\0');
    CHECK(regras_nome_normaliza("Ana", p, 1) == 0 && p[0] == '\0');
    CHECK(regras_nome_normaliza("Ana", p, 0) == 0); // cap zero: não escreve nada
}

static void testa_nome_padrao(void) {
    char s[NOME_CAPACIDADE];
    regras_nome_define("", s, sizeof(s), "VISITANTE");
    CHECK(strcmp(s, "VISITANTE") == 0);
    regras_nome_define("    ", s, sizeof(s), "VISITANTE");
    CHECK(strcmp(s, "VISITANTE") == 0);
    regras_nome_define("@#!", s, sizeof(s), "VISITANTE");
    CHECK(strcmp(s, "VISITANTE") == 0);
    regras_nome_define(" Bia ", s, sizeof(s), "VISITANTE");
    CHECK(strcmp(s, "Bia") == 0);
    // no mesmo buffer (como em jogo.c): o que sobrou depois do '\0' é zerado
    memset(s, 0, sizeof(s));
    strcpy(s, "   Lu   Mar   ");
    regras_nome_define(s, s, sizeof(s), "VISITANTE");
    CHECK(strcmp(s, "Lu Mar") == 0);
    int sujos = 0;
    // padrão FOLD
    for (size_t i = strlen(s); i < sizeof(s); i++)
        sujos += s[i] != '\0';
    CHECK(sujos == 0);
}

static void testa_nome_digitacao(void) {
    CHECK(regras_nome_caractere_valido('a') && regras_nome_caractere_valido('Z'));
    CHECK(regras_nome_caractere_valido('7') && regras_nome_caractere_valido(' '));
    CHECK(regras_nome_caractere_valido('-') && regras_nome_caractere_valido('\''));
    CHECK(regras_nome_caractere_valido(0xE7) && regras_nome_caractere_valido(0xC3)); // ç, Ã
    CHECK(!regras_nome_caractere_valido('!') && !regras_nome_caractere_valido('.'));
    CHECK(!regras_nome_caractere_valido('_'));
    CHECK(!regras_nome_caractere_valido(0xD7) && !regras_nome_caractere_valido(0xF7)); // × ÷
    CHECK(!regras_nome_caractere_valido(0x1F600) && !regras_nome_caractere_valido(-1));
    char s[NOME_CAPACIDADE] = "";
    CHECK(regras_nome_acrescenta(s, sizeof(s), 'A'));
    CHECK(regras_nome_acrescenta(s, sizeof(s), 0xE7));
    CHECK(strcmp(s, "A\xC3\xA7") == 0);
    CHECK(!regras_nome_acrescenta(s, sizeof(s), '!'));
    regras_nome_apaga_ultimo(s); // o ç sai inteiro (2 bytes)
    CHECK(strcmp(s, "A") == 0);
    regras_nome_apaga_ultimo(s);
    regras_nome_apaga_ultimo(s); // apagar com o campo vazio não quebra nada
    CHECK(s[0] == '\0');
    int aceitos = 0;
    // padrão FOLD: digita 20 "É"; só NOME_MAX_CHARS entram
    for (int i = 0; i < 20; i++)
        aceitos += regras_nome_acrescenta(s, sizeof(s), 0xC9);
    CHECK(aceitos == NOME_MAX_CHARS && regras_nome_conta_letras(s) == NOME_MAX_CHARS && strlen(s) == 32);
    char p[4] = "";
    CHECK(regras_nome_acrescenta(p, sizeof(p), 0xC9));  // 2 bytes + '\0' cabem
    CHECK(!regras_nome_acrescenta(p, sizeof(p), 0xC9)); // mais 2 não
}

// ================================================================== texturas procedurais (materiais.c)
// [REQ:PURA-materiais] testa o ruído (faixa, determinismo, repetição sem emenda) e os materiais
static void testa_ruido_materiais(void) {
    CHECK(materiais_hash(3, 7, 1985u) == materiais_hash(3, 7, 1985u)); // mesma entrada, mesmo número
    CHECK(materiais_hash(3, 7, 1985u) != materiais_hash(3, 7, 1986u)); // a semente muda o resultado
    int fora = 0;
    // padrão FOLD: conta amostras fora de [0, 1)
    for (int i = 0; i < 200; i++) {
        float r = materiais_ruido(i * 0.37f, i * 0.11f, 8, 8, 7u),
              f = materiais_fbm(i / 200.0f, 0.5f, 4, 4, 4, 7u);
        fora += r < 0.0f || r >= 1.0f || f < 0.0f || f >= 1.0f;
    }
    CHECK(fora == 0);
    CHECK(perto(materiais_ruido(2.3f, 1.7f, 8, 4, 9u), materiais_ruido(10.3f, 5.7f, 8, 4, 9u))); // período
    float salto = 0.0f;
    // padrão FOLD: o maior salto entre a borda direita e a esquerda (a textura repete sem emenda)
    for (int i = 0; i < 32; i++) {
        float v = (i + 0.5f) / 32.0f;
        salto = fmaxf(
            salto, fabsf(materiais_fbm(0.0005f, v, 12, 6, 4, 3u) - materiais_fbm(0.9995f, v, 12, 6, 4, 3u)));
    }
    CHECK(salto < 0.02f);
}

// Brilho médio perto de 1 (a cor da paleta continua sendo a cor do material: nada fica escuro demais) e
// mistura em [0, 1]; mesma semente, mesmo pixel.
static void testa_materiais(void) {
    for (int m = 0; m < MATERIAL_TOTAL; m++) {
        float soma = 0.0f;
        int invalidos = 0;
        // padrão FOLD: soma do brilho numa grade de 48 × 48 pixels
        for (int i = 0; i < 48 * 48; i++) {
            PixelMaterial p =
                materiais_pixel((TipoMaterial)m, (i % 48 + 0.5f) / 48, (i / 48 + 0.5f) / 48, 0.8f, 1985u);
            soma += p.brilho;
            invalidos += p.brilho < 0.15f || p.brilho > 2.0f || p.mistura < 0.0f || p.mistura > 1.0f;
        }
        float media = soma / (48 * 48);
        CHECK(invalidos == 0);
        CHECK(media > 0.7f && media < 1.15f);
    }
    PixelMaterial a = materiais_pixel(MATERIAL_CONCRETO, 0.3f, 0.6f, 0.8f, 1985u);
    PixelMaterial b = materiais_pixel(MATERIAL_CONCRETO, 0.3f, 0.6f, 0.8f, 1985u);
    CHECK(a.brilho == b.brilho && a.mistura == b.mistura);
    PixelMaterial limpo = materiais_pixel(MATERIAL_OLIVA, 0.3f, 0.6f, 0.0f, 1985u); // sujeira 0: sem mistura
    CHECK(limpo.mistura >= 0.0f && limpo.mistura <= 1.0f);
}

static void testa_decalques(void) {
    CHECK(materiais_mancha(DECALQUE_UMIDADE, 0.0f, 0.5f, 1u) == 0.0f); // some na borda
    CHECK(materiais_mancha(DECALQUE_ARRASTO, 0.5f, 1.0f, 1u) == 0.0f);
    CHECK(materiais_mancha(DECALQUE_UMIDADE, 0.5f, 0.35f, 1u) > 0.3f); // e existe no meio
    CHECK(materiais_mancha(DECALQUE_POSTO, 0.5f, 0.5f, 1u) == 0.0f);   // placa de texto não é mancha
    float minimo = 1.0f, maximo = 0.0f;
    // padrão FOLD: faixa do desgaste da tinta de estêncil
    for (int i = 0; i < 256; i++) {
        float d = materiais_desgaste((i % 16 + 0.5f) / 16, (i / 16 + 0.5f) / 16, 5u);
        minimo = fminf(minimo, d);
        maximo = fmaxf(maximo, d);
    }
    CHECK(minimo >= 0.0f && maximo <= 1.0f && maximo > 0.5f);
}

int main(void) {
    SetTraceLogLevel(LOG_ERROR); // o aviso de vetor cheio é esperado aqui
    testa_movimento();
    testa_visao();
    testa_ruido_percebido();
    testa_estados_criatura();
    testa_postura_criatura();
    testa_piscada_cela();
    testa_quadro_susto();
    testa_placar();
    testa_vidro();
    testa_evento_do_erro();
    testa_colecao_ruidos();
    testa_colecao_eventos();
    testa_colecao_particulas();
    testa_colecao_rachaduras();
    testa_colecao_paredes();
    testa_colecao_interativos();
    testa_nome_normaliza();
    testa_nome_limite();
    testa_nome_padrao();
    testa_nome_digitacao();
    testa_ruido_materiais();
    testa_materiais();
    testa_decalques();
    return resultado_final("test_regras");
}
