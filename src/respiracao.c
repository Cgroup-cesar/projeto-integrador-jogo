// respiracao.c — o desafio do armário: a espera (se ele vinha atrás de quem se escondeu, vai até a porta),
// o desafio (3 acertos, 3 chances; a tecla ESPAÇO desenhada embaixo da barra é toda a explicação) e o
// afastamento (o E só volta quando ele está longe de verdade). As contas são puras (regras_respiracao_*,
// regras_armario_*); aqui fica o fluxo, o som e o desenho.
#include "respiracao.h"
#include "criatura.h"
#include "audio_sintese.h"
#include "ui.h"
#include "dialogo.h"
#include <math.h>

bool respiracao_ativa(const Estado *e) {
    return e->resp.etapa != RESP_NADA;
}

bool respiracao_segura_criatura(const Estado *e) {
    return e->resp.etapa == RESP_DESAFIO;
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
    r->acertos = 0;
    r->zona = sorteia_zona();
    criatura_na_porta_do_armario(e);
}

void respiracao_comeca(Estado *e) {
    Respiracao *r = &e->resp;
    r->perseguido = false;
    if (!regras_fase_eh_saida(e->fase) || e->painel.resolvido) return; // energia de volta: ele está contido
    r->perseguido = regras_armario_perseguido(e->cri.ativo, e->cri.estado, e->cri.ve_jogador);
}

