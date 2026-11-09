// jogador.c — o Prof. Elias Moura em 1ª pessoa: olhar, andar, correr, esconder-se e interagir.
#include "jogador.h"
#include "nivel.h"
#include "ui.h"
#include "dialogo.h"
#include "audio_sintese.h"
#include <math.h>

#define PITCH_MAXIMO 1.35f

void jogador_teleporta(Estado *e, Vector3 pos, float yaw) {
    e->jog.pos = pos;
    e->jog.yaw = yaw;
    e->jog.pitch = 0.0f;
    e->jog.mira = -1;
    e->jog.correndo = false;
}

void jogador_vai_ao_console(Estado *e) {
    Vector3 con = e->nivel.console;
    jogador_teleporta(e, (Vector3){con.x, 0.0f, con.z + 0.4f}, PI); // yaw π = olhando para o norte (o vidro)
    e->jog.pitch = -0.06f;
}

Vector3 jogador_olho(const Estado *e) {
    if (e->jog.modo == MODO_ARMARIO) {
        Vector3 a = e->nivel.armarios[e->jog.armario];
        return (Vector3){a.x, 1.55f, a.z};
    }
    float balanco = sinf(e->jog.balanco) * 0.035f;
    return (Vector3){e->jog.pos.x, e->jog.pos.y + ALTURA_OLHOS + balanco, e->jog.pos.z};
}

Vector3 jogador_direcao(const Estado *e) {
    float cp = cosf(e->jog.pitch);
    return (Vector3){sinf(e->jog.yaw) * cp, sinf(e->jog.pitch), cosf(e->jog.yaw) * cp};
}

// ------------------------------------------------------------------ olhar
static void olha(Estado *e) {
    Jogador *j = &e->jog;
    if (j->modo == MODO_PAINEL || j->modo == MODO_DIARIO) return;
    j->yaw -= e->in.mouse.x * SENSIBILIDADE_MOUSE;
    j->pitch -= e->in.mouse.y * SENSIBILIDADE_MOUSE;
    if (j->pitch > PITCH_MAXIMO) j->pitch = PITCH_MAXIMO;
    if (j->pitch < -PITCH_MAXIMO) j->pitch = -PITCH_MAXIMO;
}

// ------------------------------------------------------------------ andar
// [REQ:RF4] colisão do jogador com paredes, vidro e armários (fold de CheckCollisionBoxes), eixo por eixo
static void move_com_colisao(Estado *e, Vector3 alvo) {
    Jogador *j = &e->jog;
    Vector3 tenta_x = {alvo.x, j->pos.y, j->pos.z};
    if (!col_paredes_colide(&e->paredes, tenta_x, JOGADOR_RAIO, 1.8f)) j->pos.x = alvo.x;
    Vector3 tenta_z = {j->pos.x, j->pos.y, alvo.z};
    if (!col_paredes_colide(&e->paredes, tenta_z, JOGADOR_RAIO, 1.8f)) j->pos.z = alvo.z;
}

static void atualiza_folego(Jogador *j, bool quer_correr, float dt) {
    j->correndo = quer_correr && !j->exausto && j->folego > 0.0f;
    if (j->correndo) j->folego -= dt;
    else j->folego += JOGADOR_FOLEGO_RECUPERA * dt;
    if (j->folego <= 0.0f) {
        j->folego = 0.0f;
        j->exausto = true; // sem fôlego: só volta a correr depois de recuperar 1 s
    }
    if (j->folego >= 1.0f) j->exausto = false;
    if (j->folego > JOGADOR_FOLEGO_MAX) j->folego = JOGADOR_FOLEGO_MAX;
}

// Passos fazem barulho: andar, raio de 3 m; correr, raio de 10 m.
static void passos(Estado *e, bool andando, float dt) {
    Jogador *j = &e->jog;
    if (!andando) {
        j->tempo_passo = 0.0f;
        return;
    }
    j->balanco += dt * (j->correndo ? 13.0f : 8.5f);
    j->tempo_passo += dt;
    float intervalo = j->correndo ? INTERVALO_PASSO_CORRER : INTERVALO_PASSO_ANDAR;
    if (j->tempo_passo < intervalo) return;
    j->tempo_passo -= intervalo;
    float raio = j->correndo ? RUIDO_RAIO_CORRER : RUIDO_RAIO_ANDAR;
    if (regras_fase_eh_saida(e->fase))
        col_ruidos_adiciona(&e->ruidos, (Ruido){j->pos, raio, 1.0f, j->correndo ? RUIDO_CORRIDA : RUIDO_PASSO});
    audio_toca(e, SOM_PASSO, j->correndo ? 1.0f : 0.6f, 0.9f + GetRandomValue(0, 20) / 100.0f);
}

