// retorno.c — a volta à sala de observação. Com a energia de volta, ao entrar na sala o jogo assume o
// jogador: ele vira para a porta, fecha a porta (porta_sala.c) e entra na sala, virando para a cela. A
// troca de fase acontece no meio da cena, quando a folha bate: a rodada seguinte começa com o jogador onde
// ele está (jogo.c não teleporta), e a cena continua até ele terminar de entrar. A coreografia é a regra
// pura regras_retorno_pose; aqui ela só é aplicada.
#include "retorno.h"
#include "nivel.h"
#include "jogador.h"
#include "porta_sala.h"
#include "susto.h"
#include <math.h>

bool retorno_ativo(const Estado *e) {
    return e->retorno.ativo;
}

void retorno_entra_fase(Estado *e, Fase fase, bool reinicio) {
    Retorno *r = &e->retorno;
    bool continua = r->ativo && !reinicio && fase == regras_proxima_fase(r->saida, EV_SAIDA_CONCLUIDA);
    if (!continua) r->ativo = false; // menu, pausa, F5: a cena acaba ali
}

static bool pode_comecar(const Estado *e) {
    bool de_volta = regras_fase_eh_saida(e->fase) && e->painel.resolvido &&
                    e->painel.resolvido_t > RETORNO_ESPERA_S && e->jog.modo == MODO_ANDANDO;
    return de_volta && !susto_ativo(e) && nivel_na_sala_de_observacao(&e->nivel, e->jog.pos);
}

// De onde ele estava até de frente para a porta (por dentro), e dali sala adentro, olhando para a cela.
static void comeca(Estado *e) {
    Retorno *r = &e->retorno;
    Vector3 porta = e->nivel.porta_sala, cela = e->nivel.cela_precursor;
    *r = (Retorno){0};
    r->ativo = true;
    r->saida = e->fase;
    r->de = (PoseJogador){e->jog.pos, e->jog.yaw, e->jog.pitch};
    r->porta = (PoseJogador){{porta.x - RETORNO_DIANTE_PORTA_M, 0.0f, porta.z}, PI * 0.5f, -0.05f};
    Vector3 dentro = {r->porta.pos.x - RETORNO_ENTRA_M, 0.0f, porta.z};
    r->sala = (PoseJogador){dentro, atan2f(cela.x - dentro.x, cela.z - dentro.z), 0.0f};
    e->jog.correndo = false;
}

void retorno_atualiza(Estado *e, float dt) {
    Retorno *r = &e->retorno;
    if (!r->ativo && !pode_comecar(e)) return;
    if (!r->ativo) comeca(e);
    r->t += dt;
    EtapaRetorno etapa = regras_retorno_etapa(r->t);
    if (etapa >= RETORNO_FECHA && !r->fechou) {
        r->fechou = true;
        porta_sala_fecha(e);
    }
    if (etapa >= RETORNO_ENTRA && !r->trocou) { // a folha bateu: a rodada seguinte começa aqui mesmo
        r->trocou = true;
        if (regras_fase_eh_saida(e->fase)) e->evento = EV_SAIDA_CONCLUIDA;
    }
    PoseJogador p = regras_retorno_pose(r->t, r->de, r->porta, r->sala);
    bool andando = Vector3Distance(e->jog.pos, p.pos) > 0.3f * dt; // mais de 0,3 m/s: conta como passo
    e->jog.pos = p.pos;
    e->jog.yaw = p.yaw;
    e->jog.pitch = p.pitch;
    e->jog.mira = -1;                                     // nada de "E — ..." no meio da cena
    e->in.mover = (Vector2){0.0f, andando ? 1.0f : 0.0f}; // os passos gravados (trilhas.c) seguem a cena
    jogador_passos(e, andando, dt);
    if (etapa == RETORNO_FIM) r->ativo = false; // o controle volta
}

void retorno_desenha(const Estado *e) {
    if (!e->retorno.ativo) return;
    int w = GetScreenWidth(), h = GetScreenHeight();
    int faixa = (int)(h * RETORNO_FAIXA_ALTURA * regras_retorno_faixa(e->retorno.t));
    DrawRectangle(0, 0, w, faixa, BLACK);
    DrawRectangle(0, h - faixa, w, faixa, BLACK);
}
