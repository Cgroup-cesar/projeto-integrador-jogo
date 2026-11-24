// porta_sala.c — a porta da sala de observação: o estado da folha, a colisão que acompanha a folha, quem
// abre e fecha (o jogador com E, a criatura chegando nela, a volta à sala) e o desenho.
#include "porta_sala.h"
#include "porta_bunker.h"
#include "nivel.h"
#include "audio_sintese.h"
#include "dialogo.h"
#include "texturas.h"
#include "ui.h"
#include <math.h>

// Alguém no vão (o jogador ou a criatura): a folha não fecha em cima dele.
static bool no_vao(const Estado *e, Vector3 p, float raio) {
    Vector3 c = e->nivel.porta_sala;
    return fabsf(p.x - c.x) < 0.25f + raio && fabsf(p.z - c.z) < 1.0f + raio;
}

static bool alguem_no_vao(const Estado *e) {
    return no_vao(e, e->jog.pos, JOGADOR_RAIO) || (e->cri.ativo && no_vao(e, e->cri.pos, 0.4f));
}

// [REQ:FILTER-parede-porta-sala] a folha saiu do vão: tira a porta das paredes de colisão
static void tira_parede(ColParedes *c) {
    int j = 0;
    for (int i = 0; i < c->n; i++) // padrão FILTER: fica tudo menos a porta da sala
        if (c->itens[i].tipo != PAREDE_PORTA) c->itens[j++] = c->itens[i];
    c->n = j;
}

// A colisão segue a folha: quase fechada, ela entra nas paredes; abrindo, sai.
static void acerta_colisao(Estado *e) {
    bool solida = regras_porta_solida(e->porta.abertura);
    if (solida == e->porta.solida) return;
    e->porta.solida = solida;
    if (solida)
        nivel_tranca_porta(&e->nivel, &e->paredes);
    else
        tira_parede(&e->paredes);
}

// Abrir e fechar faz barulho: a criatura ouve (o tranco dela mesma, não).
static void move_folha(Estado *e, bool abre, float duracao, bool barulho) {
    e->porta.aberta = abre;
    e->porta.duracao = duracao;
    float tom = duracao < PORTA_ABRE_S * 0.5f ? 1.5f : 1.0f; // o tranco da criatura: rápido e mais agudo
    audio_toca_em(e, abre ? SOM_PORTA_ABRE : SOM_PORTA_FECHA, e->nivel.porta_sala, PORTA_SOM_ALCANCE_M, tom);
    if (barulho && regras_fase_eh_saida(e->fase))
        col_ruidos_adiciona(&e->ruidos, (Ruido){e->nivel.porta_sala, RUIDO_RAIO_PORTA, 1.0f, RUIDO_PORTA});
}

void porta_sala_entra_fase(Estado *e) {
    e->porta = (PortaSala){false, 0.0f, PORTA_FECHA_S, false}; // as paredes acabaram de ser refeitas
    acerta_colisao(e);
}

void porta_sala_interage(Estado *e) {
    if (!regras_fase_eh_saida(e->fase)) {
        ui_fala(e, FALAS_PORTA_TRANCADA);
        return;
    }
    if (!e->porta.aberta)
        move_folha(e, true, PORTA_ABRE_S, true);
    else if (!no_vao(e, e->jog.pos, JOGADOR_RAIO)) // de pé no vão, não fecha a porta em cima de si
        move_folha(e, false, PORTA_FECHA_S, true);
}

void porta_sala_fecha(Estado *e) {
    if (e->porta.aberta) move_folha(e, false, PORTA_FECHA_S, false);
}

void porta_sala_atualiza(Estado *e, float dt) {
    PortaSala *p = &e->porta;
    bool solta = e->cri.ativo && !e->painel.resolvido && regras_fase_eh_saida(e->fase);
    if (!p->aberta && solta && Vector3Distance(e->cri.pos, e->nivel.porta_sala) < PORTA_CRIATURA_ABRE_M)
        move_folha(e, true, PORTA_CRIATURA_ABRE_S, false);
    float antes = p->abertura;
    p->abertura = regras_porta_passo(p->abertura, p->aberta, dt, p->duracao);
    if (!p->solida && regras_porta_solida(p->abertura) && alguem_no_vao(e)) { // ia fechar em cima de alguém
        p->abertura = antes;
        move_folha(e, true, PORTA_ABRE_S, false); // volta a abrir, como porta de elevador
    }
    acerta_colisao(e);
}

float porta_sala_abertura(const Estado *e) {
    float x = e->porta.abertura;
    return x * x * (3.0f - 2.0f * x);
}

// A porta de bunker (porta_bunker.c), com dois pilares de concreto fechando o vão de 2 m em volta do
// batente; sem o modelo, uma porta de correr que some na parede ao abrir.
void porta_sala_desenha(const Estado *e) {
    Vector3 p = e->nivel.porta_sala;
    for (int lado = -1; lado <= 1; lado += 2) // padrão MAP: um pilar de cada lado do batente
        texturas_caixa_mat(e, MATERIAL_CONCRETO, (Vector3){p.x, 1.1f, p.z + lado * 0.83f},
                           (Vector3){0.3f, 2.2f, 0.34f}, texturas_modo(MATERIAL_CONCRETO, 7 + lado), WHITE);
    if (porta_bunker_desenha(e)) return;
    texturas_caixa_mat(e, MATERIAL_PORTA, (Vector3){p.x, 1.1f, p.z + 1.4f * porta_sala_abertura(e)},
                       (Vector3){0.16f, 2.2f, 1.32f}, texturas_modo(MATERIAL_PORTA, 0), WHITE);
}
