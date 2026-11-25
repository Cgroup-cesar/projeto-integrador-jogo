// lanterna.c — a lanterna de quem joga. F liga e desliga (sem bateria: a luz é sempre cheia). A luz é um
// cone saindo dos olhos; o modelo fica na mão, no canto da tela.
#include "lanterna.h"
#include "jogador.h"
#include "audio_sintese.h"
#include "ui.h"
#include "rlgl.h"
#include <math.h>

// O modelo vem deitado no eixo X. +1: a lente fica no lado +X; -1: no lado -X. O lanterna.glb atual tem a
// lente no lado -X (com +1 ela aparecia virada para o jogador).
#define LENTE_EM_X -1.0f

// ================================================================== modelo opcional
// [REQ:RNF4] asset ausente: sem o .glb, entra a lanterna de cilindros e só fica um TraceLog
void lanterna_carrega_modelo(Recursos *rec) {
    char caminho[512];
    rec->tem_lanterna = false;
    if (!ui_caminho_asset("modelos/lanterna.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/lanterna.glb; usando a lanterna de cilindros");
        return;
    }
    rec->modelo_lanterna = LoadModel(caminho);
    rec->tem_lanterna = IsModelValid(rec->modelo_lanterna) && rec->modelo_lanterna.meshCount > 0;
    if (!rec->tem_lanterna) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; usando a lanterna de cilindros", caminho);
        return;
    }
    for (int i = 0; i < rec->modelo_lanterna.materialCount && rec->luz_ok; i++)
        rec->modelo_lanterna.materials[i].shader = rec->luz;
}

void lanterna_descarrega_modelo(Recursos *rec) {
    if (rec->tem_lanterna) UnloadModel(rec->modelo_lanterna);
}

// ================================================================== estado
void lanterna_reinicia(Estado *e) {
    e->lanterna.ligada = false;
    e->lanterna.ja_acesa = false;
}

// Só ilumina quando há jogo na tela (no menu e nos finais ela não aparece).
static bool acesa(const Estado *e) {
    bool fora_do_jogo = e->fase == FASE_MENU || e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM;
    return e->lanterna.ligada && !fora_do_jogo;
}

void lanterna_atualiza(Estado *e, float dt) {
    (void)dt; // sem bateria, nada depende do tempo
    Lanterna *l = &e->lanterna;
    if (e->jog.modo == MODO_ARMARIO) l->ligada = false; // escondido não se acende luz
    if (e->in.lanterna && e->jog.modo == MODO_ANDANDO) {
        l->ligada = !l->ligada;
        l->ja_acesa = l->ja_acesa || l->ligada;
        audio_toca(e, SOM_INTERRUPTOR, 0.45f, l->ligada ? 1.7f : 1.5f);
    }
}

// ================================================================== a luz
// De onde sai a luz: um pouco atrás dos olhos, para o cone já abrir no rosto.
static Vector3 origem_da_luz(const Estado *e) {
    return Vector3Add(jogador_olho(e), Vector3Scale(jogador_direcao(e), -0.15f));
}

bool lanterna_mira_criatura(const Estado *e) {
    const Criatura *c = &e->cri;
    if (!acesa(e) || (!c->contido && !c->ativo)) return false;
    Vector3 olho = origem_da_luz(e),
            peito = Vector3Add(c->pos, (Vector3){0.0f, esqueleto_modelo()->peito_m, 0.0f});
    float abertura = cosf(LANTERNA_ABERTURA_GRAUS * DEG2RAD);
    if (!regras_no_cone(olho, jogador_direcao(e), abertura, CRIATURA_LANTERNA_ALCANCE_M, peito)) return false;
    if (c->contido) return true;
    float dist = Vector3Distance(olho, peito);
    Vector3 para = Vector3Normalize(Vector3Subtract(peito, olho));
    return col_paredes_distancia_raio(&e->paredes, olho, para, dist) >= dist - 0.3f;
}

// 1 normal; mirando o Precursor, a força oscila como uma lanterna com mau contato.
static float forca(const Estado *e) {
    return lanterna_mira_criatura(e) ? regras_falha_lanterna(e->tempo_total) : 1.0f;
}

