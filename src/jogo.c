// jogo.c — o fluxo da partida: aplica a máquina de fases (pura) e despacha atualiza/desenha por fase.
#include "jogo.h"
#include "nivel.h"
#include "jogador.h"
#include "criatura.h"
#include "rodadas.h"
#include "terminal.h"
#include "circuito.h"
#include "compositor.h"
#include "glifos.h"
#include "posfx.h"
#include "audio_sintese.h"
#include "dialogo.h"
#include "ui.h"
#include <string.h>

// ================================================================== início e fim
static void limpa_partida(Estado *e) {
    e->eventos.n = 0;
    e->rachaduras.n = 0;
    e->erros_totais = 0;
    e->tempo_partida = 0.0f;
    e->diario_lido = false;
    memset(e->conectivo_decifrado, 0, sizeof(e->conectivo_decifrado));
}

static void pontua(Estado *e, TipoPonto tipo, int pontos, Fase fase) {
    col_eventos_adiciona(&e->eventos, (EventoPlacar){tipo, pontos, fase});
}

// Pontos que dependem de como a fase terminou (o placar em si é um fold dos eventos).
static void pontua_evento(Estado *e, Fase fase, EventoFase ev) {
    if (ev == EV_RODADA_CONCLUIDA && e->erros_na_fase == 0) pontua(e, PONTO_RODADA_SEM_ERRO, PONTOS_RODADA_SEM_ERRO, fase);
    if (ev == EV_SAIDA_CONCLUIDA && e->pegos_na_fase == 0) pontua(e, PONTO_SAIDA_LIMPA, PONTOS_SAIDA_SEM_SER_PEGO, fase);
    bool final = ev == EV_TRANSMITIU_CONTRADICAO || ev == EV_TRANSMITIU_SATISFAZIVEL;
    if (!final) return;
    Fase destino = regras_proxima_fase(fase, ev);
    if (ev == EV_TRANSMITIU_CONTRADICAO) pontua(e, PONTO_FINAL_BOM, PONTOS_FINAL_BOM, destino);
    int bonus = regras_bonus_tempo(e->tempo_partida);
    if (bonus > 0) pontua(e, PONTO_BONUS_TEMPO, bonus, destino);
}

// ================================================================== entrada nas fases
static void prepara_mundo(Estado *e, Fase fase) {
    e->ruidos.n = 0;
    e->particulas.n = 0;
    nivel_constroi_paredes(&e->nivel, &e->paredes);
    if (!regras_fase_eh_saida(fase)) nivel_tranca_porta(&e->nivel, &e->paredes);
    nivel_constroi_interativos(&e->nivel, &e->interativos);
    col_interativos_filtra_fase(&e->interativos, fase);
    e->jog.modo = MODO_ANDANDO;
    e->jog.folego = JOGADOR_FOLEGO_MAX;
    e->jog.exausto = false;
    e->jumpscare_t = e->silencio_t = e->apagao_t = e->falha_luz_t = e->alarme_t = e->retorno_t = 0.0f;
    e->legendas.n = 0;
}

static void entra_rodada(Estado *e, Fase fase, bool reinicio) {
    e->rachaduras.n = 0; // vidro restaurado
    rodadas_inicia(e, regras_numero_rodada(fase));
    criatura_contem(e);
    if (fase != FASE_R1) e->cri.avanco_vidro = 1.0f; // depois da R1 ele ficou mais perto do vidro
    jogador_teleporta(e, e->nivel.inicio, PI);
    if (reinicio) ui_fala(e, FALAS_RODADA_RECOMECA);
}

static void entra_saida(Estado *e, Fase fase, bool reinicio) {
    circuito_inicia(e, fase == FASE_S1 ? 1 : 2);
    criatura_inicia_saida(e);
    jogador_teleporta(e, e->nivel.inicio, PI * 0.5f); // olhando para a porta da sala, a leste (+x)
    ui_fala(e, reinicio ? FALAS_PEGO : fase == FASE_S1 ? FALAS_INICIO_S1 : FALAS_INICIO_S2);
}

