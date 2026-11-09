// main.c — game loop imperativo: entrada → atualiza → desenha; modo feira; --smoke.
// Aqui fica só a orquestração (o "como"). Quem decide (o "o quê") são as regras puras.
#include "estado.h"
#include "jogo.h"
#include "jogador.h"
#include "rodadas.h"
#include "circuito.h"
#include "compositor.h"
#include "posfx.h"
#include "audio_sintese.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

static Estado estado; // o estado inteiro do jogo (estático: nada de malloc por quadro)
static const char *arquivo_captura = NULL; // --captura <arquivo.png>: salva um quadro e segue
static float captura_em = 0.8f;            // --captura-em <segundos>
static Fase fase_inicial = FASE_MENU;      // --fase <NOME> (debug): começa direto numa fase

// ================================================================== entrada
static void le_entrada(Entrada *in) {
    memset(in, 0, sizeof(*in));
    in->mover.x = (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
    in->mover.y = (float)(IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
    in->mouse = GetMouseDelta();
    in->correr = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    in->interagir = IsKeyPressed(KEY_E);
    in->voltar = IsKeyPressed(KEY_ESCAPE);
    in->confirmar = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    in->cima = IsKeyPressed(KEY_UP);
    in->baixo = IsKeyPressed(KEY_DOWN);
    in->esquerda = IsKeyPressed(KEY_LEFT);
    in->direita = IsKeyPressed(KEY_RIGHT);
    in->tecla1 = IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1);
    in->tecla2 = IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2);
    in->tecla3 = IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3);
#if TECLAS_DEBUG
    in->f1 = IsKeyPressed(KEY_F1);
    in->f2 = IsKeyPressed(KEY_F2);
    in->f3 = IsKeyPressed(KEY_F3);
    in->f5 = IsKeyPressed(KEY_F5);
    in->f9 = IsKeyPressed(KEY_F9);
    in->f12 = IsKeyPressed(KEY_F12);
#endif
    in->alguma = GetKeyPressed() != 0 || in->mover.x != 0 || in->mover.y != 0 || in->mouse.x != 0 ||
                 in->mouse.y != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ================================================================== piloto automático (--smoke)
// Joga a partida inteira duas vezes (final ruim, depois final bom) usando a mesma interface do jogador:
// teclas no terminal, no painel e no compositor. Nas saídas ele "anda" por teletransporte até o painel.
// A cada troca de fase confere se a sequência bate com regras_roteiro_partida (o mesmo do test_fluxo).
typedef struct {
    int partida; // 0 = final ruim, 1 = final bom
    Fase fase_vista;
    float tempo_na_fase;
    float espera;
    EventoFase roteiro[16];
    int roteiro_n, roteiro_i;
    float andando_t;        // teste de colisão: anda de verdade até o vidro na primeira saída
    bool colisao_conferida;
    int codigo_saida;       // −1 = ainda jogando; 0 = ok; 1 = falhou
} Piloto;

static Piloto piloto = {.codigo_saida = -1};

static void piloto_falha(const char *motivo) {
    printf("SMOKE FALHOU na fase %s (partida %d): %s\n", regras_nome_fase(estado.fase), piloto.partida + 1, motivo);
    piloto.codigo_saida = 1;
}

static void piloto_comeca_partida(void) {
    piloto.roteiro_n = regras_roteiro_partida(piloto.partida == 1, piloto.roteiro, 16);
    piloto.roteiro_i = 0;
}

// Confere a troca de fase contra o roteiro. Recomeços (vidro rompido, pego) mantêm a fase e são aceitos.
static void piloto_confere_troca(Fase de, Fase para) {
    if (piloto.roteiro_i >= piloto.roteiro_n) {
        piloto_falha("fase além do roteiro");
        return;
    }
    Fase esperada = regras_proxima_fase(de, piloto.roteiro[piloto.roteiro_i]);
    if (para != esperada) {
        piloto_falha("troca de fase fora do roteiro");
        return;
    }
    piloto.roteiro_i++;
    printf("SMOKE  partida %d: %s -> %s\n", piloto.partida + 1, regras_nome_fase(de), regras_nome_fase(para));
    if (para != FASE_MENU) return;
    piloto.partida++;
    if (piloto.partida >= 2) {
        printf("SMOKE OK: a partida foi jogada até os dois finais.\n");
        piloto.codigo_saida = 0;
    } else {
        piloto_comeca_partida();
    }
}

