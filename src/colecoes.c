// colecoes.c — operações sobre as coleções: adicionar (com vetor cheio tratado), map, filter e fold.
// Cada laço leva o comentário do padrão, como nos slides. O filter é sempre compactação no próprio vetor.
#include "raylib.h"
#include "colecoes.h"
#include "funcional.h"
#include "regras.h"

// Vetor cheio não trava o jogo: o item novo é descartado e fica registrado no log.
// [REQ:RNF4] vetor cheio descarta com TraceLog, sem crash
static bool tem_espaco(int n, int capacidade, const char *nome) {
    if (n < capacidade) return true;
    TraceLog(LOG_WARNING, "PRECURSOR: coleção %s cheia (%d); item descartado", nome, capacidade);
    return false;
}

// ================================================================== ruídos
bool col_ruidos_adiciona(ColRuidos *c, Ruido r) {
    if (!tem_espaco(c->n, MAX_RUIDOS, "ruidos")) return false;
    c->itens[c->n++] = r;
    return true;
}

void col_ruidos_atualiza(ColRuidos *c, float dt) {
    // [REQ:MAP-ruidos] cada ruído perde intensidade com o tempo (multiplicado por dt)
    // padrão MAP
    for (int i = 0; i < c->n; i++) c->itens[i].intensidade -= RUIDO_DECAIMENTO * dt;

    // [REQ:FILTER-ruidos] remove os ruídos esgotados, compactando o vetor
    int j = 0;
    // padrão FILTER
    for (int i = 0; i < c->n; i++)
        if (c->itens[i].intensidade > 0.0f) c->itens[j++] = c->itens[i];
    c->n = j;
}

// [REQ:FOLD-ruidos] escolhe o ruído percebido mais forte (atenuado pela distância); −1 se não houver nenhum
int col_ruidos_mais_forte(const ColRuidos *c, Vector3 ouvinte, float *intensidade) {
    int melhor = -1;
    float maior = 0.0f;
    // padrão FOLD
    for (int i = 0; i < c->n; i++) {
        float percebido = regras_ruido_percebido(c->itens[i], ouvinte);
        if (percebido > maior) {
            maior = percebido;
            melhor = i;
        }
    }
    if (intensidade != NULL) *intensidade = maior;
    return melhor;
}

// ================================================================== partículas (usam as versões genéricas)
bool col_particulas_adiciona(ColParticulas *c, Particula p) {
    if (!tem_espaco(c->n, MAX_PARTICULAS, "particulas")) return false;
    c->itens[c->n++] = p;
    return true;
}

typedef struct {
    float dt;
    Vector3 min, max;
} ContextoParticulas;

static void move_e_envelhece(void *elemento, void *contexto) {
    Particula *p = elemento;
    const ContextoParticulas *ctx = contexto;
    p->vel.y -= (p->tipo == PARTICULA_POEIRA ? 0.15f : 2.5f) * ctx->dt; // poeira quase flutua
    p->pos = Vector3Add(p->pos, Vector3Scale(p->vel, ctx->dt));
    p->vida -= ctx->dt;
}

static bool viva_e_dentro(const void *elemento, void *contexto) {
    const Particula *p = elemento;
    const ContextoParticulas *ctx = contexto;
    bool dentro = p->pos.x >= ctx->min.x && p->pos.x <= ctx->max.x && p->pos.y >= ctx->min.y &&
                  p->pos.y <= ctx->max.y && p->pos.z >= ctx->min.z && p->pos.z <= ctx->max.z;
    return p->vida > 0.0f && dentro;
}

void col_particulas_atualiza(ColParticulas *c, float dt, Vector3 limite_min, Vector3 limite_max) {
    ContextoParticulas ctx = {dt, limite_min, limite_max};
    // [REQ:MAP-particulas] move e envelhece cada partícula (fx_map genérico)
    fx_map(c->itens, c->n, sizeof(Particula), move_e_envelhece, &ctx);
    // [REQ:FILTER-particulas] remove as mortas e as que saíram do mapa (fx_filter_inplace genérico)
    c->n = fx_filter_inplace(c->itens, c->n, sizeof(Particula), viva_e_dentro, &ctx);
}

// ================================================================== rachaduras
bool col_rachaduras_adiciona(ColRachaduras *c, Rachadura r) {
    if (!tem_espaco(c->n, MAX_RACHADURAS, "rachaduras")) return false;
    c->itens[c->n++] = r;
    return true;
}

