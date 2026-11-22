// respiracao.c — o desafio do armário: tutorial (1ª vez na partida), desafio e alívio. As contas da linha
// e do acerto são puras (regras_respiracao_*); aqui fica o fluxo, o som e o desenho.
#include "respiracao.h"
#include "criatura.h"
#include "audio_sintese.h"
#include "ui.h"
#include "dialogo.h"
#include <math.h>

#define DEMO_PAUSA_S 1.2f // tutorial: a linha fica parada na faixa ("assim!") por esse tempo

bool respiracao_ativa(const Estado *e) {
    return e->resp.etapa != RESP_NADA;
}

bool respiracao_congela_mundo(const Estado *e) {
    return e->resp.etapa == RESP_TUTORIAL;
}

bool respiracao_segura_criatura(const Estado *e) {
    return e->resp.etapa == RESP_TUTORIAL || e->resp.etapa == RESP_DESAFIO;
}

static float sorteia_zona(void) {
    return GetRandomValue(22, 78) / 100.0f; // nunca colada nas pontas da barra
}

// ================================================================== fluxo
static void comeca_desafio(Estado *e) {
    Respiracao *r = &e->resp;
    r->etapa = RESP_DESAFIO;
    r->t = 0.0f;
    r->erros = 0;
    r->zona = sorteia_zona();
    criatura_na_porta_do_armario(e);
}

void respiracao_comeca(Estado *e) {
    if (!regras_fase_eh_saida(e->fase) || e->painel.resolvido) return; // energia de volta: ele está contido
    Respiracao *r = &e->resp;
    if (r->tutorial_visto) {
        comeca_desafio(e);
        return;
    }
    r->etapa = RESP_TUTORIAL;
    r->t = 0.0f;
    r->zona = 0.62f;
    r->demo_t = -1.0f;
}

// Tutorial: a linha anda sozinha; na 1ª passagem pela faixa ela para ("assim!") e depois segue.
static float linha_tutorial(const Respiracao *r) {
    if (r->demo_t < 0.0f) return regras_respiracao_linha(r->t, RESP_PERIODO_S);
    if (r->t - r->demo_t < DEMO_PAUSA_S) return r->demo_linha;
    return regras_respiracao_linha(r->t - DEMO_PAUSA_S, RESP_PERIODO_S);
}

static bool tutorial_pronto(const Respiracao *r) {
    return r->demo_t >= 0.0f && r->t - r->demo_t >= DEMO_PAUSA_S;
}

static void atualiza_tutorial(Estado *e, float dt) {
    Respiracao *r = &e->resp;
    r->t += dt;
    float linha = linha_tutorial(r);
    if (r->demo_t < 0.0f && r->t > 1.0f && regras_respiracao_acertou(linha, r->zona, RESP_ZONA * 0.5f)) {
        r->demo_t = r->t;
        r->demo_linha = linha;
    }
    if (!tutorial_pronto(r) || !e->in.respirar) return;
    r->tutorial_visto = true;
    comeca_desafio(e);
}

// Um som: ele chega mais perto e a linha acelera. No 3º, ele abre o armário.
static void erra(Estado *e) {
    Respiracao *r = &e->resp;
    r->erros++;
    r->aviso_t = 0.9f;
    if (r->erros >= RESP_CHANCES) {
        r->etapa = RESP_NADA;
        criatura_captura(e);
        return;
    }
    r->t = 0.0f;
    r->zona = sorteia_zona();
    criatura_chega_mais_perto(e);
}

static void atualiza_desafio(Estado *e, float dt) {
    Respiracao *r = &e->resp;
    r->t += dt;
    float linha = regras_respiracao_linha(r->t, regras_respiracao_periodo(r->erros));
    if (e->in.respirar) {
        if (!regras_respiracao_acertou(linha, r->zona, RESP_ZONA)) {
            erra(e);
            return;
        }
        r->etapa = RESP_ALIVIO; // segurou: ele desiste e vai embora
        r->t = 0.0f;
        criatura_desiste(e);
        return;
    }
    if (r->t >= RESP_PRAZO_S) erra(e); // não segurou a tempo: o ar escapa
}

void respiracao_atualiza(Estado *e, float dt) {
    Respiracao *r = &e->resp;
    r->aviso_t = fmaxf(0.0f, r->aviso_t - dt);
    if (r->etapa != RESP_NADA && e->jog.modo != MODO_ARMARIO) { // pego, recomeço: o desafio acaba
        r->etapa = RESP_NADA;
        return;
    }
    // Uma etapa por quadro: o ESPAÇO que fecha o tutorial não pode contar como tentativa no desafio.
    if (r->etapa == RESP_TUTORIAL) {
        atualiza_tutorial(e, dt);
    } else if (r->etapa == RESP_DESAFIO) {
        atualiza_desafio(e, dt);
    } else if (r->etapa == RESP_ALIVIO) {
        r->t += dt;
        if (r->t < RESP_ALIVIO_S) return;
        r->etapa = RESP_NADA;
        audio_toca(e, SOM_RESPIRACAO_OFEGANTE, 0.8f, 1.0f); // enfim, solta o ar
    }
}

