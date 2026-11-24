// animacao.c — monta a EntradaAnim do quadro a partir do Estado e chama regras_anim_passo (a pose vai para
// e->cri.anim). Só apresentação: a posição, o yaw e os estados da criatura são lidos, nunca escritos.
#include "animacao.h"
#include "lanterna.h"
#include <math.h>

#define TELEPORTE_M    1.5f // pulo maior que isso = ele "apareceu" lá (replanta os pés)
#define DESVIO_ALCANCA 5.0f // 1/s: um pulo curto (armário, mão no vidro) é alcançado em ~0,5 s

// ================================================================== posição desenhada
Vector3 animacao_pos(const Estado *e) {
    return Vector3Add(e->cri.pos, e->cri.anim_desvio);
}

// Pulos curtos de posição (chegar mais perto da porta do armário, a mão no vidro) não teletransportam o
// corpo: o desenho fica onde estava e alcança a posição nova dando passos. Pulos longos replantam os pés.
static bool segue_posicao(Criatura *c, float dt) {
    Vector3 pulo = Vector3Subtract(c->pos, c->anim_pos_ant);
    float d = Vector3Length(pulo), normal = CRIATURA_VEL_PERSEGUE * CRIATURA_BOTE_VEL * dt * 1.5f + 0.05f;
    bool teleporte = !c->anim.iniciado || d > TELEPORTE_M;
    if (teleporte)
        c->anim_desvio = (Vector3){0};
    else if (d > normal)
        c->anim_desvio = Vector3Subtract(c->anim_desvio, pulo);
    c->anim_desvio = Vector3Scale(c->anim_desvio, expf(-DESVIO_ALCANCA * dt));
    c->anim_pos_ant = c->pos;
    return teleporte;
}

// ================================================================== o bueiro
// Segundos desde o começo da saída do bueiro (−1 = fora dela). Começa ANIM_BUEIRO_PRE_S antes de a IA
// ativá-lo (as mãos aparecem na borda enquanto ele ainda não caça) e termina com `surgindo` = 1.
static float tempo_bueiro(const Estado *e) {
    const Criatura *c = &e->cri;
    if (c->contido || !regras_fase_eh_saida(e->fase) || c->distorcao > 0.0f) return -1.0f;
    if (!c->ativo)
        return c->tempo_para_surgir < ANIM_BUEIRO_PRE_S ? ANIM_BUEIRO_PRE_S - c->tempo_para_surgir : -1.0f;
    return c->surgindo < 1.0f ? ANIM_BUEIRO_PRE_S + c->surgindo * ANIM_BUEIRO_SAI_S : -1.0f;
}

bool animacao_visivel(const Estado *e) {
    return e->cri.contido || e->cri.ativo || tempo_bueiro(e) >= 0.0f;
}

// ================================================================== a entrada do quadro
static bool escutando(const Estado *e) {
    const Criatura *c = &e->cri;
    if (c->contido || !c->ativo || e->jog.modo != MODO_ARMARIO) return false;
    return Vector3Distance(c->pos, e->nivel.armarios[e->jog.armario]) < RESP_DIST_PORTA + 1.0f;
}

// Para onde a cabeça tende a olhar: na cela, para quem está no vidro; caçando, para o jogador; na porta do
// armário, para a porta.
static void olhar(const Estado *e, EntradaAnim *in) {
    const Criatura *c = &e->cri;
    in->olhar = e->camera.position;
    if (escutando(e)) {
        in->olhar = Vector3Add(e->nivel.armarios[e->jog.armario], (Vector3){0, 1.3f, 0});
        in->tem_olhar = true;
    } else {
        in->tem_olhar = c->contido || e->fase == FASE_FIM_RUIM ||
                        (c->ativo && (c->ve_jogador || c->estado == CRIATURA_PERSEGUE));
    }
}

// A mão esquerda dele (+x do corpo) espalmada no vidro, na altura do rosto de quem está do outro lado.
static Vector3 ponto_no_vidro(const Estado *e) {
    const Criatura *c = &e->cri;
    Vector3 lado = {cosf(c->yaw), 0.0f, -sinf(c->yaw)};
    Vector3 p = Vector3Add(animacao_pos(e), Vector3Scale(lado, 0.40f));
    return (Vector3){p.x, 1.75f, e->nivel.vidro_z - 0.06f};
}

static EntradaAnim entrada(const Estado *e, bool teleporte) {
    const Criatura *c = &e->cri;
    EntradaAnim in = {0};
    in.pos = animacao_pos(e);
    in.yaw = c->yaw;
    in.escala = e->rec.rig_escala;
    in.base_y = e->rec.rig_base_y;
    in.teleporte = teleporte;
    in.persegue = c->ativo && c->estado == CRIATURA_PERSEGUE ? 1.0f : 0.0f;
    in.escuta = escutando(e) ? 1.0f : 0.0f;
    in.lanterna = c->contido && lanterna_mira_criatura(e) ? 1.0f : 0.0f;
    in.fala = regras_fase_eh_rodada(e->fase) && c->fala_t > 0.0f; // na resposta final ele só espera
    in.mao_no_vidro = c->contido && c->mao_no_vidro;
    in.vidro = ponto_no_vidro(e);
    olhar(e, &in);
    in.bueiro_t = tempo_bueiro(e);
    in.bueiro = e->nivel.duto;
    in.distorcao = c->distorcao;
    in.convulsao = e->fase == FASE_FIM_BOM && c->contido;
    in.congelado = c->congelado_t > 0.0f;
    return in;
}

void animacao_atualiza(Estado *e, float dt) {
    bool teleporte = segue_posicao(&e->cri, dt);
    EntradaAnim in = entrada(e, teleporte);
    regras_anim_passo(&e->cri.anim, &in, dt);
}
