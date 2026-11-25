// circuito.c — painéis das saídas, os dois com as chaves A, B e C. Painel 1 (gerador): SAÍDA = (A ∧ ¬B) ∧ C.
// Painel 2 (Arquivo, o segundo gerador): SAÍDA = (¬A ∨ B) ∧ C.
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

// ================================================================== circuito do painel
static void monta(const Painel *p, Circuito *c) {
    if (p->numero == 1)
        regras_circuito_painel1(c);
    else
        regras_circuito_painel2(c);
}

// A SAÍDA com as chaves de agora; `leds` (pode ser NULL) recebe a saída de cada porta.
static bool avalia(const Painel *p, bool *leds) {
    Circuito c;
    monta(p, &c);
    return regras_avaliar_circuito(&c, p->chaves, leds);
}

// ================================================================== início
void circuito_inicia(Estado *e, int numero) {
    memset(&e->painel, 0, sizeof(e->painel));
    e->painel.numero = numero;
    // O painel 2 abre com a chave A ligada: A acesa e B apagada é a única linha em que SE A, ENTÃO B é
    // falso. Com tudo desligado bastaria ligar C; assim o mínimo são 2 trocas, como no painel 1.
    if (numero == 2) e->painel.chaves[0] = true;
}

// ================================================================== atualiza
static bool atualiza_chaves(Estado *e) {
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
    if (!atualiza_chaves(e)) return;
    bool trocou = memcmp(antes.chaves, p->chaves, sizeof(p->chaves)) != 0;
    if (trocou) p->movimentos++;                  // mover o cursor não conta como movimento...
    p->barulho = fminf(1.0f, p->barulho + 0.25f); // ...mas todo clique faz barulho
    // Mexer no painel faz barulho: a criatura pode ouvir.
    Vector3 onde = p->numero == 1 ? e->nivel.painel1 : e->nivel.painel2;
    col_ruidos_adiciona(&e->ruidos, (Ruido){onde, RUIDO_RAIO_PAINEL, 1.0f, RUIDO_PAINEL});
    audio_toca(e, SOM_INTERRUPTOR, 0.9f, 0.8f + GetRandomValue(0, 30) / 100.0f);
    if (avalia(p, NULL)) resolve(e);
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

// Da SAÍDA para trás até a entrada que trava (regras_circuito_bloqueio), com a frase da porta.
static void pergunta(const Estado *e, Rectangle r, const Vector2 pontos[3][2]) {
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

// As chaves A, B e C, uma por linha, com a tecla e o estado. Devolve, em `ent`, a ponta de cada fio.
static void desenha_chaves(const Estado *e, Rectangle r, Vector2 ent[3]) {
    float s = ui_escala();
    const Painel *p = &e->painel;
    const char *nomes[3] = {"A", "B", "C"};
    for (int i = 0; i < 3; i++) { // padrão MAP: cada chave vira uma linha do painel
        ent[i] = (Vector2){r.x + 150 * s, r.y + (100 + i * 75) * s};
        char rotulo[48];
        snprintf(rotulo, sizeof(rotulo), dialogo_texto(TXT_PAINEL_CHAVE), i + 1, nomes[i],
                 dialogo_texto(p->chaves[i] ? TXT_PAINEL_CHAVE_LIGADA : TXT_PAINEL_CHAVE_DESLIGADA));
        ui_opcao(e, rotulo, (Vector2){r.x + 26 * s, ent[i].y - 10 * s}, 120 * s + 60 * s, 16 * s,
                 p->cursor == i, p->chaves[i] ? COR_FOSFORO : Fade(COR_FOSFORO, 0.7f));
        clique_area(CLIQUE_CHAVE, i, (Rectangle){r.x + 16 * s, ent[i].y - 14 * s, 180 * s, 24 * s});
        ent[i].x += 40 * s;
    }
}

// Os dois painéis têm o mesmo formato: um NÃO numa das chaves de cima (B no painel 1, A no painel 2), a porta
// do meio juntando a linha A com a linha B, e um E final com a chave C. As portas vêm do circuito.
static void desenha_painel(const Estado *e, Rectangle r) {
    float s = ui_escala();
    const Painel *p = &e->painel;
    Circuito c;
    monta(p, &c);
    bool leds[MAX_PORTAS];
    bool saida = regras_avaliar_circuito(&c, p->chaves, leds);
    Vector2 ent[3];
    desenha_chaves(e, r, ent);
    int neg = c.portas[0].a; // a chave que passa pelo NÃO (0 = A, 1 = B)
    Vector2 nao =
        porta(e, (Vector2){r.x + 330 * s, ent[neg].y}, dialogo_porta(c.portas[0].tipo), leds[0], false);
    fio(e, ent[neg], (Vector2){r.x + 260 * s, ent[neg].y}, p->chaves[neg], true);
    Vector2 m = porta(e, (Vector2){r.x + 500 * s, (ent[0].y + ent[1].y) * 0.5f},
                      dialogo_porta(c.portas[1].tipo), leds[1], false);
    fio(e, neg == 0 ? nao : ent[0], (Vector2){r.x + 430 * s, m.y - 10 * s}, neg == 0 ? leds[0] : p->chaves[0],
        true);
    fio(e, neg == 1 ? nao : ent[1], (Vector2){r.x + 430 * s, m.y + 10 * s}, neg == 1 ? leds[0] : p->chaves[1],
        true);
    Vector2 fim = porta(e, (Vector2){r.x + 660 * s, (m.y + ent[2].y) * 0.5f}, dialogo_porta(c.portas[2].tipo),
                        leds[2], false);
    fio(e, m, (Vector2){r.x + 590 * s, fim.y - 10 * s}, leds[1], true);
    fio(e, ent[2], (Vector2){r.x + 590 * s, fim.y + 10 * s}, p->chaves[2], true);
    fio(e, fim, (Vector2){r.x + 770 * s, fim.y}, saida, false);
    lampada_saida(e, (Vector2){r.x + 790 * s, fim.y}, saida);
    const Vector2 pontos[3][2] = {// a entrada de cada porta (0 = NÃO, 1 = a do meio, 2 = o E final), na tela
                                  {{r.x + 260 * s, ent[neg].y}, {r.x + 260 * s, ent[neg].y}},
                                  {{r.x + 430 * s, m.y - 10 * s}, {r.x + 430 * s, m.y + 10 * s}},
                                  {{r.x + 590 * s, fim.y - 10 * s}, {r.x + 590 * s, fim.y + 10 * s}}};
    pergunta(e, r, pontos);
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

// O NÃO e a porta do meio do painel (painel 1: NÃO e E; painel 2: NÃO e OU). O E final é o mesmo nos dois
// e a frase da dica já diz o que ele pede, então não cabe um terceiro cartão.
static void desenha_ajuda(const Estado *e, Rectangle r) {
    if (!e->ajuda_aberta || e->painel.resolvido) return;
    float s = ui_escala();
    Circuito c;
    monta(&e->painel, &c);
    cartao_da_porta(e, c.portas[0].tipo, (Rectangle){r.x + 420 * s, r.y + 262 * s, 203 * s, 130 * s}, false);
    cartao_da_porta(e, c.portas[1].tipo, (Rectangle){r.x + 633 * s, r.y + 262 * s, 203 * s, 130 * s}, false);
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
    desenha_painel(e, r);
    // as teclas ficam na barra do rodapé (teclas.c); o texto fica à esquerda dos cartões da ajuda
    TextoId rodape = p->resolvido ? TXT_PAINEL_RESOLVIDO : TXT_PAINEL_OBJETIVO;
    ui_texto_quebrado(e, dialogo_texto(rodape), (Vector2){r.x + 24 * s, r.y + r.height - 58 * s}, 380 * s,
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
    bool saida = avalia(p, leds) || p->resolvido;
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
// Acha uma solução avaliando o circuito do painel (força bruta), como um jogador que testa combinações.
void circuito_piloto(const Estado *e, Entrada *in) {
    const Painel *p = &e->painel;
    if (p->resolvido) return;
    Circuito c;
    monta(p, &c);
    for (int linha = 0; linha < 8; linha++) { // padrão FILTER: a primeira combinação que acende a SAÍDA
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
