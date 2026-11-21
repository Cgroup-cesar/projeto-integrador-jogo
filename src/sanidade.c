// sanidade.c — atualiza a sanidade a cada quadro (as contas estão em regras_sanidade.c, puras).
#include "sanidade.h"
#include "raymath.h"
#include <math.h>

#define LONGE_DEMAIS 1.0e6f // distância usada quando não há criatura solta

static bool fase_de_jogo(Fase f) {
    return regras_fase_eh_rodada(f) || regras_fase_eh_saida(f) || f == FASE_RESPOSTA_FINAL;
}

static int conta_conectivos(const Estado *e) {
    int n = 0;
    for (int i = 0; i < 4; i++) // padrão FOLD: quantos conectivos já foram decifrados
        n += e->conectivo_decifrado[i] ? 1 : 0;
    return n;
}

void sanidade_atualiza(Estado *e, float dt) {
    int agora = conta_conectivos(e);
    e->sanidade = regras_sanidade_conectivos(e->sanidade, e->sanidade_conectivos, agora);
    e->sanidade_conectivos = agora; // na partida nova os dois zeram juntos: sem bônus falso
    if (fase_de_jogo(e->fase)) {
        bool saida = regras_fase_eh_saida(e->fase);
        float dist = saida && e->cri.ativo ? Vector3Distance(e->cri.pos, e->jog.pos) : LONGE_DEMAIS;
        e->sanidade_longe_t = dist < SANIDADE_RAIO ? 0.0f : e->sanidade_longe_t + dt;
        e->sanidade = regras_sanidade_passo(e->sanidade, dist, e->cri.surgindo, e->jog.modo == MODO_ARMARIO,
                                            e->sanidade_longe_t, saida, dt);
    }
    float alvo = regras_sanidade_grao(e->sanidade) * SANIDADE_GRAO_MAX;
    e->sanidade_grao += (alvo - e->sanidade_grao) * fminf(1.0f, dt * SANIDADE_GRAO_VEL);
}

void sanidade_desenha(const Estado *e) {
    (void)e;
}