// Escondido com ele na cola: ele vai até o armário (regras_proximo_estado_criatura com sabe_esconderijo) e,
// ao chegar perto, para na porta e começa a respiração.
static void espera_no_armario(Estado *e) {
    Respiracao *r = &e->resp;
    if (!r->perseguido || e->jog.modo != MODO_ARMARIO) return;
    if (e->painel.resolvido || !e->cri.ativo) { // a energia voltou: a contenção o puxou
        r->perseguido = false;
        return;
    }
    float d = Vector3Distance(e->cri.pos, e->nivel.armarios[e->jog.armario]);
    if (!regras_armario_chegou(r->perseguido, e->cri.estado, d)) return;
    r->perseguido = false;
    r->foi_t = 0.0f;
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

// Segurou mais uma vez. No 3º acerto ele desiste e começa a se afastar.
static void acerta(Estado *e) {
    Respiracao *r = &e->resp;
    r->acertos++;
    r->acerto_t = 0.7f;
    r->t = 0.0f;
    if (r->acertos < RESP_ACERTOS) {
        r->zona = sorteia_zona();
        return;
    }
    r->etapa = RESP_ALIVIO;
    criatura_desiste(e);
}

static void atualiza_desafio(Estado *e, float dt) {
    Respiracao *r = &e->resp;
    r->t += dt;
    float linha = regras_respiracao_linha(r->t, regras_respiracao_periodo(r->erros));
    if (e->in.respirar) {
        if (regras_respiracao_acertou(linha, r->zona, RESP_ZONA))
            acerta(e);
        else
            erra(e);
        return;
    }
    if (r->t >= RESP_PRAZO_S) erra(e); // não segurou a tempo: o ar escapa
}

// Ele se afasta andando. Só quando está longe de verdade a pessoa solta o ar e pode sair; se em
// RESP_AFASTA_MAX_S ele ainda não se afastou (preso num canto), vai para o ponto de patrulha mais longe.
static void atualiza_afastamento(Estado *e, float dt) {
    Respiracao *r = &e->resp;
    r->t += dt;
    bool longe = Vector3Distance(e->cri.pos, e->jog.pos) >= RESP_LONGE_M;
    if (!longe && r->t < RESP_AFASTA_MAX_S) return;
    if (!longe) criatura_vai_para_longe(e);
    r->etapa = RESP_NADA;
    r->foi_t = RESP_FOI_S;
    audio_toca(e, SOM_RESPIRACAO_OFEGANTE, 0.8f, 1.0f); // enfim, solta o ar
}

void respiracao_atualiza(Estado *e, float dt) {
    Respiracao *r = &e->resp;
    r->aviso_t = fmaxf(0.0f, r->aviso_t - dt);
    r->acerto_t = fmaxf(0.0f, r->acerto_t - dt);
    r->foi_t = fmaxf(0.0f, r->foi_t - dt);
    if (r->etapa != RESP_NADA && e->jog.modo != MODO_ARMARIO) { // pego, recomeço: o desafio acaba
        r->etapa = RESP_NADA;
        return;
    }
    // Uma etapa por quadro: o desafio só lê o ESPAÇO a partir do quadro seguinte ao que ele chegou.
    if (r->etapa == RESP_NADA)
        espera_no_armario(e);
    else if (r->etapa == RESP_DESAFIO)
        atualiza_desafio(e, dt);
    else if (r->etapa == RESP_ALIVIO)
        atualiza_afastamento(e, dt);
}

void respiracao_piloto(const Estado *e, Entrada *in) {
    const Respiracao *r = &e->resp;
    if (r->etapa != RESP_DESAFIO) return;
    float linha = regras_respiracao_linha(r->t, regras_respiracao_periodo(r->erros));
    in->respirar = regras_respiracao_acertou(linha, r->zona, RESP_ZONA * 0.5f); // com folga: o quadro anda
}

// ================================================================== desenho
// A barra: contorno verde, a faixa branca parada e a linha vermelha (que passa um pouco das bordas).
static void barra(Rectangle b, float linha, float zona, Color borda) {
    float s = ui_escala();
    DrawRectangleRec(b, Fade(BLACK, 0.75f));
    DrawRectangleRec(
        (Rectangle){b.x + (zona - RESP_ZONA * 0.5f) * b.width, b.y, RESP_ZONA * b.width, b.height}, RAYWHITE);
    DrawRectangleLinesEx(b, 3.0f * s, borda);
    float x = b.x + linha * b.width;
    DrawRectangleRec((Rectangle){x - 2.5f * s, b.y - 9 * s, 5 * s, b.height + 18 * s},
                     (Color){255, 40, 40, 255});
}

// Uma tecla desenhada (a mesma cara da barra de teclas); `acesa`: apertada agora. Devolve a largura.
static float tecla(const Estado *e, const char *rotulo, Vector2 pos, float altura, bool acesa) {
    float s = ui_escala(), t = altura * 0.5f, largura = ui_mede(e, rotulo, t).x + 24 * s;
    Rectangle r = {pos.x, pos.y + (acesa ? 3 * s : 0.0f), largura, altura};
    if (!acesa)
        DrawRectangleRounded((Rectangle){r.x, r.y + 3 * s, r.width, r.height}, 0.25f, 4, Fade(BLACK, 0.8f));
    DrawRectangleRounded(r, 0.25f, 4, acesa ? COR_FOSFORO : (Color){22, 34, 26, 255});
    DrawRectangleRoundedLinesEx(r, 0.25f, 4, 1.5f * s, COR_FOSFORO);
    Vector2 m = ui_mede(e, rotulo, t);
    ui_texto(e, rotulo, (Vector2){r.x + (r.width - m.x) * 0.5f, r.y + (r.height - m.y) * 0.5f}, t,
             acesa ? (Color){8, 14, 10, 255} : COR_FOSFORO);
    return largura;
}

// Os indicadores do desafio: um quadrado por acerto (cheio quando conseguiu) e uma bolinha por chance
// (vazia e vermelha quando gasta).
static void acertos(Vector2 pos, int feitos, float s) {
    for (int i = 0; i < RESP_ACERTOS; i++) { // padrão MAP: cada acerto que falta ou já foi
        Rectangle q = {pos.x + i * 26 * s, pos.y - 9 * s, 18 * s, 18 * s};
        if (i < feitos) DrawRectangleRec(q, COR_FOSFORO);
        DrawRectangleLinesEx(q, 2 * s, COR_FOSFORO);
    }
}

static void chances(Vector2 pos, int erros, float s) {
    for (int i = 0; i < RESP_CHANCES; i++) { // padrão MAP: cada chance
        Vector2 c = {pos.x + 9 * s + i * 26 * s, pos.y};
        bool resta = i < RESP_CHANCES - erros;
        if (resta) DrawCircleV(c, 9 * s, COR_FOSFORO);
        DrawCircleLinesV(c, 9 * s, resta ? COR_FOSFORO : COR_ALARME);
    }
}

// Desafio: a barra no meio, os acertos à esquerda, as chances à direita e quanto ele ainda espera embaixo.
static void desenha_desafio(const Estado *e, Rectangle b) {
    const Respiracao *r = &e->resp;
    float s = ui_escala();
    ui_texto_centro(e, dialogo_texto(TXT_RESP_TITULO), b.y - 58 * s, 26 * s, COR_AMBAR);
    Color borda = r->acerto_t > 0.0f ? (Color){240, 250, 242, 255} : COR_FOSFORO; // pisca branca ao acertar
    barra(b, regras_respiracao_linha(r->t, regras_respiracao_periodo(r->erros)), r->zona, borda);
    float ye = b.y + b.height * 0.5f;
    const char *rot_a = dialogo_texto(TXT_RESP_ACERTOS), *rot_c = dialogo_texto(TXT_RESP_CHANCES);
    float xa = b.x - 26 * s * RESP_ACERTOS - 18 * s;
    ui_texto(e, rot_a, (Vector2){xa, b.y - 22 * s}, 13 * s, Fade(COR_FOSFORO, 0.7f));
    acertos((Vector2){xa, ye}, r->acertos, s);
    float xc = b.x + b.width + 22 * s;
    ui_texto(e, rot_c, (Vector2){xc, b.y - 22 * s}, 13 * s, Fade(COR_FOSFORO, 0.7f));
    chances((Vector2){xc, ye}, r->erros, s);
    float resta = fmaxf(0.0f, 1.0f - r->t / RESP_PRAZO_S); // quanto ele ainda espera na porta
    DrawRectangleRec((Rectangle){b.x, b.y + b.height + 14 * s, b.width * resta, 5 * s},
                     Fade(COR_AMBAR, 0.8f));
    // A única explicação: a tecla ESPAÇO embaixo da barra (afunda ao apertar e logo depois do acerto).
    float alt = 40 * s, larg = ui_mede(e, "ESPAÇO", alt * 0.5f).x + 24 * s;
    tecla(e, "ESPAÇO", (Vector2){b.x + (b.width - larg) * 0.5f, b.y + b.height + 34 * s}, alt,
          e->in.respirar || r->acerto_t > 0.5f);
    if (r->acerto_t > 0.0f)
        ui_texto_centro(e, dialogo_texto(TXT_RESP_SEGUROU), b.y - 110 * s, 34 * s, COR_FOSFORO);
}

void respiracao_desenha(const Estado *e) {
    const Respiracao *r = &e->resp;
    if (r->etapa == RESP_NADA && r->aviso_t <= 0.0f && r->foi_t <= 0.0f) return;
    float s = ui_escala(), w = GetScreenWidth(), h = GetScreenHeight();
    Rectangle b = {w * 0.5f - 300 * s, h * 0.46f, 600 * s, 40 * s}; // acima da legenda do rádio
    if (r->etapa == RESP_DESAFIO) desenha_desafio(e, b);
    if (r->etapa == RESP_ALIVIO)
        ui_texto_centro(e, dialogo_texto(TXT_RESP_AFASTANDO), b.y, 24 * s, COR_AMBAR);
    if (r->foi_t > 0.0f)
        ui_texto_centro(e, dialogo_texto(TXT_RESP_FOI), b.y, 26 * s,
                        Fade(COR_FOSFORO, fminf(1.0f, r->foi_t)));
    if (r->aviso_t > 0.0f)
        ui_texto_centro(e, dialogo_texto(TXT_RESP_OUVIU), b.y - 110 * s, 34 * s, COR_ALARME);
}