static void anda(Estado *e, float dt) {
    Jogador *j = &e->jog;
    bool andando = e->in.mover.x != 0.0f || e->in.mover.y != 0.0f;
    atualiza_folego(j, e->in.correr && andando, dt);
    float velocidade = j->correndo ? JOGADOR_VEL_CORRER : JOGADOR_VEL_ANDAR;
    if (andando) move_com_colisao(e, regras_nova_posicao(j->pos, j->yaw, e->in.mover, velocidade, dt));
    passos(e, andando, dt);
}

// ------------------------------------------------------------------ armário
static int armario_mais_proximo(const Estado *e, Vector3 pos) {
    int melhor = 0;
    for (int i = 1; i < e->nivel.n_armarios; i++)
        if (Vector3Distance(e->nivel.armarios[i], pos) < Vector3Distance(e->nivel.armarios[melhor], pos)) melhor = i;
    return melhor;
}

static void entra_no_armario(Estado *e, const Interativo *it) {
    Jogador *j = &e->jog;
    j->armario = armario_mais_proximo(e, it->pos);
    j->modo = MODO_ARMARIO;
    Vector3 fora = Vector3Subtract(e->nivel.armario_saida[j->armario], e->nivel.armarios[j->armario]);
    j->yaw = atan2f(fora.x, fora.z);
    j->pitch = 0.0f;
    audio_toca(e, SOM_INTERRUPTOR, 0.7f, 0.6f);
    // Entrar no armário a menos de 2 m dele enquanto ele te vê não adianta.
    bool visto = e->cri.ativo && e->cri.ve_jogador;
    if (visto && Vector3Distance(e->cri.pos, j->pos) < CRIATURA_DIST_ARMARIO) e->evento = EV_PEGO;
}

static void sai_do_armario(Estado *e) {
    Jogador *j = &e->jog;
    j->modo = MODO_ANDANDO;
    j->pos = e->nivel.armario_saida[j->armario];
    audio_toca(e, SOM_INTERRUPTOR, 0.7f, 0.55f);
}

// ------------------------------------------------------------------ interação
static void interage(Estado *e, const Interativo *it) {
    Jogador *j = &e->jog;
    switch (it->tipo) {
    case INTER_CONSOLE:
        jogador_vai_ao_console(e);
        j->modo = MODO_TERMINAL;
        break;
    case INTER_PAINEL1:
    case INTER_PAINEL2: j->modo = MODO_PAINEL; break;
    case INTER_DIARIO:
        j->modo = MODO_DIARIO;
        e->diario_lido = true;
        audio_toca(e, SOM_PAPEL, 0.8f, 1.0f);
        break;
    case INTER_ARMARIO: entra_no_armario(e, it); break;
    case INTER_PORTA: ui_fala(e, FALAS_PORTA_TRANCADA); break;
    }
}

static void atualiza_modos(Estado *e) {
    Jogador *j = &e->jog;
    if (j->modo == MODO_ARMARIO && e->in.interagir) {
        sai_do_armario(e);
    } else if (j->modo == MODO_DIARIO && (e->in.interagir || e->in.voltar)) {
        j->modo = MODO_ANDANDO;
    } else if (j->modo == MODO_ANDANDO && e->in.interagir && j->mira >= 0) {
        interage(e, &e->interativos.itens[j->mira]);
    }
}

void jogador_atualiza(Estado *e, float dt) {
    olha(e);
    Jogador *j = &e->jog;
    if (j->modo == MODO_ANDANDO) {
        anda(e, dt);
        j->mira = col_interativos_na_mira(&e->interativos, jogador_olho(e), jogador_direcao(e), DISTANCIA_INTERACAO);
    } else {
        j->mira = -1;
        atualiza_folego(j, false, dt);
    }
    atualiza_modos(e);
}

void jogador_atualiza_camera(Estado *e) {
    Vector3 olho = jogador_olho(e);
    e->camera.position = olho;
    e->camera.target = Vector3Add(olho, jogador_direcao(e));
    e->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    e->camera.fovy = 70.0f;
    e->camera.projection = CAMERA_PERSPECTIVE;
}

// Escondido: a câmera olha pelas frestas (barras pretas por cima da cena).
void jogador_desenha(const Estado *e) {
    if (e->jog.modo != MODO_ARMARIO) return;
    int w = e->rec.alvo_largura, h = e->rec.alvo_altura;
    int fresta = h / 14;
    for (int y = 0; y < h; y += fresta * 2) DrawRectangle(0, y, w, fresta + fresta / 3, BLACK);
    DrawRectangle(0, 0, w / 7, h, BLACK);
    DrawRectangle(w - w / 7, 0, w / 7, h, BLACK);
}
