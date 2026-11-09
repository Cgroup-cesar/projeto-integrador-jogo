// test_regras.c — regras puras e coleções, incluindo as bordas: lista vazia, vetor cheio, ruído esgotado,
// criatura sem ruídos e placar sem eventos. Roda sem abrir janela.
#include "raylib.h"
#include "checagem.h"
#include "regras.h"
#include "colecoes.h"
#include <math.h>

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

int main(void) {
    SetTraceLogLevel(LOG_ERROR); // o aviso de vetor cheio é esperado aqui
    testa_movimento();
    testa_visao();
    testa_ruido_percebido();
    testa_estados_criatura();
    testa_placar();
    testa_vidro();
    testa_evento_do_erro();
    testa_colecao_ruidos();
    testa_colecao_eventos();
    testa_colecao_particulas();
    testa_colecao_rachaduras();
    testa_colecao_paredes();
    testa_colecao_interativos();
    return resultado_final("test_regras");
}
