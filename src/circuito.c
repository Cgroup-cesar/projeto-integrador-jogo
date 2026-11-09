// circuito.c — painéis das saídas. Painel 1 (gerador): chaves A, B, C e SAÍDA = (A ∧ ¬B) ∧ C.
// Painel 2 (Arquivo): SAÍDA = (A [?1] B) [?2] (¬A), com alvo 00→1, 01→1, 10→0, 11→1.
// A checagem sempre avalia o circuito (regras_avaliar_circuito); nunca compara com uma resposta fixa.
#include "circuito.h"
#include "nivel.h"
#include "criatura.h"
#include "audio_sintese.h"
#include "ui.h"
#include "dialogo.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const bool ALVO_PAINEL2[4] = {true, true, false, true};

// ================================================================== circuito do painel
static void monta(const Painel *p, Circuito *c) {
    if (p->numero == 1) regras_circuito_painel1(c);
    else regras_circuito_painel2(c, (TipoPorta)p->escolha[0], (TipoPorta)p->escolha[1]);
}

static bool esta_resolvido(const Painel *p) {
    Circuito c;
    monta(p, &c);
    if (p->numero == 1) return regras_avaliar_circuito(&c, p->chaves, NULL);
    return regras_circuito_confere_tabela(&c, ALVO_PAINEL2);
}

// No painel 2 as entradas não são chaves: os LEDs percorrem as 4 linhas da tabela, uma por vez.
static int linha_mostrada(const Estado *e) {
    return (int)(e->tempo_total / 0.9f) % 4;
}

static bool avalia_mostrado(const Estado *e, bool *leds) {
    const Painel *p = &e->painel;
    Circuito c;
    monta(p, &c);
    if (p->numero == 1) return regras_avaliar_circuito(&c, p->chaves, leds);
    bool entradas[2];
    logica_valores_da_linha(linha_mostrada(e), 2, entradas);
    return regras_avaliar_circuito(&c, entradas, leds);
}

// ================================================================== início
void circuito_inicia(Estado *e, int numero) {
    memset(&e->painel, 0, sizeof(e->painel));
    e->painel.numero = numero;
    e->painel.escolha[0] = PORTA_OU; // começa errado de propósito: (A ∨ B) ∧ ¬A não bate com o alvo
    e->painel.escolha[1] = PORTA_E;
}

// ================================================================== atualiza
static bool atualiza_painel1(Estado *e) {
    Painel *p = &e->painel;
    bool teclas[3] = {e->in.tecla1, e->in.tecla2, e->in.tecla3};
    if (e->in.cima) p->cursor = (p->cursor + 2) % 3;
    if (e->in.baixo) p->cursor = (p->cursor + 1) % 3;
    if (e->in.confirmar) teclas[p->cursor] = true;
    bool mexeu = false;
    for (int i = 0; i < 3; i++) {
        if (!teclas[i]) continue;
        p->chaves[i] = !p->chaves[i];
        mexeu = true;
    }
    return mexeu;
}

static bool atualiza_painel2(Estado *e) {
    Painel *p = &e->painel;
    if (e->in.esquerda || e->in.direita) p->cursor = 1 - (p->cursor % 2);
    int passo = (e->in.cima ? -1 : 0) + (e->in.baixo || e->in.confirmar ? 1 : 0);
    if (passo == 0) return e->in.esquerda || e->in.direita;
    int encaixe = p->cursor % 2;
    p->escolha[encaixe] = (p->escolha[encaixe] + passo + 4) % 4; // E, OU, OU-EXCLUSIVO, NÃO-E
    return true;
}

// Painel resolvido: a energia volta, a criatura é puxada pela contenção e o Varela manda voltar.
static void resolve(Estado *e) {
    Painel *p = &e->painel;
    p->resolvido = true;
    p->resolvido_t = 0.0f;
    e->falha_luz_t = 0.5f;
    audio_toca(e, SOM_ENERGIA, 1.0f, 1.0f);
    ui_fala(e, p->numero == 1 ? FALAS_FIM_S1 : FALAS_FIM_S2);
    criatura_puxa_para_contencao(e);
}

// Apagões curtos enquanto a energia não volta (uns 4% de chance por segundo).
static void apagoes(Estado *e, float dt) {
    if (e->painel.resolvido || e->apagao_t > 0.0f) return;
    if (GetRandomValue(0, 10000) < (int)(dt * 400.0f)) e->apagao_t = 0.35f + GetRandomValue(0, 30) / 100.0f;
}

