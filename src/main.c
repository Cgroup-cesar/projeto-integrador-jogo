// main.c — game loop imperativo: entrada → atualiza → desenha; modo feira; --smoke.
// Aqui fica só a orquestração (o "como"). Quem decide (o "o quê") são as regras puras.
#include "estado.h"
#include "jogo.h"
#include "jogador.h"
#include "rodadas.h"
#include "circuito.h"
#include "compositor.h"
#include "criatura.h"
#include "nivel.h"
#include <math.h>
#include "posfx.h"
#include "audio_sintese.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

static Estado estado; // o estado inteiro do jogo (estático: nada de malloc por quadro)
static const char *arquivo_captura = NULL; // --captura <arquivo.png>: salva um quadro e segue
static float captura_em = 0.8f;            // --captura-em <segundos>
static Fase fase_inicial = FASE_MENU;      // --fase <NOME> (debug): começa direto numa fase
static Fase fase_captura = FASE_TOTAL;     // --captura-fase <NOME>: a captura espera essa fase
static bool mudo = false;                  // --mudo: sem som

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
    float tempo_fase_anterior;
    float andando_t;        // teste de colisão: anda de verdade até o vidro na primeira saída
    bool colisao_conferida;
    bool pego_testado;      // 1ª partida, S2: espera a criatura ouvir, perseguir e pegar
    unsigned estados_vistos; // bits dos EstadoCriatura que apareceram nesse teste
    bool armario_testado;   // 2ª partida, S1: entra e sai de um armário
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

// 1ª partida, S2: fica no corredor de baixo, a 4 células do duto, e corre sem sair do lugar (barulho de 10 m)
// quando a criatura surge. Ela precisa ouvir, investigar, ver, perseguir e pegar; a saída recomeça.
static bool piloto_espera_ser_pego(Estado *e, Entrada *in) {
    if (piloto.partida != 0 || e->fase != FASE_S2 || piloto.pego_testado) return false;
    if (e->cri.ativo) piloto.estados_vistos |= 1u << e->cri.estado;
    if (e->pegos_na_fase > 0) {
        piloto.pego_testado = true;
        bool investigou = piloto.estados_vistos & (1u << CRIATURA_INVESTIGA);
        bool perseguiu = piloto.estados_vistos & (1u << CRIATURA_PERSEGUE);
        if (!investigou || !perseguiu) piloto_falha("a criatura pegou sem passar por INVESTIGA e PERSEGUE");
        else printf("SMOKE  criatura ok: ouviu (INVESTIGA), viu (PERSEGUE) e pegou; a saída recomeça\n");
        return false;
    }
    if (criatura_em_susto(e)) return true;
    if (piloto.tempo_na_fase > 60.0f) {
        piloto_falha("a criatura não pegou o jogador parado no corredor");
        return true;
    }
    // A leste do duto, fora da 1ª rota de patrulha (que vai para oeste): ela só o acha se ouvir.
    // Olhando para o duto (oeste) e "correndo" de lado contra a parede norte: não sai do lugar, mas faz barulho.
    if (e->tempo_fase < 0.2f) jogador_teleporta(e, nivel_centro_celula(18, 11), -PI * 0.5f);
    bool surgiu = e->cri.ativo && e->cri.surgindo >= 1.0f;
    in->correr = surgiu;
    in->mover.x = surgiu ? 1.0f : 0.0f;
    return true;
}

// 2ª partida, S1: entra no primeiro armário e sai de novo.
static bool piloto_testa_armario(Estado *e, Entrada *in) {
    if (piloto.partida != 1 || e->fase != FASE_S1 || piloto.armario_testado) return false;
    if (e->jog.modo == MODO_ARMARIO) {
        in->interagir = true;
        piloto.armario_testado = true;
        printf("SMOKE  armário ok: entrou e saiu\n");
        return true;
    }
    Vector3 a = e->nivel.armarios[0], fora = e->nivel.armario_saida[0];
    jogador_teleporta(e, fora, atan2f(a.x - fora.x, a.z - fora.z));
    in->interagir = true;
    return true;
}

// Na S2 ele lê o diário antes do painel (o terminal passa a mostrar a anotação recuperada).
static bool piloto_le_diario(Estado *e, Entrada *in) {
    if (e->fase != FASE_S2) return false;
    if (e->jog.modo == MODO_DIARIO) {
        in->interagir = true; // fecha a folha
        return true;
    }
    if (e->diario_lido || e->jog.modo != MODO_ANDANDO) return false;
    Vector3 d = e->nivel.diario;
    jogador_teleporta(e, (Vector3){d.x + 1.2f, 0.0f, d.z}, -PI * 0.5f);
    in->interagir = true;
    return true;
}