bool lanterna_luz(const Estado *e, Vector3 *pos, Vector3 *cor, Vector4 *cone, float *queda) {
    if (!acesa(e)) return false;
    Vector3 frente = jogador_direcao(e);
    *pos = origem_da_luz(e);
    Color c = COR_LANTERNA;
    float k = LANTERNA_INTENSIDADE * forca(e);
    *cor = (Vector3){c.r / 255.0f * k, c.g / 255.0f * k, c.b / 255.0f * k};
    *cone = (Vector4){frente.x, frente.y, frente.z, cosf(LANTERNA_ABERTURA_GRAUS * DEG2RAD)};
    *queda = LANTERNA_QUEDA; // enfraquece mais devagar que as lâmpadas: alcança o fundo do corredor
    return true;
}

// ================================================================== na mão
typedef struct {
    Vector3 pos;                   // centro da lanterna
    Vector3 frente, direita, cima; // base da câmera
} Mao;

// Abaixo e à direita do olhar, balançando com os passos (o mesmo balanço da câmera).
static Mao mao(const Estado *e) {
    Mao m;
    m.frente = jogador_direcao(e);
    m.direita = Vector3Normalize(Vector3CrossProduct(m.frente, (Vector3){0.0f, 1.0f, 0.0f}));
    m.cima = Vector3CrossProduct(m.direita, m.frente);
    float passo = sinf(e->jog.balanco);
    m.pos = Vector3Add(jogador_olho(e), Vector3Scale(m.frente, 0.45f));
    m.pos = Vector3Add(m.pos, Vector3Scale(m.direita, 0.19f + passo * 0.008f));
    m.pos = Vector3Add(m.pos, Vector3Scale(m.cima, -0.15f - fabsf(passo) * 0.012f));
    return m;
}

static Vector3 lente(Mao m) {
    return Vector3Add(m.pos, Vector3Scale(m.frente, LANTERNA_COMPRIMENTO_M * 0.5f));
}

// O eixo X do modelo vira a frente do olhar; o resto da base sai do produto vetorial (rotação própria).
static void desenha_modelo(const Estado *e, Mao m) {
    const Model *modelo = &e->rec.modelo_lanterna;
    BoundingBox caixa = GetModelBoundingBox(*modelo);
    Vector3 centro = Vector3Scale(Vector3Add(caixa.min, caixa.max), 0.5f);
    float escala = LANTERNA_COMPRIMENTO_M / fmaxf(0.001f, caixa.max.x - caixa.min.x);
    Vector3 x = Vector3Scale(m.frente, LENTE_EM_X), z = Vector3CrossProduct(x, m.cima);
    float base[16] = {x.x, x.y, x.z, 0, m.cima.x, m.cima.y, m.cima.z, 0, z.x, z.y, z.z, 0, 0, 0, 0, 1};
    rlPushMatrix();
    rlTranslatef(m.pos.x, m.pos.y, m.pos.z);
    rlMultMatrixf(base);
    rlScalef(escala, escala, escala);
    DrawModel(*modelo, Vector3Negate(centro), 1.0f, WHITE);
    rlPopMatrix();
}

static void desenha_cilindros(Mao m) {
    float meio = LANTERNA_COMPRIMENTO_M * 0.5f;
    Vector3 tras = Vector3Add(m.pos, Vector3Scale(m.frente, -meio));
    Vector3 cabeca = Vector3Add(m.pos, Vector3Scale(m.frente, meio * 0.6f));
    DrawCylinderEx(tras, cabeca, 0.022f, 0.022f, 10, (Color){0x2a, 0x2c, 0x2a, 255});
    DrawCylinderEx(cabeca, lente(m), 0.024f, 0.036f, 10, (Color){0x3a, 0x3c, 0x38, 255});
}

static bool na_mao(const Estado *e) {
    bool jogando = e->fase != FASE_MENU && e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM;
    return jogando && e->jog.modo == MODO_ANDANDO && e->jumpscare_t <= 0.0f;
}

void lanterna_desenha(const Estado *e) {
    if (!na_mao(e)) return;
    if (e->rec.tem_lanterna)
        desenha_modelo(e, mao(e));
    else
        desenha_cilindros(mao(e));
}

void lanterna_desenha_brilho(const Estado *e) {
    if (!na_mao(e) || !acesa(e)) return;
    Mao m = mao(e);
    Color c = COR_LANTERNA;
    float f = forca(e); // a lente também falha
    Color lente_acesa = {(unsigned char)(c.r * f), (unsigned char)(c.g * f), (unsigned char)(c.b * f), 255};
    DrawSphereEx(Vector3Add(lente(m), Vector3Scale(m.frente, 0.004f)), 0.02f, 8, 8, lente_acesa);
}
