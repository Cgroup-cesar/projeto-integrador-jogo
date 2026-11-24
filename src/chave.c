// chave.c — a porta trancada do Arquivo (a sala do segundo gerador) e a chave dela, no chaveiro da sala de
// observação, ao lado do quadro. Quem decide é a regra pura: regras_tenta_porta_arquivo (só abre na S2 e com
// a chave) e regras_chave_visivel (a chave só existe na S2, até ser pega). Aberta, a porta desliza para
// dentro da parede, sai das paredes de colisão e deixa de bloquear o caminho da criatura.
#include "chave.h"
#include "audio_sintese.h"
#include "cenario.h"
#include "dialogo.h"
#include "nivel.h"
#include "texturas.h"
#include "ui.h"
#include <math.h>

#define PORTA_TAMANHO   (Vector3){0.12f, 2.2f, 1.9f}
#define PORTA_DESLIZA_M 1.45f

static void avisa(Estado *e, TextoId texto, float segundos) {
    e->chave.aviso = texto;
    e->chave.aviso_t = e->chave.aviso_dur = segundos;
}

static int celula_da_porta(const Nivel *n) {
    int c, l;
    return nivel_celula_de(n->porta_arquivo, &c, &l) ? l * MAPA_LARGURA + c : -1;
}

// [REQ:FILTER-interativos-usados] tira da coleção o interativo que já foi usado (a chave pega, a porta
// aberta)
static void tira_interativo(ColInterativos *c, TipoInterativo tipo) {
    int j = 0;
    // padrão FILTER
    for (int i = 0; i < c->n; i++)
        if (c->itens[i].tipo != tipo) c->itens[j++] = c->itens[i];
    c->n = j;
}

// [REQ:FILTER-parede-porta] destrancada, a porta sai das paredes de colisão (o jogador e a criatura passam)
static void tira_parede_da_porta(ColParedes *c) {
    int j = 0;
    // padrão FILTER
    for (int i = 0; i < c->n; i++)
        if (c->itens[i].tipo != PAREDE_PORTA_ARQUIVO) c->itens[j++] = c->itens[i];
    c->n = j;
}

void chave_entra_fase(Estado *e, Fase fase, bool reinicio) {
    ChaveArquivo *k = &e->chave;
    bool zera = fase == FASE_INTRO || fase == FASE_S1 || (fase == FASE_S2 && !reinicio);
    if (zera) *k = (ChaveArquivo){0};
    k->aviso_t = 0.0f;
    if (!regras_chave_visivel(fase, k->pega)) tira_interativo(&e->interativos, INTER_CHAVEIRO);
    if (k->porta_aberta) {
        tira_interativo(&e->interativos, INTER_PORTA_ARQUIVO);
        e->nivel.celula_trancada = -1;
        return;
    }
    Vector3 p = e->nivel.porta_arquivo, meio = Vector3Scale(PORTA_TAMANHO, 0.5f);
    Vector3 centro = {p.x, meio.y, p.z};
    col_paredes_adiciona(
        &e->paredes, (Parede){Vector3Subtract(centro, meio), Vector3Add(centro, meio), PAREDE_PORTA_ARQUIVO});
    e->nivel.celula_trancada = celula_da_porta(&e->nivel);
    if (fase == FASE_S2 && !k->pega) avisa(e, TXT_AVISO_CHAVE, CHAVE_AVISO_S);
}

void chave_atualiza(Estado *e, float dt) {
    ChaveArquivo *k = &e->chave;
    if (k->porta_aberta) k->porta_t += dt;
    k->aviso_t = fmaxf(0.0f, k->aviso_t - dt);
}

void chave_pega(Estado *e) {
    if (!regras_chave_visivel(e->fase, e->chave.pega)) return;
    e->chave.pega = true;
    tira_interativo(&e->interativos, INTER_CHAVEIRO);
    e->jog.mira = -1;                           // o interativo que estava na mira saiu da coleção
    audio_toca(e, SOM_INTERRUPTOR, 0.6f, 1.7f); // o tilintar da chave saindo do gancho
    avisa(e, TXT_AVISO_CHAVE_PEGA, CHAVE_AVISO_CURTO_S + 1.5f);
}