static void entra_fase(Estado *e, Fase fase, bool reinicio) {
    e->fase = fase;
    e->tempo_fase = 0.0f;
    if (!reinicio) {
        e->erros_na_fase = 0;
        e->pegos_na_fase = 0;
    }
    prepara_mundo(e, fase);
    switch (fase) {
    case FASE_MENU:
        limpa_partida(e);
        criatura_contem(e);
        jogador_teleporta(e, e->nivel.inicio, PI);
        e->menu.cursor = 0;
        break;
    case FASE_INTRO:
        memset(&e->intro, 0, sizeof(e->intro));
        jogador_teleporta(e, e->nivel.inicio, PI);
        break;
    case FASE_R1:
    case FASE_R2:
    case FASE_R3: entra_rodada(e, fase, reinicio); break;
    case FASE_S1:
    case FASE_S2: entra_saida(e, fase, reinicio); break;
    case FASE_RESPOSTA_FINAL:
        compositor_inicia(e);
        criatura_contem(e);
        e->rod.luz_a = e->rod.luz_b = true; // as duas luzes da cela acesas: ele está ali, encostado no vidro
        e->cri.avanco_vidro = 1.0f;
        e->cri.mao_no_vidro = true;
        jogador_vai_ao_console(e);
        e->jog.modo = MODO_TERMINAL;
        break;
    case FASE_FIM_RUIM:
    case FASE_FIM_BOM: compositor_inicia_fim(e); break;
    default: break;
    }
    // Os caches dos dois folds (vidas e placar) são recalculados depois que a fase limpou o que precisava.
    e->vidas = regras_integridade_vidro(e->rachaduras.itens, e->rachaduras.n, VIDAS_VIDRO);
    e->placar = regras_placar(e->eventos.itens, e->eventos.n);
    TraceLog(LOG_INFO, "PRECURSOR: fase %s%s", regras_nome_fase(fase), reinicio ? " (recomeço)" : "");
}

void jogo_aplica_evento(Estado *e, EventoFase evento) {
    if (evento == EV_NENHUM) return;
    Fase atual = e->fase;
    pontua_evento(e, atual, evento);
    Fase proxima = regras_proxima_fase(atual, evento);
    bool reinicia = regras_reinicia_fase(atual, evento);
    if (proxima != atual || reinicia) entra_fase(e, proxima, reinicia);
    e->placar = regras_placar(e->eventos.itens, e->eventos.n);
}

void jogo_reinicia_fase(Estado *e) {
    entra_fase(e, e->fase, true);
}

void jogo_vai_para_fase(Estado *e, Fase fase) {
    if (fase > FASE_MENU && fase < FASE_TOTAL) entra_fase(e, fase, false);
}

void jogo_inicia(Estado *e) {
    nivel_carrega(&e->nivel);
    e->jog.ultima_celula = -1;
    entra_fase(e, FASE_MENU, false);
}

void jogo_finaliza(Estado *e) {
    (void)e;
}

// ================================================================== intro
// Conta letras (não bytes): "Ç" ocupa 2 bytes em UTF-8 mas é uma letra só.
static int conta_letras(const char *texto) {
    int n = 0;
    for (const char *p = texto; *p; p++)
        if (((unsigned char)*p & 0xC0) != 0x80) n++;
    return n;
}

static void atualiza_intro(Estado *e, float dt) {
    Intro *it = &e->intro;
    if (it->interfone) {
        if (e->in.confirmar) ui_legenda_pula(e);
        if (ui_legendas_vazias(e)) e->evento = EV_INTRO_TERMINOU;
        return;
    }
    const char *linha = dialogo_intro(it->linha);
    float total = (float)conta_letras(linha);
    float antes = it->letras;
    it->letras += dt * 30.0f;
    if ((int)it->letras != (int)antes && it->letras < total) audio_toca(e, SOM_MAQUINA_ESCREVER, 0.5f, 0.9f + GetRandomValue(0, 20) / 100.0f);
    if (e->in.confirmar) it->letras = it->letras < total ? total : total + 99.0f;
    if (it->letras >= total) it->espera += dt;
    if (it->espera < 1.1f && it->letras < total + 99.0f) return;
    it->linha++;
    it->letras = 0.0f;
    it->espera = 0.0f;
    if (it->linha >= INTRO_LINHAS) {
        it->interfone = true;
        ui_fala(e, FALAS_INTRO_INTERFONE);
    }
}

// ================================================================== pausa, debug e modo feira
static bool jogando(const Estado *e) {
    return e->fase != FASE_MENU && e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM;
}

static void atualiza_pausa(Estado *e) {
    if (e->in.cima || e->in.baixo) e->menu.cursor = 1 - e->menu.cursor;
    if (e->in.voltar || (e->in.confirmar && e->menu.cursor == 0)) e->pausado = false;
    else if (e->in.confirmar && e->menu.cursor == 1) {
        e->pausado = false;
        entra_fase(e, FASE_MENU, false);
    }
}

static void atualiza_debug(Estado *e) {
#if TECLAS_DEBUG
    if (e->modo_feira) return;
    if (e->in.f2) e->evento = EV_PULAR_FASE;
    if (e->in.f3) e->debug_criatura = !e->debug_criatura;
    if (e->in.f5 && jogando(e)) jogo_reinicia_fase(e);
    if (e->in.f9) e->evento = EV_IR_RESPOSTA_FINAL;
    if (e->in.f12) e->pedir_captura = true;
#else
    (void)e;
#endif
}