// Anda 1,5 s para o norte a partir do início: o vidro (z ≈ 7) tem que segurar o jogador.
static bool piloto_confere_colisao(Estado *e, Entrada *in, float dt) {
    if (piloto.colisao_conferida) return false;
    if (piloto.andando_t == 0.0f) jogador_teleporta(e, e->nivel.inicio, PI);
    piloto.andando_t += dt;
    if (piloto.andando_t < 1.5f) {
        in->mover.y = 1.0f;
        return true;
    }
    piloto.colisao_conferida = true;
    if (e->jog.pos.z < e->nivel.vidro_z + 0.2f + JOGADOR_RAIO - 0.05f) piloto_falha("o jogador atravessou o vidro");
    else printf("SMOKE  colisão ok: andou até z=%.2f e parou antes do vidro (z=%.2f)\n", e->jog.pos.z, e->nivel.vidro_z);
    return true;
}

static void piloto_saida(Estado *e, Entrada *in) {
    bool painel1 = e->fase == FASE_S1;
    if (e->painel.resolvido) {
        jogador_teleporta(e, e->nivel.inicio, PI); // volta para a sala de observação
        return;
    }
    if (e->jog.modo == MODO_PAINEL) {
        circuito_piloto(e, in);
        return;
    }
    if (e->jog.modo != MODO_ANDANDO) return; // escondido ou lendo: espera o próximo passo
    Vector3 p = painel1 ? e->nivel.painel1 : e->nivel.painel2;
    Vector3 em_pe = painel1 ? (Vector3){p.x, 0.0f, p.z + 1.2f} : (Vector3){p.x + 1.3f, 0.0f, p.z};
    jogador_teleporta(e, em_pe, painel1 ? PI : -PI * 0.5f);
    in->interagir = true;
}

static void piloto_age(Estado *e, Entrada *in) {
    switch (e->fase) {
    case FASE_MENU:
        e->menu.cursor = 0;
        in->confirmar = true;
        break;
    case FASE_INTRO: in->confirmar = true; break;
    case FASE_R1:
    case FASE_R2:
    case FASE_R3:
        if (e->jog.modo == MODO_TERMINAL) {
            rodadas_piloto(e, in);
        } else if (e->jog.modo == MODO_ANDANDO) {
            jogador_vai_ao_console(e);
            in->interagir = true;
        }
        break;
    case FASE_S1:
    case FASE_S2: piloto_saida(e, in); break;
    case FASE_RESPOSTA_FINAL: compositor_piloto(e, in, piloto.partida == 1); break;
    case FASE_FIM_RUIM:
    case FASE_FIM_BOM: in->confirmar = e->fim.tela_final && e->fim.tempo > 0.5f; break;
    default: break;
    }
}

static void piloto_entrada(Estado *e, Entrada *in, float dt) {
    memset(in, 0, sizeof(*in));
    if (e->fase != piloto.fase_vista) {
        piloto_confere_troca(piloto.fase_vista, e->fase);
        piloto.fase_vista = e->fase;
        piloto.tempo_na_fase = 0.0f;
    }
    if (e->fase == FASE_S1 && piloto_confere_colisao(e, in, dt)) return;
    piloto.tempo_na_fase += dt;
    if (piloto.tempo_na_fase > SMOKE_LIMITE_FASE_S) {
        piloto_falha("tempo esgotado (travou?)");
        return;
    }
    piloto.espera -= dt;
    if (piloto.espera > 0.0f) return;
    piloto.espera = 0.15f;
    piloto_age(e, in);
}

// ================================================================== atualiza e desenha
static void atualiza(Estado *e, float dt) {
    e->tempo_total += dt;
    posfx_atualiza(e, dt);
    jogo_atualiza(e, dt);
    audio_atualiza(e, dt);
}

static void desenha(const Estado *e) {
    posfx_inicia_cena(e);
    jogo_desenha_cena(e);
    posfx_termina_cena(e);

    BeginDrawing();
    ClearBackground(BLACK);
    posfx_desenha(e);
    jogo_desenha_interface(e);
    EndDrawing();
}

