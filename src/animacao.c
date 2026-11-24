// animacao.c — monta a EntradaAnim do quadro a partir do Estado e chama regras_anim_passo (a pose vai para
// e->cri.anim). Só apresentação: a posição, o yaw e os estados da criatura são lidos, nunca escritos (a saída
// pela tubulação da cela desenha o corpo num caminho próprio, sem mexer na posição da IA).
#include "animacao.h"
#include "lanterna.h"
#include <math.h>

#define TELEPORTE_M    1.5f // pulo maior que isso = ele "apareceu" lá (replanta os pés)
#define DESVIO_ALCANCA 5.0f // 1/s: um pulo curto (armário, mão no vidro) é alcançado em ~0,5 s

// ================================================================== a tubulação da cela
// Quando as luzes caem no fim da R1 e da R2, ele vira, anda até a boca da tubulação no fundo da cela, vira de
// novo para o vidro e desce por ela (a saída do bueiro ao contrário). Tempos em ANIM_CANO_* (config.h).
// O relógio da cena: começa na falha de luz da R1 e da R2 e segue na saída seguinte. Uma saída recomeçada
// (pego) já o encontra no fim: ele não sai de novo.
static void atualiza_cano(Estado *e, float dt) {
    const Rodada *r = &e->rod;
    bool cai_a_luz = regras_fase_eh_rodada(e->fase) && r->etapa == ETAPA_CONCLUIDA && r->numero < 3 &&
                     r->concluida_t >= ANIM_CANO_INICIO_S;
    if (cai_a_luz) {
        e->cano = true;
        e->cano_t = r->concluida_t - ANIM_CANO_INICIO_S;
    } else if (e->cano && regras_fase_eh_saida(e->fase)) {
        e->cano_t += dt;
    } else {
        e->cano = false;
    }
}

static float angulo_entre(float de, float para, float k) {
    return de + atan2f(sinf(para - de), cosf(para - de)) * k;
}

static float suave(float t, float de, float dura) {
    float x = Clamp((t - de) / dura, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

// Onde e para onde ele está no caminho até a boca. Sai de onde a cela o pôs (na saída seguinte ele já está
// na boca) e chega de frente para o vidro (yaw 0), com a borda da frente entre ele e quem olha.
static void caminho_cano(const Estado *e, Vector3 *pos, float *yaw) {
    Vector3 boca = e->nivel.cano_cela, de = e->cri.contido ? e->cri.pos : boca;
    Vector3 ida = Vector3Subtract(boca, de);
    float t = e->cano_t, rumo = atan2f(ida.x, ida.z), anda = suave(t, ANIM_CANO_VIRA_S, ANIM_CANO_ANDA_S);
    *pos = Vector3Lerp(de, boca, anda);
    *yaw = angulo_entre(0.0f, rumo, suave(t, 0.0f, ANIM_CANO_VIRA_S));
    *yaw = angulo_entre(*yaw, 0.0f, suave(t, ANIM_CANO_VIRA_S + ANIM_CANO_ANDA_S, ANIM_CANO_VIRA_S));
}

// ================================================================== posição desenhada
static Vector3 pos_da_ia(const Estado *e) { // a da IA, ou a do caminho até a tubulação
    Vector3 pos = e->cri.pos;
    float yaw;
    if (e->cano) caminho_cano(e, &pos, &yaw);
    return pos;
}

Vector3 animacao_pos(const Estado *e) {
    return Vector3Add(pos_da_ia(e), e->cri.anim_desvio);
}

float animacao_yaw(const Estado *e) {
    Vector3 pos;
    float yaw = e->cri.yaw;
    if (e->cano) caminho_cano(e, &pos, &yaw);
    return yaw;
}

// Pulos curtos de posição (chegar mais perto da porta do armário, a mão no vidro) não teletransportam o
// corpo: o desenho fica onde estava e alcança a posição nova dando passos. Pulos longos replantam os pés.
static bool segue_posicao(const Estado *e, Criatura *c, float dt) {
    Vector3 atual = pos_da_ia(e), pulo = Vector3Subtract(atual, c->anim_pos_ant);
    float d = Vector3Length(pulo), normal = CRIATURA_VEL_PERSEGUE * CRIATURA_BOTE_VEL * dt * 1.5f + 0.05f;
    bool teleporte = !c->anim.iniciado || d > TELEPORTE_M;
    if (teleporte)
        c->anim_desvio = (Vector3){0};
    else if (d > normal)
        c->anim_desvio = Vector3Subtract(c->anim_desvio, pulo);
    c->anim_desvio = Vector3Scale(c->anim_desvio, expf(-DESVIO_ALCANCA * dt));
    c->anim_pos_ant = atual;
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
    bool no_cano = e->cano && e->cano_t < ANIM_CANO_FIM; // depois, já está dentro da tubulação
    return e->cri.contido || e->cri.ativo || tempo_bueiro(e) >= 0.0f || no_cano;
}

// A descida pela tubulação é a saída do bueiro tocada de trás para a frente (−1 = ainda não desce). Do vidro,
// o parapeito esconde o chão do fundo da cela: a parte que se vê (agachar, o joelho na borda, o tronco
// afundando, até os ombros: os últimos 40% do roteiro) fica com ANIM_CANO_VISTO do tempo; o resto, rápido.
#define CANO_ROTEIRO_VISTO 0.4f

static float tempo_descida(const Estado *e) {
    if (!e->cano || e->cano_t < ANIM_CANO_DESCE_EM) return -1.0f;
    float k = Clamp((e->cano_t - ANIM_CANO_DESCE_EM) / ANIM_CANO_DESCE_S, 0.0f, 1.0f);
    float v = ANIM_CANO_VISTO, r = CANO_ROTEIRO_VISTO;
    float f = k < v ? r * k / v : r + (1.0f - r) * (k - v) / (1.0f - v);
    return (ANIM_BUEIRO_PRE_S + ANIM_BUEIRO_SAI_S) * (1.0f - f);
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
    in.yaw = animacao_yaw(e);
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
    if (tempo_descida(e) >= 0.0f) {
        in.bueiro_t = tempo_descida(e);
        in.bueiro = e->nivel.cano_cela;
        in.desce = true;
    }
    in.distorcao = c->distorcao;
    in.convulsao = e->fase == FASE_FIM_BOM && c->contido;
    in.congelado = c->congelado_t > 0.0f;
    return in;
}

void animacao_atualiza(Estado *e, float dt) {
    atualiza_cano(e, dt);
    bool teleporte = segue_posicao(e, &e->cri, dt);
    EntradaAnim in = entrada(e, teleporte);
    regras_anim_passo(&e->cri.anim, &in, dt);
}
