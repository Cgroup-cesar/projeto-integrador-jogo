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

// [REQ:PURA-regras_volumes_trilhas] testa silêncio, menu, passos e o coração por distância
static void testa_volumes_trilhas(void) {
    float v[TRILHA_TOTAL];
    SituacaoSom tudo = {true, true, true, true, 3.0f, true, true, true}; // silêncio vence tudo
    regras_volumes_trilhas(tudo, v);
    bool mudo = true;
    for (int i = 0; i < TRILHA_TOTAL; i++) // padrão FOLD: algum volume ficou acima de zero?
        if (v[i] > 0.0f) mudo = false;
    CHECK(mudo);
    SituacaoSom menu = {.tela_inicial = true};
    regras_volumes_trilhas(menu, v);
    CHECK(v[TRILHA_MENU] > 0.0f && perto(v[TRILHA_PASSO_ANDAR], 0.0f));
    SituacaoSom anda = {.andando = true}, corre = {.andando = true, .correndo = true};
    regras_volumes_trilhas(anda, v);
    CHECK(v[TRILHA_PASSO_ANDAR] > 0.0f && perto(v[TRILHA_PASSO_CORRER], 0.0f)); // andar: passo devagar
    regras_volumes_trilhas(corre, v);
    CHECK(v[TRILHA_PASSO_CORRER] > 0.0f && perto(v[TRILHA_PASSO_ANDAR], 0.0f)); // correr: passo rápido
    SituacaoSom longe = {.criatura_solta = true, .distancia_criatura = 20.0f};
    regras_volumes_trilhas(longe, v);
    CHECK(perto(v[TRILHA_CORACAO_NORMAL], 0.0f) && perto(v[TRILHA_CORACAO_RAPIDO], 0.0f)); // longe: nada
    SituacaoSom meio = {.criatura_solta = true, .distancia_criatura = 10.0f};
    regras_volumes_trilhas(meio, v);
    float normal_10m = v[TRILHA_CORACAO_NORMAL];
    CHECK(normal_10m > 0.0f && perto(v[TRILHA_CORACAO_RAPIDO], 0.0f)); // um pouco longe: coração normal
    SituacaoSom quase = {.criatura_solta = true, .distancia_criatura = 7.0f};
    regras_volumes_trilhas(quase, v);
    CHECK(v[TRILHA_CORACAO_NORMAL] > normal_10m); // mais perto, mais alto
    SituacaoSom colado = {.criatura_solta = true, .distancia_criatura = 3.0f};
    regras_volumes_trilhas(colado, v);
    CHECK(v[TRILHA_CORACAO_RAPIDO] > 0.0f && perto(v[TRILHA_CORACAO_NORMAL], 0.0f)); // muito perto: rápido
    SituacaoSom presa = {.criatura_solta = false, .distancia_criatura = 3.0f};
    regras_volumes_trilhas(presa, v);
    CHECK(perto(v[TRILHA_CORACAO_RAPIDO], 0.0f)); // na cela: sem coração
    SituacaoSom verdadeiro = {.radio_falando = true}, falso = {.radio_falando = true, .radio_falso = true};
    regras_volumes_trilhas(verdadeiro, v);
    CHECK(v[TRILHA_VOZ_RADIO] > 0.0f &&
          perto(v[TRILHA_VOZ_PRECURSOR], 0.0f)); // o Varela de verdade: só o rádio
    regras_volumes_trilhas(falso, v);
    CHECK(v[TRILHA_VOZ_RADIO] > 0.0f && v[TRILHA_VOZ_PRECURSOR] > 0.0f); // o falso: o canto dele por baixo
    CHECK(v[TRILHA_VOZ_PRECURSOR] < VOL_VOZ_PRECURSOR);                  // ...e baixo
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

// [REQ:PURA-regras_bonus_movimentos] pensar antes de mexer vale pontos; chutar não
static void testa_bonus_movimentos(void) {
    CHECK(regras_bonus_movimentos(2) == PONTOS_PAINEL_POUCOS_MOV);
    CHECK(regras_bonus_movimentos(MOVIMENTOS_BONUS_CHEIO) == PONTOS_PAINEL_POUCOS_MOV);
    CHECK(regras_bonus_movimentos(MOVIMENTOS_BONUS_CHEIO + 1) == PONTOS_PAINEL_POUCOS_MOV / 2);
    CHECK(regras_bonus_movimentos(MOVIMENTOS_BONUS_MEIO + 1) == 0);
    CHECK(regras_bonus_movimentos(-1) == 0);
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

// [REQ:PURA-regras_monta_texto] testa {NOME}, {PROF}, {x|y}, marcador desconhecido e o corte sem quebrar
// acento
static void testa_monta_texto(void) {
    Marcador m[] = {{"NOME", "Inês"}, {"PROF", "Professora"}};
    char s[64];
    regras_monta_texto("Olá, {PROF} {NOME}. Bem-vind{o|a}.", m, 2, true, s, sizeof(s));
    CHECK(strcmp(s, "Olá, Professora Inês. Bem-vinda.") == 0);
    regras_monta_texto("Saiu viv{o|a}.", m, 2, false, s, sizeof(s));
    CHECK(strcmp(s, "Saiu vivo.") == 0); // masculino: a primeira opção
    regras_monta_texto("{XYZ} e {sem fim", m, 2, false, s, sizeof(s));
    CHECK(strcmp(s, "{XYZ} e {sem fim") == 0); // desconhecido e chave aberta ficam como estão
    regras_monta_texto("sem marcador", NULL, 0, false, s, sizeof(s));
    CHECK(strcmp(s, "sem marcador") == 0);
    char curto[4]; // cabem 3 bytes: "In" e o "ê" (2 bytes) não cabe inteiro
    regras_monta_texto("{NOME}", m, 2, false, curto, sizeof(curto));
    CHECK(strcmp(curto, "In") == 0);
}

// [REQ:PURA-regras_nome_edita] testa letras com acento, o limite de letras, o Backspace e os espaços
// recusados
static void testa_nome_edita(void) {
    char nome[40] = "";
    CHECK(!regras_nome_edita(nome, sizeof(nome), 16, ' ')); // não começa com espaço
    CHECK(regras_nome_edita(nome, sizeof(nome), 16, 'J'));
    CHECK(regras_nome_edita(nome, sizeof(nome), 16, 0xE3)); // ã
    CHECK(strcmp(nome, "Jã") == 0);
    CHECK(regras_nome_edita(nome, sizeof(nome), 16, ' '));
    CHECK(!regras_nome_edita(nome, sizeof(nome), 16, ' ')); // dois espaços seguidos
    CHECK(!regras_nome_edita(nome, sizeof(nome), 16, '7')); // número
    CHECK(regras_nome_edita(nome, sizeof(nome), 16, NOME_APAGAR));
    CHECK(regras_nome_edita(nome, sizeof(nome), 16, NOME_APAGAR)); // apaga o "ã" inteiro (2 bytes)
    CHECK(strcmp(nome, "J") == 0);
    for (int i = 0; i < 20; i++) // tenta digitar 20 "é"; só cabem 15 (16 letras com o J)
        regras_nome_edita(nome, sizeof(nome), 16, 0xE9);
    CHECK(strlen(nome) == 1 + 15 * 2);
    char pequeno[4] = "";
    CHECK(!regras_nome_edita(pequeno, sizeof(pequeno), 16, NOME_APAGAR)); // apagar o vazio não faz nada
    CHECK(regras_nome_edita(pequeno, sizeof(pequeno), 16, 'a') &&
          regras_nome_edita(pequeno, sizeof(pequeno), 16, 'b'));
    CHECK(!regras_nome_edita(pequeno, sizeof(pequeno), 16, 0xE7)); // o "ç" (2 bytes) não cabe mais
    char minusculo[40] = "";
    const int digitado[] = {'j', 'o', 0xE3, 'o', ' ', 0xE9, 'd', 'e', 'r'}; // "joão éder"
    for (int i = 0; i < 9; i++) // padrão FOLD: cada letra digitada entra no nome
        regras_nome_edita(minusculo, sizeof(minusculo), 16, digitado[i]);
    CHECK(strcmp(minusculo, "Jo\xC3\xA3o \xC3\x89"
                            "der") == 0); // "João Éder": só o começo de cada palavra
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

// [REQ:PURA-materiais_uv] testa a UV de mundo: em metros, emendada entre caixas e presa ao teto
static void testa_uv_mundo(void) {
    Vector2 m = {6.0f, PE_DIREITO}, zero = {0.0f, 0.0f};
    Vector2 topo = materiais_uv((Vector3){4.0f, PE_DIREITO, 10.0f}, 2, m, zero, PE_DIREITO);
    Vector2 chao = materiais_uv((Vector3){4.0f, 0.0f, 10.0f}, 2, m, zero, PE_DIREITO);
    CHECK(perto(topo.y, 0.0f) && perto(chao.y, 1.0f)); // v = 0 no teto, 1 no chão
    Vector2 a = materiais_uv((Vector3){1.0f, 1.0f, 10.0f}, 2, m, zero, PE_DIREITO);
    Vector2 b = materiais_uv((Vector3){4.0f, 1.0f, 10.0f}, 2, m, zero, PE_DIREITO);
    CHECK(perto(b.x - a.x, 0.5f)); // 3 m numa repetição de 6 m: meia textura (nada estica)
    Vector2 x1 = materiais_uv((Vector3){16.0f, 2.0f, 7.0f}, 0, m, zero, PE_DIREITO);
    CHECK(perto(x1.x, 7.0f / 6.0f) && perto(x1.y, (PE_DIREITO - 2.0f) / PE_DIREITO)); // face em x usa z
    Vector2 piso = materiais_uv((Vector3){3.0f, 0.0f, 5.0f}, 1, (Vector2){2.0f, 2.0f}, zero, PE_DIREITO);
    CHECK(perto(piso.x, 1.5f) && perto(piso.y, 2.5f));
    Vector2 d =
        materiais_uv((Vector3){3.0f, 0.0f, 5.0f}, 1, (Vector2){2.0f, 2.0f}, (Vector2){0.25f, 0.5f}, 0);
    CHECK(perto(d.x, 1.75f) && perto(d.y, 3.0f)); // o deslocamento só soma
    Vector2 sem_metros = materiais_uv((Vector3){3.0f, 0.0f, 5.0f}, 1, zero, zero, 0); // metros 0 → 1 (sem /0)
    CHECK(perto(sem_metros.x, 3.0f));
}

// [REQ:PURA-regras_falha_texto] testa a legenda com defeito: rara, curta, repetível, sem quebrar acentos
static void testa_falha_texto(void) {
    const char *rotulo = "GEN. VARELA (r\xC3\xA1"
                         "dio)"; // "GEN. VARELA (rádio)"
    char s[64], de_novo[64];
    int falhas = 0, inicios = 0;
    bool antes = false, acento_intacto = true, calma_igual = true, falha_muda = false;
    for (int i = 0; i < 1000; i++) { // padrão FOLD: 10 s, a cada 0,01 s, contando as falhas
        bool f = regras_falha_texto(rotulo, i * 0.01f, s, sizeof(s));
        falhas += f;
        inicios += f && !antes;
        antes = f;
        if (strlen(s) != strlen(rotulo) || strstr(s, "\xC3\xA1") == NULL) acento_intacto = false;
        if (!f && strcmp(s, rotulo) != 0) calma_igual = false;
        if (f && strcmp(s, rotulo) != 0) falha_muda = true;
    }
    CHECK(falhas > 0 && falhas < 250); // falha às vezes, mas fica legível quase o tempo todo
    CHECK(inicios >= 4);               // várias vezes em 10 s (mais ou menos uma a cada 1,4 s)
    CHECK(acento_intacto && calma_igual && falha_muda);
    regras_falha_texto(rotulo, 3.21f, s, sizeof(s));
    regras_falha_texto(rotulo, 3.21f, de_novo, sizeof(de_novo));
    CHECK(strcmp(s, de_novo) == 0); // repetível: o mesmo instante, a mesma falha
    char curto[5];
    regras_falha_texto(rotulo, 0.0f, curto, sizeof(curto));
    CHECK(strlen(curto) == 4); // nunca passa do tamanho
    CHECK(!regras_falha_texto(rotulo, -1.0f, s, sizeof(s)) &&
          strcmp(s, rotulo) == 0); // tempo negativo: calmo
}

// [REQ:PURA-regras_ranking_insere] maior placar na frente; empate: o mais rápido; cheio: o pior sai
static void testa_ranking_insere(void) {
    static Ranking r; // grande: fora da pilha
    r.n = 0;
    EntradaRanking a = {300, 200, true, true, "Inês"}, b = {150, 100, false, false, "Ari"};
    EntradaRanking c = {300, 150, true, false, "Moura"}, d = {300, 150, false, true, "Lia"};
    CHECK(regras_ranking_insere(&r, &a) == 0);
    CHECK(regras_ranking_insere(&r, &b) == 1);
    CHECK(regras_ranking_insere(&r, &c) == 0); // mesmo placar da Inês, mais rápido
    CHECK(regras_ranking_insere(&r, &d) == 1); // empate total com Moura: quem chegou antes fica na frente
    CHECK(r.n == 4 && strcmp(r.itens[2].nome, "Inês") == 0 && strcmp(r.itens[3].nome, "Ari") == 0);
    r.n = 0;
    for (int i = 0; i < RANKING_MAX; i++) { // padrão MAP: enche com placares 1000, 990, ...
        EntradaRanking x = {1000 - i * 10, 60, true, true, "X"};
        regras_ranking_insere(&r, &x);
    }
    EntradaRanking pior = {-5000, 60, false, false, "Pior"}, meio = {995, 60, true, true, "Meio"};
    CHECK(regras_ranking_insere(&r, &pior) == -1 && r.n == RANKING_MAX);
    CHECK(regras_ranking_insere(&r, &meio) == 1 && r.n == RANKING_MAX); // entra e o último sai
    CHECK(r.itens[RANKING_MAX - 1].placar == 1000 - (RANKING_MAX - 2) * 10);
}

// [REQ:PURA-regras_ranking_le_linha] ida e volta pelo arquivo; linhas estragadas são recusadas
static void testa_ranking_linha(void) {
    EntradaRanking a = {-25, 431, false, true, "Maria Inês"}, b;
    char linha[96];
    CHECK(regras_ranking_escreve_linha(&a, linha, sizeof(linha)) > 0);
    CHECK(regras_ranking_le_linha(linha, &b));
    CHECK(b.placar == -25 && b.segundos == 431 && !b.vitoria && b.feminino &&
          strcmp(b.nome, "Maria Inês") == 0);
    CHECK(regras_ranking_le_linha("300;120;1;0;Ari\r\n", &b) && strcmp(b.nome, "Ari") == 0);
    CHECK(!regras_ranking_le_linha("", &b));
    CHECK(!regras_ranking_le_linha("300;120;1;0;", &b));     // sem nome
    CHECK(!regras_ranking_le_linha("abc;120;1;0;Ari", &b));  // placar não é número
    CHECK(!regras_ranking_le_linha("300;-4;1;0;Ari", &b));   // tempo negativo
    CHECK(!regras_ranking_le_linha("300;120;1;0;A;ri", &b)); // ';' no nome quebraria o arquivo
    CHECK(regras_ranking_escreve_linha(&a, linha, 8) == -1); // não coube
}

int main(void) {
    SetTraceLogLevel(LOG_ERROR); // o aviso de vetor cheio é esperado aqui
    testa_movimento();
    testa_visao();
    testa_ruido_percebido();
    testa_estados_criatura();
    testa_postura_criatura();
    testa_piscada_cela();
    testa_volumes_trilhas();
    testa_quadro_susto();
    testa_placar();
    testa_bonus_movimentos();
    testa_ranking_insere();
    testa_ranking_linha();
    testa_vidro();
    testa_evento_do_erro();
    testa_monta_texto();
    testa_nome_edita();
    testa_falha_texto();
    testa_colecao_ruidos();
    testa_colecao_eventos();
    testa_colecao_particulas();
    testa_colecao_rachaduras();
    testa_colecao_paredes();
    testa_colecao_interativos();
    testa_ruido_materiais();
    testa_materiais();
    testa_decalques();
    testa_uv_mundo();
    return resultado_final("test_regras");
}