static void atualiza_feira(Estado *e, float dt) {
    if (!e->modo_feira || !jogando(e)) return;
    e->tempo_inativo = e->in.alguma ? 0.0f : e->tempo_inativo + dt;
    if (e->tempo_inativo > FEIRA_INATIVIDADE_S) {
        e->tempo_inativo = 0.0f;
        entra_fase(e, FASE_MENU, false);
    }
}

static void atualiza_cursor(const Estado *e) {
    bool prende = jogando(e) && !e->pausado && !e->modo_smoke && e->fase != FASE_INTRO;
    if (prende && !IsCursorHidden()) DisableCursor();
    if (!prende && IsCursorHidden()) EnableCursor();
}

// ================================================================== atualiza
static void atualiza_fase(Estado *e, float dt) {
    switch (e->fase) {
    case FASE_INTRO: atualiza_intro(e, dt); break;
    case FASE_R1:
    case FASE_R2:
    case FASE_R3:
        rodadas_atualiza(e, dt);
        terminal_atualiza(e, dt);
        break;
    case FASE_S1:
    case FASE_S2: circuito_atualiza(e, dt); break;
    case FASE_RESPOSTA_FINAL: compositor_atualiza(e, dt); break;
    case FASE_FIM_RUIM:
    case FASE_FIM_BOM: compositor_atualiza_fim(e, dt); break;
    default: break;
    }
}

static void atualiza_temporizadores(Estado *e, float dt) {
    e->tempo_fase += dt;
    if (jogando(e) && e->fase != FASE_INTRO) e->tempo_partida += dt;
    e->apagao_t = e->apagao_t > 0.0f ? e->apagao_t - dt : 0.0f;
    e->falha_luz_t = e->falha_luz_t > 0.0f ? e->falha_luz_t - dt : 0.0f;
}

void jogo_atualiza(Estado *e, float dt) {
    atualiza_cursor(e);
    atualiza_debug(e);
    if (e->pausado) {
        atualiza_pausa(e);
        return;
    }
    bool anda_livre = e->fase != FASE_MENU && e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM &&
                      !(e->fase == FASE_INTRO && !e->intro.interfone);
    if (e->in.voltar && anda_livre && e->jog.modo == MODO_ANDANDO) {
        e->pausado = true;
        e->menu.cursor = 0;
        return;
    }
    atualiza_temporizadores(e, dt);
    atualiza_feira(e, dt);
    criatura_atualiza_susto(e, dt);
    if (anda_livre && !criatura_em_susto(e)) jogador_atualiza(e, dt);
    nivel_atualiza(e, dt);
    atualiza_fase(e, dt);
    criatura_atualiza(e, dt);
    glifos_atualiza(e, dt);
    col_ruidos_atualiza(&e->ruidos, dt);
    col_particulas_atualiza(&e->particulas, dt, (Vector3){0, -0.5f, 0},
                            (Vector3){MAPA_LARGURA * TAMANHO_CELULA, PE_DIREITO, MAPA_ALTURA * TAMANHO_CELULA});
    ui_atualiza(e, dt);
    jogador_atualiza_camera(e);
    EventoFase ev = e->evento;
    e->evento = EV_NENHUM;
    jogo_aplica_evento(e, ev);
}

// ================================================================== desenho
static void desenha_mundo_3d(const Estado *e) {
    Vector3 pos[4], cor[4];
    int n = nivel_luzes(e, pos, cor);
    float ambiente = e->apagao_t > 0.0f ? LUZ_AMBIENTE * 0.3f : LUZ_AMBIENTE;
    BeginMode3D(e->camera);
    posfx_comeca_luz(e, pos, cor, n, ambiente);
    nivel_desenha(e);
    criatura_desenha(e);
    posfx_termina_luz(e);
    nivel_desenha_emissivos(e);
    circuito_desenha_3d(e);
    glifos_desenha(e);
    nivel_desenha_vidro(e);
    EndMode3D();
}

void jogo_desenha_cena(const Estado *e) {
    ClearBackground(BLACK);
    bool tela_preta = e->fase == FASE_INTRO && !e->intro.interfone;
    if (!tela_preta) desenha_mundo_3d(e);
    if (regras_fase_eh_rodada(e->fase) || e->fase == FASE_RESPOSTA_FINAL) rodadas_desenha(e);
    jogador_desenha(e);
    criatura_desenha_jumpscare(e);
    if (e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM) compositor_desenha_fim(e);
}

void jogo_desenha_interface(const Estado *e) {
    if (e->jumpscare_t <= 0.0f) {
        terminal_desenha(e);
        compositor_desenha(e);
        circuito_desenha(e);
    }
    ui_desenha(e);
}
