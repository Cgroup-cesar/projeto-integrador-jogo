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
#include "chave.h"
#include "porta_sala.h"
#include <math.h>

#define PITCH_MAXIMO    1.35f
#define CONSOLE_PITCH   -0.06f // no console, o olhar desce um pouco (para a tela e o vidro)
#define CONSOLE_PERTO_M 0.5f   // a até isto da frente da mesa, ele está no console (e sair dá o passo)

void jogador_teleporta(Estado *e, Vector3 pos, float yaw) {
    e->jog.pos = pos;
    e->jog.yaw = yaw;
    e->jog.pitch = 0.0f;
    e->jog.mira = -1;
    e->jog.correndo = false;
    e->jog.armario_anima = false; // teleporte (fase nova, recomeço): sem animação de armário pela metade
    e->jog.console_anda = false;
}

// Onde se fica para usar o console: na frente da mesa (o vidro fica ao norte, além dela).
static Vector3 lugar_do_console(const Estado *e) {
    Vector3 con = e->nivel.console;
    return (Vector3){con.x, 0.0f, con.z + 0.4f};
}

static void comeca_passo(Estado *e, Vector3 para, bool vira) {
    Jogador *j = &e->jog;
    j->console_anda = true;
    j->console_vira = vira;
    j->console_t = 0.0f;
    j->console_de = j->pos;
    j->console_para = para;
    j->console_yaw_de = j->yaw;
    j->console_pitch_de = j->pitch;
}

void jogador_vai_ao_console(Estado *e) {
    comeca_passo(e, lugar_do_console(e), true);
}

void jogador_poe_no_console(Estado *e) {
    jogador_teleporta(e, lugar_do_console(e), PI); // yaw π = olhando para o norte (o vidro)
    e->jog.pitch = CONSOLE_PITCH;
}

// Sair do console é só se afastar dele: um passo para trás (para o sul, longe da mesa), olhando para onde
// já olhava. Longe da mesa, nada acontece.
void jogador_sai_do_console(Estado *e) {
    Jogador *j = &e->jog;
    j->console_anda = false; // saindo no meio da chegada: a chegada para ali
    Vector3 lugar = lugar_do_console(e);
    if (Vector3Distance(j->pos, lugar) > CONSOLE_PERTO_M) return;
    comeca_passo(e, (Vector3){j->pos.x, j->pos.y, fmaxf(j->pos.z, lugar.z + CONSOLE_AFASTA_M)}, false);
}

