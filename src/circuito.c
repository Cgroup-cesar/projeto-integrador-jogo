// circuito.c — painéis das saídas. Painel 1 (gerador): chaves A, B, C e SAÍDA = (A ∧ ¬B) ∧ C.
// Painel 2 (Arquivo): SAÍDA = (A [?1] B) [?2] (¬A), com alvo 00→1, 01→1, 10→0, 11→1.
// A checagem sempre avalia o circuito (regras_avaliar_circuito); nunca compara com uma resposta fixa.
#include "circuito.h"
#include "nivel.h"
#include "criatura.h"
#include "audio_sintese.h"
#include "ui.h"
#include "manual.h"
#include "dialogo.h"
#include "crt.h"
#include "clique.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const bool ALVO_PAINEL2[4] = {true, true, false, true};

// ================================================================== circuito do painel
static void monta(const Painel *p, Circuito *c) {
    if (p->numero == 1)
        regras_circuito_painel1(c);
    else
        regras_circuito_painel2(c, (TipoPorta)p->escolha[0], (TipoPorta)p->escolha[1]);
}

static bool esta_resolvido(const Painel *p) {
    Circuito c;
    monta(p, &c);
    if (p->numero == 1) return regras_avaliar_circuito(&c, p->chaves, NULL);
    return regras_circuito_confere_tabela(&c, ALVO_PAINEL2);
}

// No painel 2 as entradas não são chaves. Enquanto alguma linha não bate com o ALVO, o painel para na
// primeira delas (uma pergunta por vez); resolvido, os LEDs voltam a percorrer as 4 linhas.
static int linha_mostrada(const Estado *e) {
    const Painel *p = &e->painel;
    if (p->numero == 2 && !p->resolvido) {
        Circuito c;
        monta(p, &c);
        for (int l = 0; l < 4; l++) { // padrão FILTER: a primeira linha em que a SAÍDA difere do ALVO
            bool ent[2];
            logica_valores_da_linha(l, 2, ent);
            if (regras_avaliar_circuito(&c, ent, NULL) != ALVO_PAINEL2[l]) return l;
        }
    }
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
    bool clicou = !e->ajuda_aberta && clique_escolhe(&e->in, CLIQUE_CHAVE, &p->cursor); // clicar liga/desliga
    if (e->in.confirmar || clicou) teclas[p->cursor] = true;
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
    // O mouse: passar por cima escolhe o encaixe; clicar nele troca a porta (como a seta para baixo).
    bool clicou = !e->ajuda_aberta && clique_escolhe(&e->in, CLIQUE_ENCAIXE, &p->cursor);
    int passo = (e->in.cima ? -1 : 0) + (e->in.baixo || e->in.confirmar || clicou ? 1 : 0);
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
    if (e->painel.resolvido || e->apagao_t > 0.0f || e->tempo_fase < 4.0f)
        return; // nada de apagão logo de cara
    if (col_sorteia_por_segundo(0.04f, dt)) e->apagao_t = 0.35f + GetRandomValue(0, 30) / 100.0f;
}

