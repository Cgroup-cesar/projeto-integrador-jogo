// compositor.c — a resposta final por fichas e os dois finais.
// A frase é analisada pelo motor de lógica: inválida → SINTAXE INVÁLIDA (sem penalidade);
// contradição (nunca pode ser verdadeira) → FIM BOM; qualquer frase satisfazível → FIM RUIM.
#include "compositor.h"
#include "terminal.h"
#include "criatura.h"
#include "susto.h"
#include "jogador.h"
#include "audio_sintese.h"
#include "trilhas.h"
#include "ui.h"
#include "dialogo.h"
#include "clique.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum {
    FICHA_P,
    FICHA_Q,
    FICHA_NAO_,
    FICHA_E_,
    FICHA_OU_,
    FICHA_SE_,
    FICHA_APAGAR,
    FICHA_TRANSMITIR,
    TOTAL_FICHAS
};
#define COLUNAS_FICHAS 4

static void desenha_textos_fim(const Estado *e);

// ================================================================== a frase
void compositor_inicia(Estado *e) {
    memset(&e->comp, 0, sizeof(e->comp));
    e->comp.resultado = EV_NENHUM;
}

static void avisa(Compositor *c, TextoId texto, const char *detalhe) {
    c->mensagem = dialogo_texto(texto);
    snprintf(c->detalhe, sizeof(c->detalhe), "%s", detalhe ? detalhe : "");
    c->mensagem_t = 4.0f;
}

static Ficha ficha_da_tecla(int indice) {
    switch (indice) {
    case FICHA_P:
        return (Ficha){FICHA_ATOMO, ATOMO_P};
    case FICHA_Q:
        return (Ficha){FICHA_ATOMO, ATOMO_Q};
    case FICHA_NAO_:
        return (Ficha){FICHA_NAO, 0};
    case FICHA_E_:
        return (Ficha){FICHA_E, 0};
    case FICHA_OU_:
        return (Ficha){FICHA_OU, 0};
    default:
        return (Ficha){FICHA_SE_ENTAO, 0};
    }
}

// [REQ:RF6] a condição de vitória/derrota: a frase transmitida é analisada pelo motor de lógica
static void transmite(Estado *e) {
    Compositor *c = &e->comp;
    ErroLogica erro;
    EventoFase resultado = regras_avalia_transmissao(c->fichas, c->n, &erro);
    if (resultado == EV_NENHUM) { // sem penalidade: só explica o que faltou
        avisa(c, TXT_TERMINAL_SINTAXE_INVALIDA, erro.mensagem);
        audio_toca(e, SOM_FALSO, 0.6f, 1.3f);
        return;
    }
    c->transmitindo = true;
    c->transmitindo_t = 0.0f;
    c->resultado = resultado;
    if (!trilhas_tem(e, TRILHA_VOZ_PRECURSOR)) audio_toca(e, SOM_VOZ_PRECURSOR, 1.0f, 0.85f);
}

static void usa_ficha(Estado *e, int indice) {
    Compositor *c = &e->comp;
    c->acoes++;
    audio_toca(e, SOM_INTERRUPTOR, 0.6f, 1.2f);
    if (indice == FICHA_TRANSMITIR) {
        transmite(e);
    } else if (indice == FICHA_APAGAR) {
        if (c->n > 0) c->n--;
    } else if (c->n >= MAX_FICHAS_FRASE) {
        avisa(c, TXT_TERMINAL_FRASE_CHEIA, NULL);
    } else {
        c->fichas[c->n++] = ficha_da_tecla(indice);
    }
}

static void move_cursor(Compositor *c, const Entrada *in) {
    int col = c->cursor % COLUNAS_FICHAS, lin = c->cursor / COLUNAS_FICHAS;
    if (in->esquerda) col = (col + COLUNAS_FICHAS - 1) % COLUNAS_FICHAS;
    if (in->direita) col = (col + 1) % COLUNAS_FICHAS;
    if (in->cima || in->baixo) lin = 1 - lin;
    c->cursor = lin * COLUNAS_FICHAS + col;
}