static void piloto_saida(Estado *e, Entrada *in) {
    bool painel1 = e->fase == FASE_S1;
    if (piloto_testa_armario(e, in) || piloto_le_diario(e, in)) return;
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

// Para o smoke passar também pelos caminhos de erro: na 1ª partida o vidro rompe uma vez na R3
// (3 erros, jumpscare e recomeço); na 2ª, a R2 tem um erro.
static int piloto_erros_desejados(const Estado *e) {
    if (piloto.partida == 0 && e->fase == FASE_R3) return VIDAS_VIDRO;
    if (piloto.partida == 1 && e->fase == FASE_R2) return 1;
    return 0;
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
            rodadas_piloto(e, in, piloto_erros_desejados(e));
        } else if (e->jog.modo == MODO_ANDANDO) {
            jogador_vai_ao_console(e);
            in->interagir = true;
        }
        break;
    case FASE_S1:
    case FASE_S2: piloto_saida(e, in); break;
    case FASE_RESPOSTA_FINAL: compositor_piloto(e, in, piloto.partida == 1); break;
    case FASE_FIM_RUIM:
    case FASE_FIM_BOM: in->confirmar = e->fim.tela_final && e->fim.tempo_tela > 0.5f; break;
    default: break;
    }
}

static void piloto_entrada(Estado *e, Entrada *in, float dt) {
    memset(in, 0, sizeof(*in));
    if (e->fase != piloto.fase_vista) {
        piloto_confere_troca(piloto.fase_vista, e->fase);
        piloto.fase_vista = e->fase;
        piloto.tempo_na_fase = 0.0f;
        piloto.tempo_fase_anterior = 0.0f;
    }
    if (e->tempo_fase + 0.001f < piloto.tempo_fase_anterior)
        printf("SMOKE  partida %d: %s recomeçou (%d erro(s) na fase)\n", piloto.partida + 1, regras_nome_fase(e->fase), e->erros_na_fase);
    piloto.tempo_fase_anterior = e->tempo_fase;
    if (e->fase == FASE_S1 && piloto_confere_colisao(e, in, dt)) return;
    if (piloto_espera_ser_pego(e, in)) return; // a cada quadro: precisa segurar a corrida
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
        if (strcmp(argv[i], "--mudo") == 0) mudo = true;
        if (strcmp(argv[i], "--captura") == 0 && i + 1 < argc) arquivo_captura = argv[++i];
        if (strcmp(argv[i], "--captura-em") == 0 && i + 1 < argc) sscanf(argv[++i], "%f", &captura_em);
        bool fase = strcmp(argv[i], "--fase") == 0, captura = strcmp(argv[i], "--captura-fase") == 0;
        if ((fase || captura) && i + 1 < argc) {
            const char *nome = argv[++i];
            for (int f = 0; f < FASE_TOTAL; f++) {
                if (strcmp(nome, regras_nome_fase((Fase)f)) != 0) continue;
                if (fase) fase_inicial = (Fase)f;
                else fase_captura = (Fase)f;
            }
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
    bool na_hora = fase_captura == FASE_TOTAL ? e->tempo_total > captura_em : (e->fase == fase_captura && e->tempo_fase > captura_em);
    if (arquivo_captura != NULL && na_hora) {
        salva_captura(arquivo_captura);
        arquivo_captura = NULL;
    }
    if (e->pedir_captura) {
        captura_numerada();
        e->pedir_captura = false;
    }
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "--exporta-sons") == 0) { // não abre janela: só grava os .wav
        audio_exporta(argv[2]);
        return 0;
    }
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
    criatura_carrega_modelo(&estado.rec);
    if (mudo) SetMasterVolume(0.0f);
    if (estado.modo_smoke) {
        SetMasterVolume(0.0f); // o smoke roda calado
        SetRandomSeed(1985);   // o smoke é determinístico: mesmo resultado em toda execução
        piloto_comeca_partida();
    }
    jogo_inicia(&estado);
    if (!estado.modo_smoke) jogo_vai_para_fase(&estado, fase_inicial);

    while (!WindowShouldClose() && !estado.sair) {
        quadro(&estado);
        if (estado.modo_smoke && piloto.codigo_saida >= 0) break;
    }

    jogo_finaliza(&estado);
    criatura_descarrega_modelo(&estado.rec);
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