bool jogador_na_sala(const Estado *e) {
    int c, l;
    if (!nivel_celula_de(e->jog.pos, &c, &l)) return false;
    char ch = nivel_celula(&e->nivel, c, l);
    return ch == 'O' || ch == 'S' || ch == 'T';
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
// [REQ:RF2] câmera em 1ª pessoa pelo mouse (o deslocamento do mouse já é por quadro: não multiplica por dt)
// e pelo analógico direito (esse é uma velocidade: radianos por segundo, multiplicado por dt)
static void olha(Estado *e, float dt) {
    Jogador *j = &e->jog;
    // No terminal, no painel e no bilhete o cursor fica à mostra para clicar: a câmera não acompanha o mouse.
    if (j->modo == MODO_TERMINAL || j->modo == MODO_PAINEL || j->modo == MODO_DIARIO) return;
    if (j->modo == MODO_ARMARIO) { // escondido atrás da porta: o olhar fica preso na fresta, sem virar
        j->yaw = j->armario_yaw;
        j->pitch = 0.0f;
        return;
    }
    j->yaw -= e->in.mouse.x * SENSIBILIDADE_MOUSE + e->in.olhar.x * CONTROLE_GIRO_RAD_S * dt;
    j->pitch -= e->in.mouse.y * SENSIBILIDADE_MOUSE +
                e->in.olhar.y * CONTROLE_GIRO_RAD_S * CONTROLE_GIRO_VERTICAL * dt;
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

// Passos fazem barulho: andar, raio de 3 m; correr, raio de 10 m. A volta à sala (retorno.c) usa também.
void jogador_passos(Estado *e, bool andando, float dt) {
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
    jogador_passos(e, andando, dt);
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
    // a animação parte de onde a câmera está e de para onde ela olha (de frente para o armário)
    j->armario_de = jogador_olho(e);
    j->armario_yaw_de = j->yaw;
    j->armario_pitch_de = j->pitch;
    j->armario_saindo = false;
    j->armario_anima = true;
    j->armario_t = 0.0f;
    j->armario = armario_mais_proximo(e, it->pos);
    j->modo = MODO_ARMARIO;
    Vector3 fora = Vector3Subtract(e->nivel.armario_saida[j->armario], e->nivel.armarios[j->armario]);
    j->yaw = atan2f(fora.x, fora.z);
    j->armario_yaw = j->yaw;
    j->pitch = 0.0f;
    audio_toca(e, SOM_ARMARIO_ABRE, 0.85f, 1.0f); // a trava e o rangido: a porta abre para ele entrar
    // Entrar no armário a menos de 2 m dele enquanto ele te vê não adianta.
    bool visto = e->cri.ativo && e->cri.ve_jogador;
    if (visto && Vector3Distance(e->cri.pos, j->pos) < CRIATURA_DIST_ARMARIO) criatura_captura(e);
    if (j->modo == MODO_ARMARIO) respiracao_comeca(e); // se ele vinha atrás, pode vir até a porta
}

static void sai_do_armario(Estado *e) {
    Jogador *j = &e->jog;
    j->armario_de = jogador_olho(e); // ainda lá dentro
    j->armario_saindo = true;
    j->armario_anima = true;
    j->armario_t = 0.0f;
    j->modo = MODO_ANDANDO;
    j->pos = e->nivel.armario_saida[j->armario];
    audio_toca(e, SOM_ARMARIO_ABRE, 0.8f, 1.0f);
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
        porta_sala_interage(e); // trancada nas rodadas; nas saídas, abre e fecha
        break;
    case INTER_CHAVEIRO:
        chave_pega(e);
        break;
    case INTER_PORTA_ARQUIVO:
        chave_tenta_porta(e);
        break;
    }
}

static void atualiza_modos(Estado *e) {
    Jogador *j = &e->jog;
    bool entrando = j->armario_anima && !j->armario_saindo && j->armario_t < ARMARIO_ENTRA_S; // nem fechou
    if (j->modo == MODO_ARMARIO && e->in.interagir && !respiracao_ativa(e) && !entrando) { // segurando o ar,
                                                                                          // não sai
        sai_do_armario(e);
    } else if (j->modo == MODO_DIARIO && (e->in.interagir || e->in.voltar)) {
        j->modo = MODO_ANDANDO;
    } else if (j->modo == MODO_ANDANDO && e->in.interagir && j->mira >= 0) {
        interage(e, &e->interativos.itens[j->mira]);
    }
}

// ------------------------------------------------------------------ animação do armário
static float suave(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Quando a porta termina de fechar, em s desde o começo: no fim da entrada (por dentro); na saída, um pouco
// depois de o jogador sair (atrás dele).
static float fim_da_porta(const Jogador *j) {
    return j->armario_saindo ? ARMARIO_SAI_S + ARMARIO_FECHA_ATRAS_S + ARMARIO_FECHA_S : ARMARIO_ENTRA_S;
}

// 0 no começo do movimento da câmera, 1 no fim (ou parada).
static float progresso(const Jogador *j) {
    float dur = j->armario_saindo ? ARMARIO_SAI_S : ARMARIO_ENTRA_S;
    return j->armario_anima ? fminf(1.0f, j->armario_t / dur) : 1.0f;
}

// O relógio da animação e o som de fechar, que começa ARMARIO_BATIDA_S antes, para a batida cair quando a
// porta fecha (o de abrir toca ao apertar o botão, em entra_no_armario e sai_do_armario).
static void anima_armario(Estado *e, float dt) {
    Jogador *j = &e->jog;
    if (!j->armario_anima) return;
    float fim = fim_da_porta(j), batida = fim - ARMARIO_BATIDA_S, antes = j->armario_t;
    j->armario_t += dt;
    if (antes < batida && j->armario_t >= batida) // por fora (saindo), um pouco mais baixo
        audio_toca(e, SOM_ARMARIO_FECHA, j->armario_saindo ? 0.6f : 0.85f, 1.0f);
    if (j->armario_t > fim + 0.05f) j->armario_anima = false;
}

// Entrando: o passo para dentro (até 65% do tempo) e, a partir de 25%, a volta para a porta (dá meia-volta
// sempre pelo mesmo lado). Saindo: a porta abre primeiro e o passo para fora vem depois (a partir de 30%).
static void camera_do_armario(const Jogador *j, Vector3 *olho, Vector3 *dir) {
    float t = progresso(j);
    if (t >= 1.0f) return;
    if (j->armario_saindo) {
        *olho = Vector3Lerp(j->armario_de, *olho, suave((t - 0.3f) / 0.7f));
        return;
    }
    *olho = Vector3Lerp(j->armario_de, *olho, suave(t / 0.65f));
    float volta = j->armario_yaw - j->armario_yaw_de;
    while (volta > PI) volta -= 2.0f * PI;
    while (volta <= -PI) volta += 2.0f * PI;
    if (fabsf(volta) > PI - 0.05f) volta = PI; // meia-volta exata: sempre pelo mesmo lado
    float giro = suave((t - 0.25f) / 0.75f);
    float yaw = j->armario_yaw_de + volta * giro, pitch = j->armario_pitch_de * (1.0f - giro);
    *dir = (Vector3){sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch)};
}

// ------------------------------------------------------------------ o passo do console
// O giro de `de` até `para` pelo lado mais curto, em (−π, π].
static float menor_volta(float de, float para) {
    float volta = para - de;
    while (volta > PI)
        volta -= 2.0f * PI;
    while (volta <= -PI)
        volta += 2.0f * PI;
    return volta;
}

// Chegando, os pés vão até a frente da mesa e o olhar vira para o vidro, juntos; saindo, só os pés andam.
// Com colisão, como andar: o passo nunca atravessa a mesa nem uma parede.
static void passo_do_console(Estado *e, float dt) {
    Jogador *j = &e->jog;
    if (!j->console_anda) return;
    float dur = j->console_vira ? CONSOLE_CHEGA_S : CONSOLE_SAI_S;
    j->console_t += dt;
    float t = suave(j->console_t / dur);
    move_com_colisao(e, Vector3Lerp(j->console_de, j->console_para, t));
    if (j->console_vira) {
        j->yaw = j->console_yaw_de + menor_volta(j->console_yaw_de, PI) * t;
        j->pitch = Lerp(j->console_pitch_de, CONSOLE_PITCH, t);
    }
    if (j->console_t >= dur) j->console_anda = false;
}

// As folhas da porta abrem nos primeiros ARMARIO_ABRE_S e fecham nos ARMARIO_FECHA_S antes de fim_da_porta.
float jogador_porta_aberta(const Estado *e) {
    const Jogador *j = &e->jog;
    if (!j->armario_anima) return 0.0f;
    float fecha = suave((j->armario_t - (fim_da_porta(j) - ARMARIO_FECHA_S)) / ARMARIO_FECHA_S);
    return suave(j->armario_t / ARMARIO_ABRE_S) * (1.0f - fecha);
}

// O quanto a porta cobre a tela (0 = nada, 1 = as frestas inteiras). Com o modelo, a porta de verdade
// abre e fecha, e as frestas só entram com o jogador lá dentro e ela fechada. Sem ele, a porta da tela varre
// a tela: fecha no fim da entrada e abre no começo da saída.
float jogador_porta_armario(const Estado *e) {
    const Jogador *j = &e->jog;
    float t = progresso(j);
    if (e->rec.tem_armario) return j->modo == MODO_ARMARIO && t >= 1.0f ? 1.0f : 0.0f;
    if (j->modo != MODO_ARMARIO) return j->armario_saindo && t < 1.0f ? 1.0f - suave(t / 0.4f) : 0.0f;
    return t >= 1.0f ? 1.0f : suave((t - 0.72f) / 0.28f);
}

void jogador_atualiza(Estado *e, float dt) {
    olha(e, dt);
    Jogador *j = &e->jog;
    anima_armario(e, dt);
    passo_do_console(e, dt);
    if (j->modo == MODO_ANDANDO) {
        if (j->console_anda)
            atualiza_folego(e, false, dt); // o passo para trás anda sozinho: as teclas esperam por ele
        else
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
    Vector3 olho = jogador_olho(e), dir = jogador_direcao(e);
    camera_do_armario(&e->jog, &olho, &dir);
    e->camera.position = olho;
    e->camera.target = Vector3Add(olho, dir);
    e->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    e->camera.fovy = 70.0f;
    e->camera.projection = CAMERA_PERSPECTIVE;
}

// Escondido: a câmera olha pelas frestas (barras pretas por cima da cena). A porta varre a tela ao fechar e
// ao abrir: ela entra pela direita (a borda dela, escura, vem na frente) até cobrir tudo.
void jogador_desenha(const Estado *e) {
    float fechada = jogador_porta_armario(e);
    if (fechada <= 0.0f) return;
    int w = e->rec.alvo_largura, h = e->rec.alvo_altura;
    int x0 = (int)((1.0f - fechada) * (float)w); // onde está a borda da porta
    // Ripas finas (cerca de 30% da altura): dá para ver o Precursor inteiro na porta, mas ainda é uma fresta.
    int passo = h / 7, ripa = passo * 3 / 10;
    for (int y = passo / 2; y < h; y += passo) // padrão MAP: cada ripa da porta
        DrawRectangle(x0, y, w - x0, ripa, BLACK);
    DrawRectangle(x0 + (w - x0) - w / 7, 0, w / 7, h, BLACK);     // o batente do outro lado
    if (fechada < 1.0f) DrawRectangle(x0, 0, w / 7, h, BLACK);    // a borda da porta, vindo na frente
    else DrawRectangle(0, 0, w / 7, h, BLACK);
}
