// controle.c — o controle somado à Entrada do quadro. Só lê a raylib e preenche campos; quem decide o que
// cada ação faz continua sendo cada módulo (jogador.c, terminal.c, respiracao.c...).
#include "controle.h"
#include "ui.h"

#define CONTROLES_MAX 4 // a raylib conhece até 4; vale o primeiro que estiver ligado

// Mapeamentos no formato do SDL (gamecontrollerdb) para controles que a raylib não traz de fábrica: sem
// mapeamento, ela até vê o controle, mas todos os botões e eixos ficam em zero. Uma linha por controle.
static const char *MAPEAMENTOS =
    // GameSir K1 "for Xbox" (USB 3537:1082) no Linux: entra como HID genérico, não pelo driver xpad
    "03000000373500008210000010010000,GameSir-K1 Controller for Xbox,a:b0,b:b1,x:b3,y:b4,back:b10,start:b11,"
    "guide:b12,leftshoulder:b6,rightshoulder:b7,leftstick:b13,rightstick:b14,dpup:h0.1,dpdown:h0.4,"
    "dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a5,righttrigger:a4,"
    "platform:Linux,\n";

static bool usando_controle = false; // o último a mexer foi o controle (as dicas da tela seguem isto)
static bool seta_antes[4]; // o analógico esquerdo já estava empurrado: cima, baixo, esquerda, direita

static int primeiro_controle(void) {
    for (int i = 0; i < CONTROLES_MAX; i++) // padrão FOLD: o primeiro controle ligado (ou -1)
        if (IsGamepadAvailable(i)) return i;
    return -1;
}

static bool apertou(int c, int botao) {
    return IsGamepadButtonPressed(c, botao);
}

static Vector2 analogico(int c, int eixo_x, int eixo_y, float curva) {
    Vector2 bruto = {GetGamepadAxisMovement(c, eixo_x), GetGamepadAxisMovement(c, eixo_y)};
    return regras_analogico(bruto, CONTROLE_ZONA_MORTA, curva);
}

// Algum botão desceu neste quadro? (GetGamepadButtonPressed guarda o último, não diz se foi agora.)
static bool algum_botao(int c) {
    bool algum = false;
    for (int b = GAMEPAD_BUTTON_LEFT_FACE_UP; b <= GAMEPAD_BUTTON_RIGHT_THUMB; b++) // padrão FOLD: OU
        algum = algum || IsGamepadButtonPressed(c, b);
    return algum;
}

// O analógico esquerdo também navega nos menus: cada vez que passa do limiar conta como um toque na seta
// (como o IsKeyPressed). Andando, ninguém lê as setas, então não atrapalha.
static void setas_do_analogico(Vector2 esq, Entrada *in) {
    const float lim = CONTROLE_MENU_LIMIAR;
    bool agora[4] = {(esq.y < -lim), (esq.y > lim), (esq.x < -lim), (esq.x > lim)};
    bool *setas[4] = {&in->cima, &in->baixo, &in->esquerda, &in->direita};
    for (int i = 0; i < 4; i++) { // padrão MAP: direção empurrada agora e não antes → toque na seta
        *setas[i] = *setas[i] || (agora[i] && !seta_antes[i]);
        seta_antes[i] = agora[i];
    }
}

static void le_botoes(int c, Entrada *in) {
    bool a = apertou(c, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    bool start = apertou(c, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    in->interagir = in->interagir || a;
    in->confirmar = in->confirmar || a;
    in->respirar = in->respirar || a; // só o armário lê: lá, segurando o ar, o A não abre a porta
    in->voltar = in->voltar || apertou(c, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || start;
    in->pausa = in->pausa || start;
    in->lanterna = in->lanterna || apertou(c, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    in->tecla1 = in->tecla1 || apertou(c, GAMEPAD_BUTTON_RIGHT_FACE_LEFT); // X: LUZ A, chave A
    in->tecla2 = in->tecla2 || apertou(c, GAMEPAD_BUTTON_RIGHT_FACE_UP);   // Y: LUZ B, chave B
    in->ajuda = in->ajuda || apertou(c, GAMEPAD_BUTTON_MIDDLE_LEFT);
    in->cima = in->cima || apertou(c, GAMEPAD_BUTTON_LEFT_FACE_UP);
    in->baixo = in->baixo || apertou(c, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    in->esquerda = in->esquerda || apertou(c, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    in->direita = in->direita || apertou(c, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
}

// Devolve true se o controle foi mexido neste quadro (botão, analógico ou gatilho).
static bool le_controle(int c, Entrada *in) {
    Vector2 esq = analogico(c, GAMEPAD_AXIS_LEFT_X, GAMEPAD_AXIS_LEFT_Y, 1.0f);
    Vector2 dir = analogico(c, GAMEPAD_AXIS_RIGHT_X, GAMEPAD_AXIS_RIGHT_Y, CONTROLE_CURVA);
    bool gatilho = GetGamepadAxisMovement(c, GAMEPAD_AXIS_LEFT_TRIGGER) > CONTROLE_GATILHO_LIMIAR;
    bool anda = esq.x != 0.0f || esq.y != 0.0f, olha = dir.x != 0.0f || dir.y != 0.0f;
    if (anda) in->mover = (Vector2){esq.x, -esq.y}; // no analógico, para cima é negativo
    in->olhar = dir;
    in->correr = in->correr || gatilho || IsGamepadButtonDown(c, GAMEPAD_BUTTON_LEFT_TRIGGER_1) ||
                 IsGamepadButtonDown(c, GAMEPAD_BUTTON_LEFT_THUMB);
    setas_do_analogico(esq, in);
    le_botoes(c, in);
    return algum_botao(c) || anda || olha || gatilho;
}

// Avisa no log quando um controle aparece ou some (ajuda a conferir na feira se ele foi reconhecido).
static void avisa_troca(int c) {
    static int antes = -1;
    if (c == antes) return;
    if (c >= 0)
        TraceLog(LOG_INFO, "CONTROLE: %d conectado (%s)", c, GetGamepadName(c));
    else
        TraceLog(LOG_INFO, "CONTROLE: desconectado");
    antes = c;
}

void controle_inicia(void) {
    SetGamepadMappings(MAPEAMENTOS);
    char caminho[512]; // opcional: assets/gamecontrollerdb.txt traz os mapeamentos de outros controles
    if (!ui_caminho_asset("gamecontrollerdb.txt", caminho, sizeof(caminho))) return;
    char *texto = LoadFileText(caminho);
    if (texto == NULL) return;
    TraceLog(LOG_INFO, "CONTROLE: %d mapeamentos de %s", SetGamepadMappings(texto), caminho);
    UnloadFileText(texto);
}

void controle_le(Entrada *in) {
    bool teclado = in->alguma; // o teclado e o mouse já foram lidos
    int c = primeiro_controle();
    avisa_troca(c);
    bool mexeu = c >= 0 && le_controle(c, in);
    if (mexeu)
        usando_controle = true;
    else if (teclado || c < 0)
        usando_controle = false;
    in->controle = usando_controle;
    in->alguma = in->alguma || mexeu; // o modo feira não volta ao menu com alguém jogando no controle
}