void circuito_atualiza(Estado *e, float dt) {
    Painel *p = &e->painel;
    apagoes(e, dt);
    if (p->resolvido) {
        p->resolvido_t += dt;
        if (p->resolvido_t > 1.8f && e->jog.modo == MODO_PAINEL) e->jog.modo = MODO_ANDANDO;
        if (p->resolvido_t > 1.0f && nivel_na_sala_de_observacao(&e->nivel, e->jog.pos)) e->evento = EV_SAIDA_CONCLUIDA;
        return;
    }
    if (e->jog.modo != MODO_PAINEL) return;
    if (e->in.voltar) {
        e->jog.modo = MODO_ANDANDO;
        return;
    }
    bool mexeu = p->numero == 1 ? atualiza_painel1(e) : atualiza_painel2(e);
    if (!mexeu) return;
    // Mexer no painel faz barulho: a criatura pode ouvir.
    Vector3 onde = p->numero == 1 ? e->nivel.painel1 : e->nivel.painel2;
    col_ruidos_adiciona(&e->ruidos, (Ruido){onde, RUIDO_RAIO_PAINEL, 1.0f, RUIDO_PAINEL});
    audio_toca(e, SOM_INTERRUPTOR, 0.9f, 0.8f + GetRandomValue(0, 30) / 100.0f);
    if (esta_resolvido(p)) resolve(e);
}

// ================================================================== desenho 2D
static Color cor_sinal(bool v) {
    return v ? COR_FOSFORO : (Color){0x2a, 0x40, 0x30, 255};
}

static void fio(Vector2 a, Vector2 b, bool v) {
    Vector2 meio = {(a.x + b.x) * 0.5f, a.y};
    DrawLineEx(a, meio, 2.0f, cor_sinal(v));
    DrawLineEx(meio, (Vector2){meio.x, b.y}, 2.0f, cor_sinal(v));
    DrawLineEx((Vector2){meio.x, b.y}, b, 2.0f, cor_sinal(v));
}

// Uma porta lógica: caixa com o nome e o LED da saída ao vivo. Devolve o ponto de saída.
static Vector2 porta(const Estado *e, Vector2 centro, const char *nome, bool led, bool selecionada) {
    float s = ui_escala();
    Rectangle r = {centro.x - 70 * s, centro.y - 22 * s, 140 * s, 44 * s};
    DrawRectangleRounded(r, 0.2f, 4, selecionada ? Fade(COR_AMBAR, 0.25f) : (Color){12, 20, 15, 255});
    DrawRectangleRoundedLinesEx(r, 0.2f, 4, selecionada ? 2.5f : 1.5f, selecionada ? COR_AMBAR : Fade(COR_FOSFORO, 0.6f));
    Vector2 m = ui_mede(e, nome, 16 * s);
    ui_texto(e, nome, (Vector2){centro.x - m.x * 0.5f - 8 * s, centro.y - 8 * s}, 16 * s, selecionada ? COR_AMBAR : COR_FOSFORO);
    DrawCircleV((Vector2){r.x + r.width - 12 * s, centro.y}, 6 * s, led ? COR_FOSFORO : (Color){20, 40, 25, 255});
    return (Vector2){r.x + r.width, centro.y};
}

static void lampada_saida(const Estado *e, Vector2 c, bool acesa) {
    float s = ui_escala();
    if (acesa) DrawCircleV(c, 30 * s, Fade(COR_AMBAR, 0.25f));
    DrawCircleV(c, 18 * s, acesa ? COR_AMBAR : (Color){40, 32, 18, 255});
    DrawCircleLinesV(c, 18 * s, COR_AMBAR);
    ui_texto(e, dialogo_texto(TXT_PAINEL_SAIDA), (Vector2){c.x - 22 * s, c.y + 26 * s}, 14 * s, COR_AMBAR);
}

