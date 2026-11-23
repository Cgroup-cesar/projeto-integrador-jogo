// sanidade.c — atualiza a sanidade e o grão a cada quadro e desenha a barra no topo da tela.
#include "sanidade.h"
#include "ui.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

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

// Aproxima `atual` de `alvo` com velocidades diferentes para subir e para descer (independe do FPS).
static float segue(float atual, float alvo, float sobe, float desce, float dt) {
    float vel = alvo > atual ? sobe : desce;
    return atual + (alvo - atual) * fminf(1.0f, dt * vel);
}

// A barra desce junto com a sanidade e sobe suave nos bônus; o eco (o que acabou de ser perdido) desce
// devagar até ela, para quem joga ver quanto a criatura tirou.
static void atualiza_barra(Estado *e, float dt) {
    if (e->sanidade < e->sanidade_barra)
        e->sanidade_barra = e->sanidade;
    else
        e->sanidade_barra = segue(e->sanidade_barra, e->sanidade, SANIDADE_BARRA_SOBE, 0.0f, dt);
    e->sanidade_eco = fmaxf(e->sanidade_barra, e->sanidade_eco - SANIDADE_ECO_VEL * dt);
}

void sanidade_atualiza(Estado *e, float dt) {
    int agora = conta_conectivos(e);
    e->sanidade = regras_sanidade_conectivos(e->sanidade, e->sanidade_conectivos, agora);
    e->sanidade_conectivos = agora; // na partida nova os dois zeram juntos: sem bônus falso
    bool saida = regras_fase_eh_saida(e->fase);
    float dist = saida && e->cri.ativo ? Vector3Distance(e->cri.pos, e->jog.pos) : LONGE_DEMAIS;
    if (fase_de_jogo(e->fase)) {
        e->sanidade_longe_t = dist < SANIDADE_RAIO ? 0.0f : e->sanidade_longe_t + dt;
        e->sanidade = regras_sanidade_passo(e->sanidade, dist, e->cri.surgindo, e->jog.modo == MODO_ARMARIO,
                                            e->sanidade_longe_t, saida, dt);
    }
    atualiza_barra(e, dt);
    // A força do efeito segue a barra (e não a sanidade crua): muda junto com a porcentagem, sem saltos.
    e->sanidade_grao = regras_sanidade_intensidade(e->sanidade_barra);
    e->sanidade_surto = regras_surto_passo(e->sanidade_surto, e->sanidade_barra, dt); // zerada: o surto
}

// ------------------------------------------------------------------ desenho
static Color cor_da_barra(const Estado *e) {
    float s = e->sanidade_barra;
    if (s >= 60.0f) return COR_FOSFORO;
    if (s >= SANIDADE_ALERTA) return COR_AMBAR;
    float pisca = 0.5f + 0.5f * sinf(e->tempo_total * (6.0f + 10.0f * (1.0f - s / SANIDADE_ALERTA)));
    return Fade(COR_ALARME, 0.55f + 0.45f * pisca); // pulsa mais rápido quanto mais baixa
}

// Nas salas ela não cai: cheia, a barra some (só polui a tela); volta enquanto enche depois de uma saída.
// No painel e no bilhete ela continua à vista: é por ela que a pessoa sabe que ele está chegando e que é hora
// de largar a tarefa e se esconder.
static bool mostra_barra(const Estado *e) {
    bool importa = regras_fase_eh_saida(e->fase) || e->sanidade_barra < SANIDADE_MAX - 0.5f;
    return fase_de_jogo(e->fase) && importa && e->jumpscare_t <= 0.0f && !e->pausado;
}

// [REQ:SANIDADE-HUD] fixa no topo; com o terminal aberto (ele ocupa a direita), centrada no que sobra
void sanidade_desenha(const Estado *e) {
    if (!mostra_barra(e)) return;
    float s = ui_escala(), largura = 260 * s;
    float livre = e->jog.modo == MODO_TERMINAL ? GetScreenWidth() - 680 * s : (float)GetScreenWidth();
    Rectangle barra = {(livre - largura) * 0.5f, 40 * s, largura, 9 * s};
    Color cor = cor_da_barra(e);
    ui_painel((Rectangle){barra.x - 14 * s, 10 * s, largura + 28 * s, 48 * s}, cor);
    char pct[8];
    snprintf(pct, sizeof(pct), "%d%%", (int)(e->sanidade_barra + 0.5f));
    ui_texto(e, dialogo_texto(TXT_HUD_SANIDADE), (Vector2){barra.x, 15 * s}, 18 * s, cor);
    Vector2 m = ui_mede(e, pct, 18 * s);
    ui_texto(e, pct, (Vector2){barra.x + largura - m.x, 15 * s}, 18 * s, cor);
    DrawRectangleRec(barra, Fade(BLACK, 0.7f));
    float por_ponto = largura / SANIDADE_MAX;
    DrawRectangleRec((Rectangle){barra.x, barra.y, e->sanidade_eco * por_ponto, barra.height},
                     Fade(COR_ALARME, 0.45f)); // o que a criatura acabou de tirar
    DrawRectangleRec((Rectangle){barra.x, barra.y, e->sanidade_barra * por_ponto, barra.height}, cor);
}