void compositor_atualiza(Estado *e, float dt) {
    Compositor *c = &e->comp;
    c->mensagem_t = fmaxf(0.0f, c->mensagem_t - dt);
    e->cri.fala_t = 0.3f; // ele espera, com a mão no vidro
    if (c->transmitindo) {
        c->transmitindo_t += dt;
        if (c->transmitindo_t > 1.8f) e->evento = c->resultado;
        return;
    }
    if (e->jog.modo != MODO_TERMINAL) return;
    if (e->in.voltar) {
        e->jog.modo = MODO_ANDANDO;
        jogador_sai_do_console(e); // um passo para trás, sem virar
        return;
    }
    move_cursor(c, &e->in);
    bool clicou = clique_escolhe(&e->in, CLIQUE_FICHA, &c->cursor); // passar por cima escolhe; clicar usa
    if (e->in.confirmar || clicou) usa_ficha(e, c->cursor);
}

// ================================================================== desenho do compositor
static void desenha_fichas(const Estado *e, Vector2 pos) {
    float s = ui_escala();
    for (int i = 0; i < TOTAL_FICHAS; i++) {
        Vector2 p = {pos.x + (i % COLUNAS_FICHAS) * 136 * s, pos.y + (i / COLUNAS_FICHAS) * 40 * s};
        Color cor = i == FICHA_TRANSMITIR ? COR_ALARME : i == FICHA_APAGAR ? COR_AMBAR : COR_FOSFORO;
        ui_opcao(e, dialogo_ficha(i), p, 130 * s, 16 * s, e->comp.cursor == i, cor);
        clique_area(CLIQUE_FICHA, i, (Rectangle){p.x - 10 * s, p.y - 4 * s, 130 * s, 24 * s});
    }
}

static int ficha_para_indice(Ficha f) {
    if (f.tipo == FICHA_ATOMO) return f.atomo == ATOMO_P ? FICHA_P : FICHA_Q;
    return f.tipo == FICHA_NAO  ? FICHA_NAO_
           : f.tipo == FICHA_E  ? FICHA_E_
           : f.tipo == FICHA_OU ? FICHA_OU_
                                : FICHA_SE_;
}

// A frase montada, em fichas, e a mesma frase em notação lógica (quando já fecha).
// A frase das fichas em notação lógica (P, Q, ¬, ∧, ∨, →). Devolve false se ela ainda não é uma frase válida.
static bool formula_das_fichas(const Compositor *c, char *saida, size_t tamanho) {
    PoolLogico pool;
    logica_pool_limpa(&pool);
    ErroLogica erro;
    int raiz = c->n > 0 ? logica_parse_fichas(&pool, c->fichas, c->n, &erro) : -1;
    static const char *const nomes[] = {"P", "Q"};
    if (raiz < 0) return false;
    logica_para_texto(&pool, raiz, nomes, saida, tamanho);
    return true;
}

static void desenha_frase(const Estado *e, Vector2 pos, float largura) {
    float s = ui_escala();
    const Compositor *c = &e->comp;
    char frase[256] = "";
    // padrão FOLD: junta as fichas num texto só
    for (int i = 0; i < c->n; i++) {
        strncat(frase, dialogo_ficha(ficha_para_indice(c->fichas[i])), sizeof(frase) - strlen(frase) - 2);
        strncat(frase, " ", sizeof(frase) - strlen(frase) - 1);
    }
    ui_texto(e, dialogo_texto(TXT_TERMINAL_FRASE), pos, 16 * s, Fade(COR_FOSFORO, 0.6f));
    float h = ui_texto_quebrado(e, c->n > 0 ? frase : "_", (Vector2){pos.x, pos.y + 24 * s}, largura, 18 * s,
                                COR_FOSFORO);
    char formal[128];
    bool valida = formula_das_fichas(c, formal, sizeof(formal));
    if (!valida)
        snprintf(formal, sizeof(formal), "%s", c->n > 0 ? dialogo_texto(TXT_TERMINAL_INCOMPLETA) : "");
    ui_texto(e, formal, (Vector2){pos.x, pos.y + 30 * s + h}, 22 * s,
             valida ? COR_VIDRO : Fade(COR_FOSFORO, 0.4f));
}