static void desenha_painel1(const Estado *e, Rectangle r) {
    float s = ui_escala();
    const Painel *p = &e->painel;
    bool leds[MAX_PORTAS];
    bool saida = avalia_mostrado(e, leds);
    const char *nomes[3] = {"A", "B", "C"};
    Vector2 ent[3];
    for (int i = 0; i < 3; i++) {
        ent[i] = (Vector2){r.x + 150 * s, r.y + (110 + i * 90) * s};
        char rotulo[48];
        snprintf(rotulo, sizeof(rotulo), "[%d] %s  %s", i + 1, nomes[i], dialogo_texto(p->chaves[i] ? TXT_PAINEL_CHAVE_LIGADA : TXT_PAINEL_CHAVE_DESLIGADA));
        ui_opcao(e, rotulo, (Vector2){r.x + 26 * s, ent[i].y - 10 * s}, 120 * s + 60 * s, 16 * s, p->cursor == i, p->chaves[i] ? COR_FOSFORO : Fade(COR_FOSFORO, 0.7f));
        ent[i].x += 40 * s;
    }
    Vector2 nao = porta(e, (Vector2){r.x + 330 * s, ent[1].y}, dialogo_porta(PORTA_NAO), leds[0], false);
    fio(ent[1], (Vector2){r.x + 260 * s, ent[1].y}, p->chaves[1]);
    Vector2 e1 = porta(e, (Vector2){r.x + 500 * s, (ent[0].y + ent[1].y) * 0.5f}, dialogo_porta(PORTA_E), leds[1], false);
    fio(ent[0], (Vector2){r.x + 430 * s, e1.y - 10 * s}, p->chaves[0]);
    fio(nao, (Vector2){r.x + 430 * s, e1.y + 10 * s}, leds[0]);
    Vector2 e2 = porta(e, (Vector2){r.x + 660 * s, (e1.y + ent[2].y) * 0.5f}, dialogo_porta(PORTA_E), leds[2], false);
    fio(e1, (Vector2){r.x + 590 * s, e2.y - 10 * s}, leds[1]);
    fio(ent[2], (Vector2){r.x + 590 * s, e2.y + 10 * s}, p->chaves[2]);
    fio(e2, (Vector2){r.x + 770 * s, e2.y}, saida);
    lampada_saida(e, (Vector2){r.x + 790 * s, e2.y}, saida);
}

static void desenha_tabela_painel2(const Estado *e, Vector2 pos) {
    float s = ui_escala();
    Circuito c;
    monta(&e->painel, &c);
    char cabecalho[48];
    snprintf(cabecalho, sizeof(cabecalho), "A B   %s   %s", dialogo_texto(TXT_PAINEL_SAIDA), dialogo_texto(TXT_PAINEL_ALVO));
    ui_texto(e, cabecalho, pos, 16 * s, Fade(COR_FOSFORO, 0.7f));
    for (int l = 0; l < 4; l++) {
        bool ent[2];
        logica_valores_da_linha(l, 2, ent);
        bool saida = regras_avaliar_circuito(&c, ent, NULL);
        bool bate = saida == ALVO_PAINEL2[l];
        float y = pos.y + (28 + l * 28) * s;
        if (l == linha_mostrada(e)) DrawRectangleRounded((Rectangle){pos.x - 8 * s, y - 4 * s, 200 * s, 26 * s}, 0.3f, 4, Fade(COR_FOSFORO, 0.12f));
        char linha[48];
        snprintf(linha, sizeof(linha), "%d %d     %s       %s", ent[0], ent[1], saida ? "1" : "0", ALVO_PAINEL2[l] ? "1" : "0");
        ui_texto(e, linha, (Vector2){pos.x, y}, 18 * s, bate ? COR_FOSFORO : COR_ALARME);
    }
}

static void desenha_painel2(const Estado *e, Rectangle r) {
    float s = ui_escala();
    const Painel *p = &e->painel;
    bool leds[MAX_PORTAS];
    bool saida = avalia_mostrado(e, leds);
    bool ent[2];
    logica_valores_da_linha(linha_mostrada(e), 2, ent);
    Vector2 a = {r.x + 60 * s, r.y + 130 * s}, b = {r.x + 60 * s, r.y + 210 * s};
    ui_texto(e, "A", (Vector2){a.x - 30 * s, a.y - 10 * s}, 20 * s, cor_sinal(ent[0]));
    ui_texto(e, "B", (Vector2){b.x - 30 * s, b.y - 10 * s}, 20 * s, cor_sinal(ent[1]));
    Vector2 p1 = porta(e, (Vector2){r.x + 220 * s, (a.y + b.y) * 0.5f}, dialogo_porta(p->escolha[0]), leds[0], p->cursor % 2 == 0);
    fio(a, (Vector2){r.x + 150 * s, p1.y - 10 * s}, ent[0]);
    fio(b, (Vector2){r.x + 150 * s, p1.y + 10 * s}, ent[1]);
    Vector2 nao = porta(e, (Vector2){r.x + 220 * s, r.y + 300 * s}, dialogo_porta(PORTA_NAO), leds[1], false);
    fio((Vector2){a.x, a.y}, (Vector2){r.x + 150 * s, nao.y}, ent[0]);
    Vector2 p2 = porta(e, (Vector2){r.x + 420 * s, (p1.y + nao.y) * 0.5f}, dialogo_porta(p->escolha[1]), leds[2], p->cursor % 2 == 1);
    fio(p1, (Vector2){r.x + 350 * s, p2.y - 10 * s}, leds[0]);
    fio(nao, (Vector2){r.x + 350 * s, p2.y + 10 * s}, leds[1]);
    fio(p2, (Vector2){r.x + 520 * s, p2.y}, saida);
    lampada_saida(e, (Vector2){r.x + 540 * s, p2.y}, p->resolvido || saida);
    desenha_tabela_painel2(e, (Vector2){r.x + 620 * s, r.y + 110 * s});
}

