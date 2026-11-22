// identidade.c — as duas perguntas do começo: "Professor ou professora?" (setas) e o nome (campo de texto).
// A edição do nome é a função pura regras_nome_edita; aqui só ficam a entrada, o som e o desenho.
#include "identidade.h"
#include "audio_sintese.h"
#include "dialogo.h"
#include "falha.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Zera o buffer inteiro, não só o primeiro byte: na feira, o nome de quem jogou antes não fica na memória.
void identidade_reinicia(Estado *e) {
    e->ident.feminino = false;
    memset(e->ident.nome, 0, sizeof(e->ident.nome));
}

void identidade_garante_nome(Estado *e) {
    Identidade *id = &e->ident;
    size_t n = strlen(id->nome);
    if (n > 0 && id->nome[n - 1] == ' ') id->nome[n - 1] = '\0'; // o espaço digitado no fim não conta
    if (id->nome[0] == '\0') snprintf(id->nome, sizeof(id->nome), "%s", dialogo_texto(TXT_NOME_PADRAO));
}

static void avanca(Estado *e, EtapaIntro proxima) {
    e->intro.etapa = proxima;
    e->intro.tempo_etapa = 0.0f;
}

static void atualiza_genero(Estado *e, bool segue_sozinho) {
    if (e->in.esquerda || e->in.direita || e->in.cima || e->in.baixo) {
        e->ident.feminino = !e->ident.feminino;
        e->menu.troca_t = e->tempo_total; // a opção nova entra com a falha (falha.c)
        audio_toca(e, SOM_INTERRUPTOR, 0.4f, 1.3f);
    }
    if (e->in.confirmar || segue_sozinho) avanca(e, INTRO_NOME);
}

static void atualiza_nome(Estado *e, bool segue_sozinho) {
    Identidade *id = &e->ident;
    bool mudou = false;
    for (int i = 0; i < e->in.n_letras; i++) // padrão FOLD: cada letra digitada entra no nome (acumulador)
        mudou |= regras_nome_edita(id->nome, sizeof(id->nome), NOME_MAX_LETRAS, e->in.letras[i]);
    if (e->in.apagar) mudou |= regras_nome_edita(id->nome, sizeof(id->nome), NOME_MAX_LETRAS, NOME_APAGAR);
    if (mudou) audio_toca(e, SOM_MAQUINA_ESCREVER, 0.5f, 0.9f + GetRandomValue(0, 20) / 100.0f);
    if (!e->in.confirmar && !segue_sozinho) return;
    identidade_garante_nome(e); // ENTER com o campo vazio fica com o nome padrão
    avanca(e, INTRO_DATILOGRAFA);
}

void identidade_atualiza(Estado *e, float dt) {
    Intro *it = &e->intro;
    if (e->fase != FASE_INTRO || it->etapa > INTRO_NOME) return;
    it->tempo_etapa = e->in.alguma ? 0.0f : it->tempo_etapa + dt; // conta o tempo sem nenhuma tecla
    bool segue_sozinho = e->modo_feira && it->tempo_etapa > IDENTIDADE_FEIRA_S;
    if (it->etapa == INTRO_GENERO)
        atualiza_genero(e, segue_sozinho);
    else
        atualiza_nome(e, segue_sozinho);
}

// ------------------------------------------------------------------ desenho
static void desenha_opcao_genero(const Estado *e, TextoId texto, float centro_x, float y, bool ativa) {
    float s = ui_escala();
    const char *t = dialogo_texto(texto);
    Vector2 m = ui_mede(e, t, 30 * s);
    if (!ativa) {
        ui_opcao(e, t, (Vector2){centro_x - m.x * 0.5f - 6 * s, y}, m.x + 32 * s, 30 * s, false, COR_FOSFORO);
        return;
    }
    float forca = 0.0f, dx = falha_deslocamento(e, MENU_FALHA_LEVE, &forca);
    Rectangle r = {centro_x - m.x * 0.5f - 16 * s + dx, y - 4 * s, m.x + 32 * s, 38 * s};
    DrawRectangleRounded(r, 0.25f, 6, Fade(COR_FOSFORO, 0.16f));
    DrawRectangle((int)r.x, (int)r.y, (int)(3 * s), (int)r.height, COR_FOSFORO);
    falha_texto(e, t, (Vector2){r.x + 16 * s, y}, 30 * s, COR_FOSFORO, forca, r);
}

static void desenha_genero(const Estado *e) {
    float s = ui_escala(), meio = GetScreenWidth() * 0.5f;
    ui_texto_centro(e, dialogo_texto(TXT_ID_PERGUNTA_GENERO), 250 * s, 36 * s, COR_FOSFORO);
    desenha_opcao_genero(e, TXT_ID_PROFESSOR, meio - 170 * s, 350 * s, !e->ident.feminino);
    desenha_opcao_genero(e, TXT_ID_PROFESSORA, meio + 170 * s, 350 * s, e->ident.feminino);
    ui_texto_centro(e, dialogo_texto(TXT_ID_DICA_GENERO), 560 * s, 18 * s, Fade(COR_VIDRO, 0.8f));
}

static void desenha_nome(const Estado *e) {
    float s = ui_escala();
    char texto[160];
    dialogo_personaliza(&e->ident, dialogo_texto(TXT_ID_PERGUNTA_NOME), texto, sizeof(texto));
    ui_texto_centro(e, texto, 250 * s, 36 * s, COR_FOSFORO);
    Rectangle campo = {GetScreenWidth() * 0.5f - 260 * s, 330 * s, 520 * s, 56 * s};
    ui_painel(campo, COR_FOSFORO);
    bool cursor_aceso = fmodf(e->tempo_fase, 1.0f) < 0.55f; // o "_" pisca, como num terminal
    snprintf(texto, sizeof(texto), "%s%s", e->ident.nome, cursor_aceso ? "_" : "");
    ui_texto(e, texto, (Vector2){campo.x + 20 * s, campo.y + 12 * s}, 32 * s, RAYWHITE);
    snprintf(texto, sizeof(texto), dialogo_texto(TXT_ID_DICA_NOME), dialogo_texto(TXT_NOME_PADRAO));
    ui_texto_centro(e, texto, 560 * s, 18 * s, Fade(COR_VIDRO, 0.8f));
    ui_texto_centro(e, dialogo_texto(TXT_ID_AVISO_NOME), 600 * s, 16 * s, Fade(COR_FOSFORO, 0.5f));
}

void identidade_desenha(const Estado *e) {
    if (e->fase != FASE_INTRO || e->intro.etapa > INTRO_NOME) return;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
    falha_chuvisco(e, TELAS_ESTATICA_ALFA, TELAS_ESTATICA_PICO, false); // bem fraco: só um grão de fita
    if (e->intro.etapa == INTRO_GENERO)
        desenha_genero(e);
    else
        desenha_nome(e);
}