void compositor_desenha(const Estado *e) {
    if (e->fase == FASE_FIM_RUIM || e->fase == FASE_FIM_BOM) {
        desenha_textos_fim(e);
        return;
    }
    if (e->fase != FASE_RESPOSTA_FINAL || e->jog.modo != MODO_TERMINAL) return;
    const Compositor *c = &e->comp;
    float s = ui_escala();
    Rectangle area = terminal_moldura(e, dialogo_texto(TXT_TERMINAL_RESPONDA));
    if (c->transmitindo) {
        bool pisca = fmodf(c->transmitindo_t, 0.5f) < 0.3f;
        ui_texto(e, dialogo_texto(TXT_TERMINAL_TRANSMITINDO), (Vector2){area.x, area.y + 120 * s}, 34 * s,
                 pisca ? COR_ALARME : Fade(COR_ALARME, 0.4f));
        return;
    }
    ui_texto_quebrado(e, dialogo_texto(TXT_TERMINAL_FICHAS), (Vector2){area.x, area.y}, area.width, 16 * s,
                      COR_FOSFORO);
    ui_texto(e, dialogo_texto(TXT_TERMINAL_LEGENDA_PQ), (Vector2){area.x, area.y + 26 * s}, 15 * s,
             Fade(COR_VIDRO, 0.8f));
    desenha_fichas(e, (Vector2){area.x + 10 * s, area.y + 64 * s});
    desenha_frase(e, (Vector2){area.x, area.y + 170 * s}, area.width);
    if (c->mensagem_t > 0.0f) {
        ui_texto(e, c->mensagem, (Vector2){area.x, area.y + 330 * s}, 26 * s, COR_ALARME);
        ui_texto_quebrado(e, c->detalhe, (Vector2){area.x, area.y + 364 * s}, area.width, 16 * s, COR_AMBAR);
    }
    // Esc sai do terminal: está na barra de teclas do rodapé (teclas.c)
}

// ================================================================== piloto automático (--smoke)
// Final ruim: primeiro uma frase inválida ([E] + TRANSMITIR → SINTAXE INVÁLIDA), depois P SE…ENTÃO Q.
// Final bom: P E NÃO P.
void compositor_piloto(const Estado *e, Entrada *in, bool final_bom) {
    static const int ruim[] = {FICHA_E_,  FICHA_TRANSMITIR, FICHA_APAGAR,    FICHA_P,
                               FICHA_SE_, FICHA_Q,          FICHA_TRANSMITIR};
    static const int bom[] = {FICHA_P, FICHA_E_, FICHA_NAO_, FICHA_P, FICHA_TRANSMITIR};
    const int *plano = final_bom ? bom : ruim;
    int tamanho = final_bom ? 5 : 7;
    const Compositor *c = &e->comp;
    if (c->transmitindo || c->acoes >= tamanho) return;
    if (e->jog.modo != MODO_TERMINAL) {
        in->interagir = true; // voltou a andar? o console está logo à frente
        return;
    }
    int alvo = plano[c->acoes];
    int col = c->cursor % COLUNAS_FICHAS, lin = c->cursor / COLUNAS_FICHAS;
    if (lin != alvo / COLUNAS_FICHAS)
        in->baixo = true;
    else if (col < alvo % COLUNAS_FICHAS)
        in->direita = true;
    else if (col > alvo % COLUNAS_FICHAS)
        in->esquerda = true;
    else
        in->confirmar = true;
}

// ================================================================== os dois finais
void compositor_inicia_fim(Estado *e) {
    memset(&e->fim, 0, sizeof(e->fim));
    // Os glifos do vidro se desfazem em fragmentos (nos dois finais).
    const Nivel *n = &e->nivel;
    for (int i = 0; i < 70; i++) {
        Vector3 p = {n->vidro_x0 + 3.0f + GetRandomValue(0, 600) / 100.0f,
                     1.8f + GetRandomValue(0, 90) / 100.0f, n->vidro_z - 0.05f};
        Vector3 v = {GetRandomValue(-120, 120) / 100.0f, GetRandomValue(-40, 120) / 100.0f,
                     GetRandomValue(-60, 60) / 100.0f};
        col_particulas_adiciona(&e->particulas, (Particula){p, v, 2.5f, 2.5f, PARTICULA_FRAGMENTO});
    }
}