void respiracao_piloto(const Estado *e, Entrada *in) {
    const Respiracao *r = &e->resp;
    if (r->etapa == RESP_TUTORIAL) in->respirar = tutorial_pronto(r);
    if (r->etapa != RESP_DESAFIO) return;
    float linha = regras_respiracao_linha(r->t, regras_respiracao_periodo(r->erros));
    in->respirar = regras_respiracao_acertou(linha, r->zona, RESP_ZONA * 0.5f); // com folga: o quadro anda
}

// ================================================================== desenho
// A barra: contorno verde, a faixa branca parada e a linha vermelha (que passa um pouco das bordas).
static void barra(Rectangle b, float linha, float zona) {
    float s = ui_escala();
    DrawRectangleRec(b, Fade(BLACK, 0.75f));
    DrawRectangleRec(
        (Rectangle){b.x + (zona - RESP_ZONA * 0.5f) * b.width, b.y, RESP_ZONA * b.width, b.height}, RAYWHITE);
    DrawRectangleLinesEx(b, 3.0f * s, COR_FOSFORO);
    float x = b.x + linha * b.width;
    DrawRectangleRec((Rectangle){x - 2.5f * s, b.y - 9 * s, 5 * s, b.height + 18 * s},
                     (Color){255, 40, 40, 255});
}

static void desenha_tutorial(const Estado *e, Rectangle b) {
    const Respiracao *r = &e->resp;
    float s = ui_escala(), w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangle(0, 0, (int)w, (int)h, Fade(BLACK, 0.72f));
    ui_texto_centro(e, dialogo_texto(TXT_RESP_TITULO), h * 0.14f, 40 * s, COR_AMBAR);
    ui_texto_quebrado(e, dialogo_texto(TXT_RESP_TUTORIAL), (Vector2){w * 0.5f - 360 * s, h * 0.23f}, 720 * s,
                      22 * s, RAYWHITE);
    float linha = linha_tutorial(r);
    barra(b, linha, r->zona);
    if (r->demo_t >= 0.0f && r->t - r->demo_t < DEMO_PAUSA_S) {
        const char *assim = dialogo_texto(TXT_RESP_ASSIM);
        float x = b.x + linha * b.width - ui_mede(e, assim, 24 * s).x * 0.5f;
        ui_texto(e, assim, (Vector2){x, b.y - 44 * s}, 24 * s, COR_AMBAR);
    }
    if (tutorial_pronto(r) && fmodf(r->t, 1.0f) < 0.7f)
        ui_texto_centro(e, dialogo_texto(TXT_RESP_COMECAR), b.y + b.height + 40 * s, 24 * s, COR_FOSFORO);
}

// As chances: uma bolinha por fôlego que ainda resta (as gastas ficam vazias e vermelhas).
static void chances(const Estado *e, Rectangle b) {
    float s = ui_escala();
    for (int i = 0; i < RESP_CHANCES; i++) { // padrão MAP: cada chance
        Vector2 c = {b.x + b.width + 30 * s + i * 26 * s, b.y + b.height * 0.5f};
        bool resta = i < RESP_CHANCES - e->resp.erros;
        if (resta) DrawCircleV(c, 8 * s, COR_FOSFORO);
        DrawCircleLinesV(c, 8 * s, resta ? COR_FOSFORO : COR_ALARME);
    }
}

void respiracao_desenha(const Estado *e) {
    const Respiracao *r = &e->resp;
    if (r->etapa == RESP_NADA && r->aviso_t <= 0.0f) return;
    float s = ui_escala(), w = GetScreenWidth(), h = GetScreenHeight();
    Rectangle b = {w * 0.5f - 300 * s, h * 0.46f, 600 * s, 40 * s}; // acima da legenda do rádio
    if (r->etapa == RESP_TUTORIAL) {
        desenha_tutorial(e, b);
        return;
    }
    if (r->etapa == RESP_DESAFIO) {
        ui_texto_centro(e, dialogo_texto(TXT_RESP_TITULO), b.y - 58 * s, 26 * s, COR_AMBAR);
        barra(b, regras_respiracao_linha(r->t, regras_respiracao_periodo(r->erros)), r->zona);
        chances(e, b);
        float resta = fmaxf(0.0f, 1.0f - r->t / RESP_PRAZO_S); // quanto ele ainda espera na porta
        DrawRectangleRec((Rectangle){b.x, b.y + b.height + 14 * s, b.width * resta, 5 * s},
                         Fade(COR_AMBAR, 0.8f));
    }
    if (r->etapa == RESP_ALIVIO)
        ui_texto_centro(e, dialogo_texto(TXT_RESP_FOI), b.y, 26 * s,
                        Fade(COR_FOSFORO, 1.0f - r->t / RESP_ALIVIO_S));
    if (r->aviso_t > 0.0f)
        ui_texto_centro(e, dialogo_texto(TXT_RESP_OUVIU), b.y - 110 * s, 34 * s, COR_ALARME);
}
