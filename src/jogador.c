// jogador.c — quem joga (professor ou professora) em 1ª pessoa: olhar, andar, correr, esconder-se e
// interagir.
#include "jogador.h"
#include "nivel.h"
#include "ui.h"
#include "respiracao.h"
#include "dialogo.h"
#include "audio_sintese.h"
#include "trilhas.h"
#include "criatura.h"
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
// Diferença angular a - b normalizada para [-π, π].
static float dif_angulo(float a, float b) {
    float d = fmodf(a - b, 2.0f * PI);
    if (d > PI) d -= 2.0f * PI;
    if (d < -PI) d += 2.0f * PI;
    return d;
}

// [REQ:RF2] câmera em 1ª pessoa pelo mouse (o deslocamento do mouse já é por quadro: não multiplica por dt)
static void olha(Estado *e) {
    Jogador *j = &e->jog;
    if (j->modo == MODO_PAINEL || j->modo == MODO_DIARIO) return;
    j->yaw -= e->in.mouse.x * SENSIBILIDADE_MOUSE;
    j->pitch -= e->in.mouse.y * SENSIBILIDADE_MOUSE;

    float pitch_max = PITCH_MAXIMO;
    if (j->modo == MODO_ARMARIO) {
        // Escondido, o olhar fica preso a um cone em volta da porta (só se vê pelas frestas).
        float d = dif_angulo(j->yaw, j->armario_yaw);
        if (d > ARMARIO_YAW_MAX) d = ARMARIO_YAW_MAX;
        if (d < -ARMARIO_YAW_MAX) d = -ARMARIO_YAW_MAX;
        j->yaw = j->armario_yaw + d;
        pitch_max = ARMARIO_PITCH_MAX;
    }
    if (j->pitch > pitch_max) j->pitch = pitch_max;
    if (j->pitch < -pitch_max) j->pitch = -pitch_max;
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

static void atualiza_folego(Estado *e, bool quer_correr, float dt) {
    Jogador *j = &e->jog;
    j->correndo = quer_correr && !j->exausto && j->folego > 0.0f;
    bool estava_correndo = j->correndo;
    if (j->correndo)
        j->folego -= dt;
    else
        j->folego += JOGADOR_FOLEGO_RECUPERA * dt;
    if (j->folego > JOGADOR_FOLEGO_MAX) j->folego = JOGADOR_FOLEGO_MAX;
    if (estava_correndo && j->folego <= 0.0f) { // zerou correndo: ofega e so volta a correr com 75% da barra
        j->exausto = true;
        audio_toca(e, SOM_RESPIRACAO_OFEGANTE, 1.0f, 1.0f);
    }
    if (j->folego < 0.0f) j->folego = 0.0f;
    if (j->exausto && j->folego >= JOGADOR_FOLEGO_MAX * FOLEGO_RETOMA_FRACAO) j->exausto = false;
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
        col_ruidos_adiciona(&e->ruidos,
                            (Ruido){j->pos, raio, 1.0f, j->correndo ? RUIDO_CORRIDA : RUIDO_PASSO});
    if (!trilhas_tem(e, TRILHA_PASSO_ANDAR)) // sem o arquivo, o passo sintetizado de antes
        audio_toca(e, SOM_PASSO, j->correndo ? 1.0f : 0.6f, 0.9f + GetRandomValue(0, 20) / 100.0f);
}

// [REQ:RF2] WASD anda, Shift corre (com fôlego); a posição nova vem da regra pura
static void anda(Estado *e, float dt) {
    Jogador *j = &e->jog;
    bool andando = e->in.mover.x != 0.0f || e->in.mover.y != 0.0f;
    atualiza_folego(e, e->in.correr && andando, dt);
    float velocidade = j->correndo ? JOGADOR_VEL_CORRER : JOGADOR_VEL_ANDAR;
    if (andando) move_com_colisao(e, regras_nova_posicao(j->pos, j->yaw, e->in.mover, velocidade, dt));
    passos(e, andando, dt);
}

// ------------------------------------------------------------------ armário
static int armario_mais_proximo(const Estado *e, Vector3 pos) {
    int melhor = 0;
    // padrão FOLD: o armário mais próximo
    for (int i = 1; i < e->nivel.n_armarios; i++)
        if (Vector3Distance(e->nivel.armarios[i], pos) < Vector3Distance(e->nivel.armarios[melhor], pos))
            melhor = i;
    return melhor;
}

static void entra_no_armario(Estado *e, const Interativo *it) {
    Jogador *j = &e->jog;
    j->armario = armario_mais_proximo(e, it->pos);
    j->modo = MODO_ARMARIO;
    Vector3 fora = Vector3Subtract(e->nivel.armario_saida[j->armario], e->nivel.armarios[j->armario]);
    j->yaw = atan2f(fora.x, fora.z);
    j->armario_yaw = j->yaw;
    j->pitch = 0.0f;
    audio_toca(e, SOM_INTERRUPTOR, 0.7f, 0.6f);
    // Entrar no armário a menos de 2 m dele enquanto ele te vê não adianta.
    bool visto = e->cri.ativo && e->cri.ve_jogador;
    if (visto && Vector3Distance(e->cri.pos, j->pos) < CRIATURA_DIST_ARMARIO) criatura_captura(e);
    if (j->modo == MODO_ARMARIO) respiracao_comeca(e); // o Precursor vem até a porta: segure a respiração
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
    case INTER_PAINEL2:
        j->modo = MODO_PAINEL;
        break;
    case INTER_DIARIO:
        j->modo = MODO_DIARIO;
        e->diario_lido = true;
        audio_toca(e, SOM_PAPEL, 0.8f, 1.0f);
        break;
    case INTER_ARMARIO:
        entra_no_armario(e, it);
        break;
    case INTER_PORTA:
        ui_fala(e, FALAS_PORTA_TRANCADA);
        break;
    }
}

static void atualiza_modos(Estado *e) {
    Jogador *j = &e->jog;
    if (j->modo == MODO_ARMARIO && e->in.interagir && !respiracao_ativa(e)) { // segurando o ar, não sai
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
        j->mira = col_interativos_na_mira(&e->interativos, jogador_olho(e), jogador_direcao(e),
                                          DISTANCIA_INTERACAO);
    } else {
        j->mira = -1;
        atualiza_folego(e, false, dt); // parado (inclusive escondido no armário), o fôlego volta
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
    for (int y = 0; y < h; y += fresta * 2)
        DrawRectangle(0, y, w, fresta + fresta / 3, BLACK);
    DrawRectangle(0, 0, w / 7, h, BLACK);
    DrawRectangle(w - w / 7, 0, w / 7, h, BLACK);
}