static void abre_porta(Estado *e) {
    e->chave.porta_aberta = true;
    e->chave.porta_t = 0.0f;
    e->chave.aviso_t = 0.0f;
    e->nivel.celula_trancada = -1;
    tira_parede_da_porta(&e->paredes);
    tira_interativo(&e->interativos, INTER_PORTA_ARQUIVO);
    e->jog.mira = -1;
    col_ruidos_adiciona(&e->ruidos, (Ruido){e->nivel.porta_arquivo, RUIDO_RAIO_PORTA, 1.0f, RUIDO_PORTA});
    audio_toca(e, SOM_INTERRUPTOR, 1.0f, 0.45f); // a fechadura cede e a folha corre
}

void chave_tenta_porta(Estado *e) {
    switch (regras_tenta_porta_arquivo(e->fase, e->chave.pega)) {
    case PORTA_ARQUIVO_ABRE:
        abre_porta(e);
        break;
    case PORTA_ARQUIVO_SEM_CHAVE:
        audio_toca(e, SOM_INTERRUPTOR, 0.8f, 0.3f); // a maçaneta não gira
        avisa(e, TXT_AVISO_PORTA_SEM_CHAVE, CHAVE_AVISO_CURTO_S);
        break;
    case PORTA_ARQUIVO_ANTES_DA_HORA:
        audio_toca(e, SOM_INTERRUPTOR, 0.8f, 0.3f);
        avisa(e, TXT_AVISO_PORTA_ANTES, CHAVE_AVISO_CURTO_S);
        break;
    }
}

// ================================================================== desenho
Vector3 chave_chaveiro_centro(void) {
    return (Vector3){CHAVEIRO_X, CHAVEIRO_Y, QUADRO_PAREDE_Z - CHAVEIRO_FUNDO_M * 0.5f};
}

// A chave pendurada no 2º gancho: a argola, a haste e os dentes, com uma etiqueta vermelha.
static Vector3 posicao_da_chave(void) {
    Vector3 c = chave_chaveiro_centro();
    return (Vector3){c.x + CHAVEIRO_LARGURA_M * 0.12f, c.y - 0.03f, c.z - CHAVEIRO_FUNDO_M * 0.5f - 0.012f};
}

static void desenha_chave(Vector3 k) {
    DrawCylinderWiresEx((Vector3){k.x, k.y + 0.012f, k.z}, (Vector3){k.x, k.y + 0.012f, k.z - 0.004f}, 0.016f,
                        0.016f, 10, COR_CHAVE);                                                // a argola
    DrawCubeV((Vector3){k.x, k.y - 0.035f, k.z}, (Vector3){0.008f, 0.06f, 0.004f}, COR_CHAVE); // a haste
    DrawCubeV((Vector3){k.x + 0.009f, k.y - 0.055f, k.z}, (Vector3){0.012f, 0.008f, 0.004f}, COR_CHAVE);
    DrawCubeV((Vector3){k.x + 0.007f, k.y - 0.042f, k.z}, (Vector3){0.008f, 0.007f, 0.004f}, COR_CHAVE);
    DrawCubeV((Vector3){k.x - 0.02f, k.y + 0.0f, k.z}, (Vector3){0.022f, 0.032f, 0.003f},
              (Color){0x9a, 0x22, 0x1c, 255}); // a etiqueta
}

void chave_desenha_chaveiro(const Estado *e) {
    Vector3 c = chave_chaveiro_centro();
    texturas_caixa_mat(e, MATERIAL_METAL, c,
                       (Vector3){CHAVEIRO_LARGURA_M, CHAVEIRO_ALTURA_M, CHAVEIRO_FUNDO_M},
                       texturas_modo(MATERIAL_METAL, 70), (Color){0x5a, 0x3c, 0x24, 255}); // a tábua
    float frente = c.z - CHAVEIRO_FUNDO_M * 0.5f;
    for (int i = 0; i < 4; i++) { // laço de desenho: os quatro ganchos
        float x = c.x - CHAVEIRO_LARGURA_M * 0.36f + i * CHAVEIRO_LARGURA_M * 0.24f;
        DrawCylinderEx((Vector3){x, c.y + 0.02f, frente}, (Vector3){x, c.y + 0.02f, frente - 0.04f}, 0.006f,
                       0.006f, 6, (Color){0x8a, 0x8c, 0x86, 255});
    }
    if (regras_chave_visivel(e->fase, e->chave.pega)) desenha_chave(posicao_da_chave());
}