void circuito_atualiza(Estado *e, float dt) {
    Painel *p = &e->painel;
    apagoes(e, dt);
    p->barulho = fmaxf(0.0f, p->barulho - 0.12f * dt); // o barulho some devagar
    if (p->resolvido) {
        p->resolvido_t += dt;
        if (p->resolvido_t > 1.8f && e->jog.modo == MODO_PAINEL) e->jog.modo = MODO_ANDANDO;
        return; // a volta à sala (retorno.c) fecha a porta e conclui a saída
    }
    if (e->jog.modo != MODO_PAINEL) return;
    if (e->in.voltar) {
        e->jog.modo = MODO_ANDANDO;
        return;
    }
    if (e->in.ajuda) e->ajuda_aberta = !e->ajuda_aberta; // H: os cartões das portas, como no terminal
    Painel antes = *p;
    bool mexeu = p->numero == 1 ? atualiza_painel1(e) : atualiza_painel2(e);
    if (!mexeu) return;
    bool trocou = memcmp(antes.chaves, p->chaves, sizeof(p->chaves)) != 0 ||
                  memcmp(antes.escolha, p->escolha, sizeof(p->escolha)) != 0;
    if (trocou) p->movimentos++;                  // mover o cursor não conta como movimento...
    p->barulho = fminf(1.0f, p->barulho + 0.25f); // ...mas todo clique faz barulho
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

// Pulsos de luz andando pelo fio ligado, de `p[0]` para `p[3]`: dá para ver por onde a corrente passa.
static void pulsos(const Estado *e, const Vector2 *p) {
    float s = ui_escala(), passo = 24 * s, total = 0.0f, trecho[3];
    for (int i = 0; i < 3; i++) // padrão FOLD: o comprimento do fio é a soma dos trechos
        total += trecho[i] = Vector2Distance(p[i], p[i + 1]);
    for (float d = fmodf(e->tempo_total * 60 * s, passo); d < total; d += passo) { // padrão MAP: cada pulso
        float resto = d;
        int i = 0;
        while (i < 2 && resto > trecho[i])
            resto -= trecho[i++];
        Vector2 c = trecho[i] > 0.0f ? Vector2Lerp(p[i], p[i + 1], resto / trecho[i]) : p[i];
        DrawCircleV(c, 2.5f * s, (Color){210, 255, 220, 255});
    }
}

// Um fio em três trechos (anda, sobe ou desce, anda). Ligado: grosso, com brilho e pulsos; desligado: fino
// e escuro. `entra_em_porta`: na ponta, a lâmpada de entrada — a mesma da cola do terminal.
static void fio(const Estado *e, Vector2 a, Vector2 b, bool v, bool entra_em_porta) {
    float s = ui_escala(), largura = (v ? 3.0f : 2.0f) * s;
    float meio = (a.x + b.x) * 0.5f;
    Vector2 p[4] = {a, {meio, a.y}, {meio, b.y}, b};
    for (int i = 0; i < 3; i++) { // padrão MAP: cada trecho do fio
        if (v) DrawLineEx(p[i], p[i + 1], 9 * s, Fade(COR_FOSFORO, 0.14f));
        DrawLineEx(p[i], p[i + 1], largura, cor_sinal(v));
        if (i > 0) DrawCircleV(p[i], largura * 0.5f, cor_sinal(v)); // a quina sem falha
    }
    if (v) pulsos(e, p);
    if (entra_em_porta) manual_lampada(b, 5 * s, v);
}

// Uma porta lógica: caixa com o nome e o LED da saída ao vivo. Devolve o ponto de saída.
static Rectangle caixa_da_porta(Vector2 centro) {
    float s = ui_escala();
    return (Rectangle){centro.x - 70 * s, centro.y - 22 * s, 140 * s, 44 * s};
}

static Vector2 porta(const Estado *e, Vector2 centro, const char *nome, bool led, bool selecionada) {
    float s = ui_escala();
    Rectangle r = caixa_da_porta(centro);
    DrawRectangleRounded(r, 0.2f, 4, selecionada ? Fade(COR_AMBAR, 0.25f) : (Color){12, 20, 15, 255});
    DrawRectangleRoundedLinesEx(r, 0.2f, 4, selecionada ? 2.5f : 1.5f,
                                selecionada ? COR_AMBAR : Fade(COR_FOSFORO, 0.6f));
    Vector2 m = ui_mede(e, nome, 16 * s);
    ui_texto(e, nome, (Vector2){centro.x - m.x * 0.5f - 8 * s, centro.y - 8 * s}, 16 * s,
             selecionada ? COR_AMBAR : COR_FOSFORO);
    DrawCircleV((Vector2){r.x + r.width - 12 * s, centro.y}, 6 * s,
                led ? COR_FOSFORO : (Color){20, 40, 25, 255});
    return (Vector2){r.x + r.width, centro.y};
}

static void lampada_saida(const Estado *e, Vector2 c, bool acesa) {
    float s = ui_escala();
    if (acesa) DrawCircleV(c, 30 * s, Fade(COR_AMBAR, 0.25f));
    DrawCircleV(c, 18 * s, acesa ? COR_AMBAR : (Color){40, 32, 18, 255});
    DrawCircleLinesV(c, 18 * s, COR_AMBAR);
    ui_texto(e, dialogo_texto(TXT_PAINEL_SAIDA), (Vector2){c.x - 22 * s, c.y + 26 * s}, 14 * s, COR_AMBAR);
}

// ------------------------------------------------------------------ a pergunta da vez
// Um anel âmbar pulsando no ponto que trava a corrente: a pessoa olha um lugar só, não o painel inteiro.
static void aponta(const Estado *e, Vector2 ponto) {
    float s = ui_escala(), pulso = 0.5f + 0.5f * sinf(e->tempo_total * 6.0f);
    DrawRing(ponto, (9 + 4 * pulso) * s, (12 + 4 * pulso) * s, 0, 360, 32,
             Fade(COR_AMBAR, 0.5f + 0.5f * pulso));
}

// Painel 1: da SAÍDA para trás até a entrada que trava (regras_circuito_bloqueio), com a frase da porta.
static void pergunta_painel1(const Estado *e, Rectangle r, const Vector2 pontos[3][2]) {
    const Painel *p = &e->painel;
    if (p->resolvido) return;
    Circuito c;
    monta(p, &c);
    Bloqueio b = regras_circuito_bloqueio(&c, p->chaves);
    if (b.porta < 0) return;
    aponta(e, pontos[b.porta][b.entrada]);
    TipoPorta tipo = c.portas[b.porta].tipo;
    TextoId dica = tipo == PORTA_NAO  ? TXT_PAINEL_DICA_NAO
                   : tipo == PORTA_OU ? TXT_PAINEL_DICA_OU
                                      : TXT_PAINEL_DICA_E;
    float s = ui_escala();
    ui_texto_quebrado(e, dialogo_texto(dica), (Vector2){r.x + 24 * s, r.y + 286 * s}, 380 * s, 16 * s,
                      COR_AMBAR);
}

// O preço do chute: o medidor de barulho (sobe a cada clique, desce devagar). Os movimentos não ficam
// contando na tela durante a tarefa: o bônus por poucos movimentos só aparece quando o painel é resolvido.
static void medidores(const Estado *e, Rectangle r) {
    const Painel *p = &e->painel;
    float s = ui_escala(), x = r.x + 24 * s, y = r.y + 54 * s;
    ui_texto(e, dialogo_texto(TXT_PAINEL_BARULHO), (Vector2){x, y}, 14 * s, Fade(COR_FOSFORO, 0.7f));
    x += ui_mede(e, dialogo_texto(TXT_PAINEL_BARULHO), 14 * s).x + 10 * s;
    Rectangle barra = {x, y + 2 * s, 120 * s, 11 * s};
    bool alto = p->barulho > 0.7f;
    Color cor = alto ? COR_ALARME : p->barulho > 0.4f ? COR_AMBAR : COR_FOSFORO;
    if (alto && fmodf(e->tempo_total, 0.5f) < 0.25f) cor = Fade(cor, 0.4f); // pisca: a criatura pode ouvir
    DrawRectangleRec(barra, (Color){20, 28, 22, 255});
    DrawRectangleRec((Rectangle){barra.x, barra.y, barra.width * p->barulho, barra.height}, cor);
    DrawRectangleLinesEx(barra, 1.0f * s, Fade(COR_FOSFORO, 0.5f));
    int bonus = regras_bonus_movimentos(p->movimentos);
    if (!p->resolvido || bonus <= 0) return;
    char texto[64];
    snprintf(texto, sizeof(texto), dialogo_texto(TXT_PAINEL_BONUS), p->movimentos, bonus);
    ui_texto(e, texto, (Vector2){barra.x + barra.width + 24 * s, y}, 14 * s, COR_AMBAR);
}

static void desenha_painel1(const Estado *e, Rectangle r) {
    float s = ui_escala();
    const Painel *p = &e->painel;
    bool leds[MAX_PORTAS];
    bool saida = avalia_mostrado(e, leds);
    const char *nomes[3] = {"A", "B", "C"};
    Vector2 ent[3];
    for (int i = 0; i < 3; i++) {
        ent[i] = (Vector2){r.x + 150 * s, r.y + (100 + i * 75) * s};
        char rotulo[48];
        snprintf(rotulo, sizeof(rotulo), dialogo_texto(TXT_PAINEL_CHAVE), i + 1, nomes[i],
                 dialogo_texto(p->chaves[i] ? TXT_PAINEL_CHAVE_LIGADA : TXT_PAINEL_CHAVE_DESLIGADA));
        ui_opcao(e, rotulo, (Vector2){r.x + 26 * s, ent[i].y - 10 * s}, 120 * s + 60 * s, 16 * s,
                 p->cursor == i, p->chaves[i] ? COR_FOSFORO : Fade(COR_FOSFORO, 0.7f));
        clique_area(CLIQUE_CHAVE, i, (Rectangle){r.x + 16 * s, ent[i].y - 14 * s, 180 * s, 24 * s});
        ent[i].x += 40 * s;
    }
    Vector2 nao = porta(e, (Vector2){r.x + 330 * s, ent[1].y}, dialogo_porta(PORTA_NAO), leds[0], false);
    fio(e, ent[1], (Vector2){r.x + 260 * s, ent[1].y}, p->chaves[1], true);
    Vector2 e1 = porta(e, (Vector2){r.x + 500 * s, (ent[0].y + ent[1].y) * 0.5f}, dialogo_porta(PORTA_E),
                       leds[1], false);
    fio(e, ent[0], (Vector2){r.x + 430 * s, e1.y - 10 * s}, p->chaves[0], true);
    fio(e, nao, (Vector2){r.x + 430 * s, e1.y + 10 * s}, leds[0], true);
    Vector2 e2 =
        porta(e, (Vector2){r.x + 660 * s, (e1.y + ent[2].y) * 0.5f}, dialogo_porta(PORTA_E), leds[2], false);
    fio(e, e1, (Vector2){r.x + 590 * s, e2.y - 10 * s}, leds[1], true);
    fio(e, ent[2], (Vector2){r.x + 590 * s, e2.y + 10 * s}, p->chaves[2], true);
    fio(e, e2, (Vector2){r.x + 770 * s, e2.y}, saida, false);
    lampada_saida(e, (Vector2){r.x + 790 * s, e2.y}, saida);
    const Vector2 pontos[3][2] = {// a entrada de cada porta (0 = NÃO, 1 = 1º E, 2 = último E), na tela
                                  {{r.x + 260 * s, ent[1].y}, {r.x + 260 * s, ent[1].y}},
                                  {{r.x + 430 * s, e1.y - 10 * s}, {r.x + 430 * s, e1.y + 10 * s}},
                                  {{r.x + 590 * s, e2.y - 10 * s}, {r.x + 590 * s, e2.y + 10 * s}}};
    pergunta_painel1(e, r, pontos);
}

static void desenha_tabela_painel2(const Estado *e, Vector2 pos) {
    float s = ui_escala();
    Circuito c;
    monta(&e->painel, &c);
    char cabecalho[48];
    snprintf(cabecalho, sizeof(cabecalho), "A B   %s   %s", dialogo_texto(TXT_PAINEL_SAIDA),
             dialogo_texto(TXT_PAINEL_ALVO));
    ui_texto(e, cabecalho, pos, 16 * s, Fade(COR_FOSFORO, 0.7f));
    for (int l = 0; l < 4; l++) {
        bool ent[2];
        logica_valores_da_linha(l, 2, ent);
        bool saida = regras_avaliar_circuito(&c, ent, NULL);
        bool bate = saida == ALVO_PAINEL2[l];
        float y = pos.y + (28 + l * 28) * s;
        Rectangle faixa = {pos.x - 8 * s, y - 4 * s, 200 * s, 26 * s};
        if (l == linha_mostrada(e) && !bate) { // a linha da vez: parada, em âmbar, com a pergunta embaixo
            DrawRectangleRounded(faixa, 0.3f, 4, Fade(COR_AMBAR, 0.15f));
            DrawRectangleRoundedLinesEx(faixa, 0.3f, 4, 2.0f * s, COR_AMBAR);
            char dica[128];
            snprintf(dica, sizeof(dica), dialogo_texto(TXT_PAINEL2_DICA), saida, ALVO_PAINEL2[l]);
            ui_texto_quebrado(e, dica, (Vector2){pos.x - 8 * s, pos.y + 146 * s}, 216 * s, 13 * s, COR_AMBAR);
        } else if (l == linha_mostrada(e)) {
            DrawRectangleRounded(faixa, 0.3f, 4, Fade(COR_FOSFORO, 0.12f));
        }
        char linha[48];
        snprintf(linha, sizeof(linha), "%d %d     %s       %s", ent[0], ent[1], saida ? "1" : "0",
                 ALVO_PAINEL2[l] ? "1" : "0");
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
    Vector2 c1 = {r.x + 220 * s, (a.y + b.y) * 0.5f};
    Vector2 p1 = porta(e, c1, dialogo_porta(p->escolha[0]), leds[0], p->cursor % 2 == 0);
    clique_area(CLIQUE_ENCAIXE, 0, caixa_da_porta(c1));
    fio(e, a, (Vector2){r.x + 150 * s, p1.y - 10 * s}, ent[0], true);
    fio(e, b, (Vector2){r.x + 150 * s, p1.y + 10 * s}, ent[1], true);
    Vector2 nao = porta(e, (Vector2){r.x + 220 * s, r.y + 300 * s}, dialogo_porta(PORTA_NAO), leds[1], false);
    fio(e, a, (Vector2){r.x + 150 * s, nao.y}, ent[0], true);
    Vector2 c2 = {r.x + 420 * s, (p1.y + nao.y) * 0.5f};
    Vector2 p2 = porta(e, c2, dialogo_porta(p->escolha[1]), leds[2], p->cursor % 2 == 1);
    clique_area(CLIQUE_ENCAIXE, 1, caixa_da_porta(c2));
    fio(e, p1, (Vector2){r.x + 350 * s, p2.y - 10 * s}, leds[0], true);
    fio(e, nao, (Vector2){r.x + 350 * s, p2.y + 10 * s}, leds[1], true);
    fio(e, p2, (Vector2){r.x + 520 * s, p2.y}, saida, false);
    lampada_saida(e, (Vector2){r.x + 540 * s, p2.y}, p->resolvido || saida);
    desenha_tabela_painel2(e, (Vector2){r.x + 620 * s, r.y + 70 * s});
}

// ------------------------------------------------------------------ a ajuda (H): os cartões das portas
// A tabela de uma porta sozinha, calculada pelo próprio circuito (um circuito de uma porta só): o cartão
// mostra exatamente o que o painel faz.
static void tabela_da_porta(TipoPorta tipo, int *valores) {
    Circuito c = {.n_entradas = 2, .n_portas = 1};
    c.portas[0] = (Porta){tipo, 0, 1};
    for (int l = 0; l < 4; l++) { // padrão MAP: a saída da porta em cada linha (A, B)
        bool ent[2] = {l & 1, false};
        if (tipo != PORTA_NAO) logica_valores_da_linha(l, 2, ent);
        valores[l] = regras_avaliar_circuito(&c, ent, NULL);
    }
}

static void cartao_da_porta(const Estado *e, TipoPorta tipo, Rectangle b, bool destaque) {
    static const TextoId descricao[PORTA_TOTAL] = {TXT_COLA_E, TXT_COLA_OU, TXT_COLA_XOU, TXT_COLA_NE,
                                                   TXT_COLA_NAO};
    int valores[4];
    tabela_da_porta(tipo, valores);
    manual_cartao(e, b, dialogo_porta(tipo), dialogo_texto(descricao[tipo]), tipo == PORTA_NAO, valores,
                  destaque, ui_escala());
}

// Painel 1: as duas portas que ele tem (NÃO e E). Painel 2: a porta do encaixe escolhido, que muda junto
// quando a pessoa troca — ela aprende a porta trocando a porta.
static void desenha_ajuda(const Estado *e, Rectangle r) {
    if (!e->ajuda_aberta || e->painel.resolvido) return;
    float s = ui_escala();
    if (e->painel.numero == 1) {
        cartao_da_porta(e, PORTA_NAO, (Rectangle){r.x + 420 * s, r.y + 262 * s, 203 * s, 130 * s}, false);
        cartao_da_porta(e, PORTA_E, (Rectangle){r.x + 633 * s, r.y + 262 * s, 203 * s, 130 * s}, false);
    } else {
        TipoPorta escolhida = (TipoPorta)e->painel.escolha[e->painel.cursor % 2];
        cartao_da_porta(e, escolhida, (Rectangle){r.x + 620 * s, r.y + 272 * s, 216 * s, 122 * s}, true);
    }
}

void circuito_desenha(const Estado *e) {
    if (e->jog.modo != MODO_PAINEL) return;
    float s = ui_escala();
    const Painel *p = &e->painel;
    Rectangle r = {GetScreenWidth() * 0.5f - 430 * s, 56 * s, 860 * s,
                   404 * s};           // acima da legenda do rádio e da barra de teclas
    crt_painel(e, r, COR_AMBAR, true); // o mesmo monitor do analisador, com a borda âmbar do painel
    crt_titulo(e, dialogo_texto(p->numero == 1 ? TXT_PAINEL1_TITULO : TXT_PAINEL2_TITULO),
               (Vector2){r.x + 24 * s, r.y + 18 * s}, 24 * s, COR_AMBAR);
    if (p->numero == 1)
        desenha_painel1(e, r);
    else
        desenha_painel2(e, r);
    TextoId rodape = p->resolvido     ? TXT_PAINEL_RESOLVIDO // as teclas ficam na barra do rodapé (teclas.c)
                     : p->numero == 1 ? TXT_PAINEL_OBJETIVO1
                                      : TXT_PAINEL_OBJETIVO2;
    float largura = (p->numero == 1 ? 380 : 570) * s; // à esquerda dos cartões da ajuda
    ui_texto_quebrado(e, dialogo_texto(rodape), (Vector2){r.x + 24 * s, r.y + r.height - 58 * s}, largura,
                      18 * s, p->resolvido ? COR_AMBAR : Fade(COR_FOSFORO, 0.8f));
    if (!p->resolvido) manual_selo_ajuda(e, r.x + r.width - 24 * s, r.y + 18 * s, s);
    medidores(e, r);
    desenha_ajuda(e, r);
}

// ================================================================== desenho 3D: LEDs no gabinete
void circuito_desenha_3d(const Estado *e) {
    if (!regras_fase_eh_saida(e->fase)) return;
    const Painel *p = &e->painel;
    bool leds[MAX_PORTAS];
    bool saida = avalia_mostrado(e, leds) || p->resolvido;
    bool painel1 = p->numero == 1;
    Vector3 c = painel1 ? e->nivel.painel1 : e->nivel.painel2;
    Vector3 frente = {0, 0, 0.27f}; // os dois painéis encostam numa parede norte e olham para o sul
    Vector3 lado = {0.22f, 0, 0};
    for (int i = 0; i < 3; i++) {
        Vector3 led =
            Vector3Add(Vector3Add((Vector3){c.x, 1.35f, c.z}, frente), Vector3Scale(lado, (float)(i - 1)));
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
        if (p->chaves[0] != alvo[0])
            in->tecla1 = true;
        else if (p->chaves[1] != alvo[1])
            in->tecla2 = true;
        else if (p->chaves[2] != alvo[2])
            in->tecla3 = true;
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
            if (p->cursor % 2 != encaixe)
                in->direita = true;
            else
                in->baixo = true;
            return;
        }
    }
}

void circuito_piloto(const Estado *e, Entrada *in) {
    if (e->painel.resolvido) return;
    if (e->painel.numero == 1)
        piloto_painel1(&e->painel, in);
    else
        piloto_painel2(&e->painel, in);
}