// FIM RUIM: TRANSMISSÃO ACEITA → "OBRIGADO, PROFESSOR." no vidro → o vidro se dissolve e as luzes morrem →
// a silhueta a 1 m → jumpscare final → tela preta com o texto → placar.
static void atualiza_fim_ruim(Estado *e, float antes, float t) {
    if (antes < 0.1f && t >= 0.1f) audio_toca(e, SOM_ALARME, 0.7f, 0.8f);
    if (t > 5.0f && t < 7.6f) e->apagao_t = 0.1f; // as luzes morrem
    if (antes < 6.2f && t >= 6.2f) {              // a silhueta, a 1 m do jogador
        Criatura *c = &e->cri;
        Vector3 frente = jogador_direcao(e);
        c->contido = false;
        c->ativo = true;
        c->surgindo = 1.0f;
        c->pos = (Vector3){e->jog.pos.x + frente.x * 1.0f, 0.0f, e->jog.pos.z + frente.z * 1.0f};
        c->yaw = atan2f(-frente.x, -frente.z);
    }
    if (antes < 7.0f && t >= 7.0f) susto_inicia(e, EV_NENHUM);
}

// FIM BOM: os glifos se fragmentam, ele convulsiona e congela, 2 s de silêncio total, as falas e o texto.
static void atualiza_fim_bom(Estado *e, float antes, float t) {
    if (antes < 2.6f && t >= 2.6f) e->cri.congelado_t = 1.0f;
    if (antes < 2.8f && t >= 2.8f) audio_silencia(e);
    if (antes < 4.8f && t >= 4.8f) ui_fala(e, FALAS_FIM_BOM);
}

// O texto do final e o "por quê" já estão na tela (passou o momento deles e o susto acabou).
bool compositor_fim_lendo(const Estado *e) {
    float inicio = e->fase == FASE_FIM_RUIM ? FIM_RUIM_TEXTO_S : FIM_BOM_TEXTO_S;
    return !e->fim.tela_final && e->fim.tempo >= inicio && !susto_ativo(e);
}

void compositor_atualiza_fim(Estado *e, float dt) {
    if (e->fim.tela_final) return;
    if (compositor_fim_lendo(e)) { // a cena para e espera a leitura: ENTER ou FIM_LEITURA_MAX_S vão ao placar
        e->fim.leitura += dt;
        if (e->in.confirmar || e->fim.leitura >= FIM_LEITURA_MAX_S) e->fim.tela_final = true;
        return;
    }
    float antes = e->fim.tempo;
    e->fim.tempo += dt;
    if (e->in.confirmar && e->fim.tempo > 1.0f && !susto_ativo(e)) { // ENTER acelera a cena...
        e->fim.tempo += 3.0f;
        ui_legenda_pula(e); // ...e a fala da vez, para ela não ficar por cima do texto do final
    }
    if (e->fase == FASE_FIM_RUIM)
        atualiza_fim_ruim(e, antes, e->fim.tempo);
    else
        atualiza_fim_bom(e, antes, e->fim.tempo);
}

// Efeitos por cima da cena (dentro do render target): a tela vai ao preto no fim de cada cinemática.
void compositor_desenha_fim(const Estado *e) {
    float t = e->fim.tempo;
    float preto =
        e->fase == FASE_FIM_RUIM ? (t > 8.2f ? 1.0f : 0.0f) : fminf(1.0f, fmaxf(0.0f, (t - 13.0f) / 1.0f));
    if (e->fase == FASE_FIM_BOM && t > 2.8f && t < 4.8f)
        preto = fmaxf(preto, 0.55f); // o silêncio escurece a sala
    if (preto > 0.0f) DrawRectangle(0, 0, e->rec.alvo_largura, e->rec.alvo_altura, Fade(BLACK, preto));
}