void chave_desenha_porta(const Estado *e) {
    Vector3 p = e->nivel.porta_arquivo;
    float abre = e->chave.porta_aberta ? fminf(1.0f, e->chave.porta_t / PORTA_ARQUIVO_ABRE_S) : 0.0f;
    Vector3 t = PORTA_TAMANHO;
    Vector3 centro = {p.x, t.y * 0.5f, p.z + PORTA_DESLIZA_M * abre}; // corre para dentro da parede sul
    texturas_caixa_mat(e, MATERIAL_METAL, centro, t, texturas_modo(MATERIAL_METAL, 71),
                       (Color){0x66, 0x70, 0x60, 255}); // chapa de aço pintada, como os armários
    float face = p.x + t.x * 0.5f;                      // o lado do corredor
    Color escuro = {0x22, 0x24, 0x22, 255};
    DrawCubeV((Vector3){face + 0.02f, 1.05f, centro.z - t.z * 0.38f}, (Vector3){0.03f, 0.22f, 0.04f},
              escuro); // o puxador
    DrawCubeV((Vector3){face + 0.015f, 1.25f, centro.z - t.z * 0.38f}, (Vector3){0.025f, 0.1f, 0.07f},
              escuro); // a fechadura
    DrawCubeV((Vector3){face + 0.006f, 0.12f, centro.z}, (Vector3){0.01f, 0.24f, t.z - 0.1f},
              (Color){0x30, 0x34, 0x30, 255}); // a chapa de chute, gasta
}

// O LED da fechadura, na parede ao lado da porta (vermelho trancada, verde aberta), e um brilho que pulsa na
// chave: no escuro da S2, é o que mostra onde ela está.
void chave_desenha_brilho(const Estado *e) {
    if (regras_fase_eh_saida(e->fase)) {
        Vector3 p = e->nivel.porta_arquivo;
        Vector3 led = {p.x + TAMANHO_CELULA * 0.5f + 0.02f, 1.35f, p.z + TAMANHO_CELULA * 0.5f + 0.25f};
        Color cor = e->chave.porta_aberta ? (Color){0x40, 0xe0, 0x70, 255} : (Color){0xff, 0x30, 0x20, 255};
        DrawBillboard(e->camera, e->rec.halo, led, 0.18f, Fade(cor, 0.8f));
    }
    if (!regras_chave_visivel(e->fase, e->chave.pega)) return;
    float pulso = 0.55f + 0.45f * sinf(e->tempo_total * 3.0f);
    DrawBillboard(e->camera, e->rec.halo, posicao_da_chave(), 0.32f, Fade(COR_CHAVE, 0.45f * pulso));
}

void chave_desenha_aviso(const Estado *e) {
    const ChaveArquivo *k = &e->chave;
    if (k->aviso_t <= 0.0f || e->jog.modo == MODO_PAINEL || e->jog.modo == MODO_DIARIO) return;
    float s = ui_escala();
    float alfa = fminf(1.0f, fminf(k->aviso_t / 0.6f, (k->aviso_dur - k->aviso_t) / 0.3f + 0.2f));
    const char *texto = dialogo_texto((TextoId)k->aviso);
    float tamanho = 20 * s, largura = fminf(760 * s, GetScreenWidth() - 64 * s), util = largura - 36 * s;
    int linhas = 1 + (int)(ui_mede(e, texto, tamanho).x * 1.1f / util); // folga: a quebra é por palavra
    Rectangle r = {(GetScreenWidth() - largura) * 0.5f, 96 * s, largura, linhas * tamanho * 1.25f + 28 * s};
    DrawRectangleRec(r, Fade(BLACK, 0.72f * alfa));
    DrawRectangleLinesEx(r, fmaxf(1.0f, 2 * s), Fade(COR_CHAVE, alfa));
    ui_texto_quebrado(e, texto, (Vector2){r.x + 18 * s, r.y + 14 * s}, util, tamanho, Fade(RAYWHITE, alfa));
}