void circuito_desenha(const Estado *e) {
    if (e->jog.modo != MODO_PAINEL) return;
    float s = ui_escala();
    const Painel *p = &e->painel;
    Rectangle r = {GetScreenWidth() * 0.5f - 430 * s, 64 * s, 860 * s, 430 * s}; // acima da legenda do interfone
    ui_painel(r, COR_AMBAR);
    ui_texto(e, dialogo_texto(p->numero == 1 ? TXT_PAINEL1_TITULO : TXT_PAINEL2_TITULO), (Vector2){r.x + 24 * s, r.y + 18 * s}, 24 * s, COR_AMBAR);
    if (p->numero == 1) desenha_painel1(e, r);
    else desenha_painel2(e, r);
    TextoId rodape = p->resolvido ? TXT_PAINEL_RESOLVIDO : p->numero == 1 ? TXT_PAINEL_INSTRUCOES1 : TXT_PAINEL_INSTRUCOES2;
    ui_texto(e, dialogo_texto(rodape), (Vector2){r.x + 24 * s, r.y + r.height - 40 * s}, 18 * s, p->resolvido ? COR_AMBAR : Fade(COR_FOSFORO, 0.8f));
}

// ================================================================== desenho 3D: LEDs no gabinete
void circuito_desenha_3d(const Estado *e) {
    if (!regras_fase_eh_saida(e->fase)) return;
    const Painel *p = &e->painel;
    bool leds[MAX_PORTAS];
    bool saida = avalia_mostrado(e, leds) || p->resolvido;
    bool painel1 = p->numero == 1;
    Vector3 c = painel1 ? e->nivel.painel1 : e->nivel.painel2;
    Vector3 frente = painel1 ? (Vector3){0, 0, 0.27f} : (Vector3){0.27f, 0, 0};
    Vector3 lado = painel1 ? (Vector3){0.22f, 0, 0} : (Vector3){0, 0, -0.22f};
    for (int i = 0; i < 3; i++) {
        Vector3 led = Vector3Add(Vector3Add((Vector3){c.x, 1.35f, c.z}, frente), Vector3Scale(lado, (float)(i - 1)));
        DrawSphereEx(led, 0.035f, 6, 6, leds[i] ? COR_FOSFORO : (Color){20, 40, 25, 255});
    }
    Vector3 lamp = Vector3Add((Vector3){c.x, 1.62f, c.z}, frente);
    DrawSphereEx(lamp, 0.07f, 8, 8, saida ? COR_AMBAR : (Color){40, 32, 18, 255});
}

// ================================================================== piloto automático (--smoke)
// Acha a solução avaliando o circuito (força bruta), como um jogador que testa combinações.
static void piloto_painel1(const Painel *p, Entrada *in) {
    Circuito c;
    regras_circuito_painel1(&c);
    for (int linha = 0; linha < 8; linha++) {
        bool alvo[3];
        logica_valores_da_linha(linha, 3, alvo);
        if (!regras_avaliar_circuito(&c, alvo, NULL)) continue;
        if (p->chaves[0] != alvo[0]) in->tecla1 = true;
        else if (p->chaves[1] != alvo[1]) in->tecla2 = true;
        else if (p->chaves[2] != alvo[2]) in->tecla3 = true;
        return;
    }
}

static void piloto_painel2(const Painel *p, Entrada *in) {
    for (int p1 = 0; p1 < 4; p1++) {
        for (int p2 = 0; p2 < 4; p2++) {
            Circuito c;
            regras_circuito_painel2(&c, (TipoPorta)p1, (TipoPorta)p2);
            if (!regras_circuito_confere_tabela(&c, ALVO_PAINEL2)) continue;
            const int alvo[2] = {p1, p2};
            int encaixe = p->escolha[0] != alvo[0] ? 0 : 1;
            if (p->cursor % 2 != encaixe) in->direita = true;
            else in->baixo = true;
            return;
        }
    }
}

void circuito_piloto(const Estado *e, Entrada *in) {
    if (e->painel.resolvido) return;
    if (e->painel.numero == 1) piloto_painel1(&e->painel, in);
    else piloto_painel2(&e->painel, in);
}