// O texto do final e, logo abaixo, por que ele aconteceu: a frase transmitida e o que ela tem de especial.
// Quem perdeu sai sabendo o que tentar na próxima vez; quem ganhou, por que deu certo.
static void desenha_texto_e_porque(const Estado *e, const char *texto, float y) {
    float s = ui_escala(), largura = GetScreenWidth() - 320 * s;
    float h = ui_texto_quebrado(e, texto, (Vector2){160 * s, y}, largura, 30 * s, RAYWHITE);
    char formula[128], modelo[512], porque[640];
    if (!formula_das_fichas(&e->comp, formula, sizeof(formula))) return;
    dialogo_personaliza(&e->ident,
                        dialogo_texto(e->fase == FASE_FIM_BOM ? TXT_FIM_BOM_PORQUE : TXT_FIM_RUIM_PORQUE),
                        modelo, sizeof(modelo));
    snprintf(porque, sizeof(porque), modelo, formula); // o nome nunca tem '%' (regras_nome_edita não aceita)
    ui_texto_quebrado(e, porque, (Vector2){160 * s, y + h + 28 * s}, largura, 22 * s, COR_AMBAR);
}

// Os textos das cinemáticas, em resolução de janela.
static void desenha_textos_fim(const Estado *e) {
    if (e->fim.tela_final || susto_ativo(e)) return;
    float s = ui_escala(), t = e->fim.tempo;
    char tela[512]; // o texto final, com o nome e o gênero de quem jogou
    dialogo_personaliza(&e->ident,
                        dialogo_texto(e->fase == FASE_FIM_RUIM ? TXT_FIM_RUIM_TELA : TXT_FIM_BOM_TELA), tela,
                        sizeof(tela));
    if (e->fase == FASE_FIM_RUIM) {
        if (t < 2.4f) ui_texto_centro(e, dialogo_texto(TXT_FIM_RUIM_ACEITA), 200 * s, 40 * s, COR_ALARME);
        if (t > 2.2f && t < 5.4f) { // os glifos do vidro se reorganizam em português legível
            Vector2 tela =
                GetWorldToScreen((Vector3){e->cri.pos.x, 2.95f, e->nivel.vidro_z}, e->camera); // sobre ele
            char texto[96];
            dialogo_personaliza(&e->ident, dialogo_texto(TXT_FIM_RUIM_OBRIGADO), texto, sizeof(texto));
            Vector2 m = ui_mede(e, texto, 44 * s);
            float tremor = sinf(t * 30.0f) * 2.0f * s, margem = 40 * s;
            // Segue a criatura, mas sem sair da tela: a projeção muda com o formato da tela (16:9, 16:10) e
            // com para onde a câmera olha, e o texto chegava a ficar cortado no topo.
            float x = Clamp(tela.x - m.x * 0.5f, margem, GetScreenWidth() - margem - m.x);
            float y = Clamp(tela.y - m.y * 0.5f, 110 * s, GetScreenHeight() * 0.5f - m.y);
            ui_texto(e, texto, (Vector2){x + tremor, y}, 44 * s,
                     Fade(COR_FOSFORO, fminf(1.0f, (t - 2.2f) * 1.5f)));
        }
        if (t > FIM_RUIM_TEXTO_S) desenha_texto_e_porque(e, tela, 220 * s);
    } else if (t > FIM_BOM_TEXTO_S) {
        desenha_texto_e_porque(e, tela, 200 * s);
    }
    if (!compositor_fim_lendo(e)) return;
    char dica[64]; // ENTER leva ao placar; sem ninguém, vai sozinho em FIM_LEITURA_MAX_S
    snprintf(dica, sizeof(dica), dialogo_texto(TXT_FIM_VER_PLACAR),
             (int)(FIM_LEITURA_MAX_S - e->fim.leitura + 0.99f));
    ui_texto_centro(e, dica, GetScreenHeight() - 70 * s, 22 * s, Fade(COR_AMBAR, 0.85f));
}