// Lê o quadro que acabou de ser desenhado e salva em PNG.
static void salva_captura(const char *caminho) {
    Image quadro = LoadImageFromScreen();
    if (ExportImage(quadro, caminho)) TraceLog(LOG_INFO, "PRECURSOR: captura salva em %s", caminho);
    UnloadImage(quadro);
}

// F12: docs/img/captura_001.png, captura_002.png...
static void captura_numerada(void) {
    char caminho[128];
    for (int i = 1; i < 1000; i++) {
        snprintf(caminho, sizeof(caminho), "docs/img/captura_%03d.png", i);
        if (!FileExists(caminho)) break;
    }
    salva_captura(caminho);
}

// ================================================================== início e fim
static void le_argumentos(Estado *e, int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--smoke") == 0) e->modo_smoke = true;
        if (strcmp(argv[i], "--feira") == 0) e->modo_feira = true;
        if (strcmp(argv[i], "--captura") == 0 && i + 1 < argc) arquivo_captura = argv[++i];
        if (strcmp(argv[i], "--captura-em") == 0 && i + 1 < argc) sscanf(argv[++i], "%f", &captura_em);
        if (strcmp(argv[i], "--fase") == 0 && i + 1 < argc) {
            const char *nome = argv[++i];
            for (int f = 0; f < FASE_TOTAL; f++)
                if (strcmp(nome, regras_nome_fase((Fase)f)) == 0) fase_inicial = (Fase)f;
        }
    }
}

static void abre_janela(void) {
    char titulo[128];
    snprintf(titulo, sizeof(titulo), "%s%s", TITULO_JOGO, TITULO_PROVISORIO ? " [NOME PROVISÓRIO]" : "");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_JANELA, ALTURA_JANELA, titulo);
    SetWindowMinSize(640, 360);
    SetExitKey(KEY_NULL); // Esc é do jogo (sair do terminal, pausar), não fecha a janela
}

static void quadro(Estado *e) {
    // [REQ:RF1] dt de GetFrameTime() limitado a DT_MAXIMO; no smoke, passo fixo (velocidade acelerada)
    float dt = e->modo_smoke ? DT_MAXIMO : GetFrameTime();
    if (dt > DT_MAXIMO) dt = DT_MAXIMO;
    if (e->modo_smoke) piloto_entrada(e, &e->in, dt);
    else le_entrada(&e->in);
    atualiza(e, dt);
    desenha(e);
    if (arquivo_captura != NULL && e->tempo_total > captura_em) {
        salva_captura(arquivo_captura);
        arquivo_captura = NULL;
    }
    if (e->pedir_captura) {
        captura_numerada();
        e->pedir_captura = false;
    }
}

int main(int argc, char **argv) {
    le_argumentos(&estado, argc, argv);
    abre_janela();
    if (!IsWindowReady()) { // sem monitor (tela dormindo, sessão remota): sai com uma mensagem, sem travar
        printf("Não foi possível abrir a janela do jogo (nenhum monitor disponível).\n");
        return 1;
    }
    // [REQ:RF1] janela e game loop a 60 FPS: entrada → atualiza → desenha
    SetTargetFPS(estado.modo_smoke ? 0 : FPS_ALVO);
    estado.vhs_ligado = VHS_LIGADO_AO_INICIAR;
    posfx_inicia(&estado.rec);
    ui_carrega_fonte(&estado.rec);
    audio_inicia(&estado.rec);
    if (estado.modo_smoke) piloto_comeca_partida();
    jogo_inicia(&estado);
    if (!estado.modo_smoke) jogo_vai_para_fase(&estado, fase_inicial);

    while (!WindowShouldClose() && !estado.sair) {
        quadro(&estado);
        if (estado.modo_smoke && piloto.codigo_saida >= 0) break;
    }

    jogo_finaliza(&estado);
    audio_finaliza(&estado.rec);
    ui_descarrega_fonte(&estado.rec);
    posfx_finaliza(&estado.rec);
    CloseWindow();
    if (estado.modo_smoke && piloto.codigo_saida != 0) {
        if (piloto.codigo_saida < 0) printf("SMOKE FALHOU: a janela fechou antes do fim (fase %s)\n", regras_nome_fase(estado.fase));
        return 1;
    }
    return 0;
}