void col_rachaduras_atualiza(ColRachaduras *c, float dt) {
    // [REQ:MAP-rachaduras] anima o crescimento de cada rachadura até o comprimento final
    // padrão MAP
    for (int i = 0; i < c->n; i++) {
        Rachadura *r = &c->itens[i];
        r->comprimento += (r->comprimento_alvo - r->comprimento) * fminf(1.0f, 9.0f * dt);
    }
}

// ================================================================== eventos do placar
bool col_eventos_adiciona(ColEventos *c, EventoPlacar ev) {
    if (!tem_espaco(c->n, MAX_EVENTOS, "eventos")) return false;
    c->itens[c->n++] = ev;
    return true;
}

// ================================================================== interativos
bool col_interativos_adiciona(ColInterativos *c, Interativo it) {
    if (!tem_espaco(c->n, MAX_INTERATIVOS, "interativos")) return false;
    c->itens[c->n++] = it;
    return true;
}

static bool ativo_na_fase(const void *elemento, void *contexto) {
    const Interativo *it = elemento;
    return regras_interativo_ativo(it->tipo, *(const int *)contexto);
}

// [REQ:FILTER-interativos] na entrada de cada fase, mantém só os interativos ativos nela
void col_interativos_filtra_fase(ColInterativos *c, int fase) {
    c->n = fx_filter_inplace(c->itens, c->n, sizeof(Interativo), ativo_na_fase, &fase);
}

static BoundingBox caixa_de(Vector3 centro, Vector3 meio) {
    return (BoundingBox){Vector3Subtract(centro, meio), Vector3Add(centro, meio)};
}

// [REQ:FOLD-interativos] [REQ:RF4] acha o interativo mais próximo no campo de visão (GetRayCollisionBox na mira)
int col_interativos_na_mira(const ColInterativos *c, Vector3 olho, Vector3 frente, float alcance) {
    Ray mira = {olho, Vector3Normalize(frente)};
    float limite_cos = cosf(ANGULO_INTERACAO_GRAUS * DEG2RAD);
    int melhor = -1;
    float melhor_dist = alcance;
    // padrão FOLD
    for (int i = 0; i < c->n; i++) {
        const Interativo *it = &c->itens[i];
        RayCollision acerto = GetRayCollisionBox(mira, caixa_de(it->pos, it->meio_tamanho));
        Vector3 para = Vector3Subtract(it->pos, olho);
        float dist = acerto.hit ? acerto.distance : Vector3Length(para);
        bool no_cone = acerto.hit || Vector3DotProduct(Vector3Normalize(para), mira.direction) >= limite_cos;
        if (no_cone && dist <= melhor_dist) {
            melhor_dist = dist;
            melhor = i;
        }
    }
    return melhor;
}

// ================================================================== paredes
bool col_paredes_adiciona(ColParedes *c, Parede p) {
    if (!tem_espaco(c->n, MAX_PAREDES, "paredes")) return false;
    c->itens[c->n++] = p;
    return true;
}

typedef struct {
    BoundingBox corpo;
    bool colidiu;
} ContextoColisao;

static void colide_com_parede(void *acumulador, const void *elemento, void *contexto) {
    (void)contexto;
    ContextoColisao *acc = acumulador;
    const Parede *p = elemento;
    acc->colidiu = acc->colidiu || CheckCollisionBoxes(acc->corpo, (BoundingBox){p->min, p->max});
}

// [REQ:FOLD-paredes] [REQ:RF4] "colidiu com alguma?" é um fold com OU sobre CheckCollisionBoxes (fx_fold genérico)
bool col_paredes_colide(const ColParedes *c, Vector3 centro, float raio, float altura) {
    ContextoColisao acc = {
        .corpo = {{centro.x - raio, centro.y, centro.z - raio}, {centro.x + raio, centro.y + altura, centro.z + raio}},
        .colidiu = false,
    };
    fx_fold(c->itens, c->n, sizeof(Parede), &acc, colide_com_parede, NULL);
    return acc.colidiu;
}

// [REQ:RF4] distância até a primeira parede na direção do raio (a visão da criatura usa isto)
float col_paredes_distancia_raio(const ColParedes *c, Vector3 origem, Vector3 direcao, float alcance) {
    Ray raio = {origem, Vector3Normalize(direcao)};
    float menor = alcance;
    // padrão FOLD
    for (int i = 0; i < c->n; i++) {
        RayCollision acerto = GetRayCollisionBox(raio, (BoundingBox){c->itens[i].min, c->itens[i].max});
        if (acerto.hit && acerto.distance < menor) menor = acerto.distance;
    }
    return menor;
}
