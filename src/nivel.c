// nivel.c — o bloco de contenção: mapa ASCII → paredes, móveis, interativos e luzes.
// O mapa é texto para a equipe editar. 1 célula = 2 m; pé-direito 3 m.
#include "nivel.h"
#include "console_modelo.h"
#include "caixas.h"
#include "mesa_bilhete.h"
#include "armario_modelo.h"
#include "porta_bunker.h"
#include "cameras.h"
#include "quadro.h"
#include "lampada_sala.h"
#include "texturas.h"
#include "cenario.h"
#include "posfx.h"
#include "painel_modelo.h"
#include <math.h>
#include <string.h>

// # parede · C cela · V Precursor (na cela) ou duto (no corredor) · G vidro · O sala de observação
// S início · T console · D porta · . piso · L armário · 1 painel 1 · 2 painel 2 · N diário
static const char *const MAPA[MAPA_ALTURA] = {
    "########################", "#CCCCCC##########.....##", "#CCCVCC##########..1..##",
    "#GGGGGG##########L....##", "#OOOTOO############D####", "#OOSOOOD....L.......L.##",
    "#OOOOOO#.############.##", "########.############.##", "########L############.##",
    "###..2.#.############.##", "###N...D.############L##", "###L...#......V.......##",
    "########################", "########################",
};

// Lâmpadas dos corredores e das salas (coluna, linha). Edite à vontade.
static const int LAMPADAS[][2] = {
    {10, 5}, {13, 5}, {15, 5}, {18, 5}, {20, 5}, {21, 8}, {8, 7},
    {8, 10}, {11, 11}, {16, 11}, {20, 11}, {19, 2}, {3, 5},
}; // o Arquivo não tem lâmpada de teto: a única luz lá dentro é a da mesa do bilhete (mesa_bilhete.c)

// ================================================================== coordenadas
Vector3 nivel_centro_celula(int coluna, int linha) {
    return (Vector3){coluna * TAMANHO_CELULA + 1.0f, 0.0f, linha * TAMANHO_CELULA + 1.0f};
}

bool nivel_celula_de(Vector3 pos, int *coluna, int *linha) {
    int c = (int)floorf(pos.x / TAMANHO_CELULA);
    int l = (int)floorf(pos.z / TAMANHO_CELULA);
    if (coluna) *coluna = c;
    if (linha) *linha = l;
    return c >= 0 && c < MAPA_LARGURA && l >= 0 && l < MAPA_ALTURA;
}

// [REQ:RNF4] objeto fora do mapa: qualquer célula fora da grade conta como parede
char nivel_celula(const Nivel *n, int coluna, int linha) {
    if (coluna < 0 || coluna >= MAPA_LARGURA || linha < 0 || linha >= MAPA_ALTURA) return '#';
    return n->celulas[linha][coluna];
}

bool nivel_caminhavel(const Nivel *n, int coluna, int linha) {
    char c = nivel_celula(n, coluna, linha);
    return strchr("#CGT12N", c) == NULL;
}

bool nivel_na_sala_de_observacao(const Nivel *n, Vector3 pos) {
    int c, l;
    if (!nivel_celula_de(pos, &c, &l)) return false;
    char ch = nivel_celula(n, c, l);
    return ch == 'O' || ch == 'S' || ch == 'T';
}

// ================================================================== leitura do mapa
// O armário fica encostado na primeira parede vizinha (oeste, norte, leste, sul) e deixa o corredor livre.
static void registra_armario(Nivel *n, int c, int l) {
    if (n->n_armarios >= MAX_ARMARIOS) return;
    const int dc[4] = {-1, 0, 1, 0}, dl[4] = {0, -1, 0, 1};
    int lado = 0;
    for (int i = 0; i < 4; i++) {
        if (nivel_celula(n, c + dc[i], l + dl[i]) == '#') {
            lado = i;
            break;
        }
    }
    Vector3 centro = nivel_centro_celula(c, l);
    Vector3 parede = {(float)dc[lado], 0.0f, (float)dl[lado]};
    n->armarios[n->n_armarios] = Vector3Add(centro, Vector3Scale(parede, 0.62f));
    n->armario_saida[n->n_armarios] = Vector3Subtract(centro, Vector3Scale(parede, 0.45f));
    n->n_armarios++;
}

// A cela é o retângulo das células 'C' (e da 'V' do Precursor), até o vidro: a névoa fica dentro dela.
static void cresce_cela(Nivel *n, int c, int l) {
    n->cela_min.x = fminf(n->cela_min.x, c * TAMANHO_CELULA);
    n->cela_min.z = fminf(n->cela_min.z, l * TAMANHO_CELULA);
    n->cela_max.x = fmaxf(n->cela_max.x, (c + 1) * TAMANHO_CELULA);
    n->cela_max.z = fmaxf(n->cela_max.z, (l + 1) * TAMANHO_CELULA);
}

static void registra_celula(Nivel *n, char ch, int c, int l) {
    Vector3 centro = nivel_centro_celula(c, l);
    switch (ch) {
    case 'S':
        n->inicio = centro;
        break;
    case 'T':
        n->console = centro;
        break;
    case 'C':
        cresce_cela(n, c, l);
        break;
    case 'V':
        if (nivel_celula(n, c, l - 1) == 'C' || l <= 2) {
            n->cela_precursor = centro;
            cresce_cela(n, c, l);
        } else {
            n->duto = centro;
        }
        break;
    case '1':
        n->painel1 = centro;
        break;
    case '2': // encostado na parede norte da célula, virado para dentro do Arquivo (como o painel 1)
        n->painel2 = (Vector3){centro.x, 0.0f, centro.z - TAMANHO_CELULA * 0.5f + 0.28f};
        break;
    case 'N': // a mesa encosta na parede oeste da célula, de frente para a porta do Arquivo
        n->diario = (Vector3){centro.x - TAMANHO_CELULA * 0.5f + MESA_FUNDO_M * 0.5f + 0.05f, 0.0f, centro.z};
        break;
    case 'D':
        if (nivel_celula(n, c - 1, l) == 'O') n->porta_sala = centro;
        break;
    case 'L':
        registra_armario(n, c, l);
        break;
    case 'G':
        if (n->vidro_x1 <= 0.0f) n->vidro_x0 = c * TAMANHO_CELULA;
        n->vidro_x1 = (c + 1) * TAMANHO_CELULA;
        n->vidro_z = centro.z;
        break;
    default:
        break;
    }
}

// A luz em `de` (no plano, em metros) chega à célula (c, l)? Basta um dos 5 pontos dela (o centro e quatro
// perto dos cantos) enxergar a luz sem cruzar uma célula de parede, andando de 10 em 10 cm.
static bool luz_alcanca(const Nivel *n, Vector2 de, int c, int l) {
    static const float PONTOS[5][2] = {{0.5f, 0.5f}, {0.1f, 0.1f}, {0.9f, 0.1f}, {0.1f, 0.9f}, {0.9f, 0.9f}};
    for (int k = 0; k < 5; k++) { // padrão FOLD: algum ponto da célula vê a luz?
        Vector2 alvo = {(c + PONTOS[k][0]) * TAMANHO_CELULA, (l + PONTOS[k][1]) * TAMANHO_CELULA};
        int passos = (int)(Vector2Distance(de, alvo) / 0.1f) + 1;
        bool livre = true;
        for (int s = 1; s < passos && livre; s++) { // padrão FOLD: o caminho cruza alguma parede?
            Vector2 p = Vector2Lerp(de, alvo, (float)s / (float)passos);
            int pc = (int)floorf(p.x / TAMANHO_CELULA), pl = (int)floorf(p.y / TAMANHO_CELULA);
            livre = nivel_celula(n, pc, pl) != '#';
        }
        if (livre) return true;
    }
    return false;
}

// O mapa de alcance: para cada célula, os bits das luzes que chegam nela (calculado uma vez, ao carregar).
static void calcula_alcance_luz(Nivel *n) {
    for (int l = 0; l < MAPA_ALTURA; l++)       // padrão MAP: cada célula → os bits das luzes que a alcançam
        for (int c = 0; c < MAPA_LARGURA; c++) {
            unsigned short bits = 0;
            for (int i = 0; i < n->n_lampadas && i < LUZ_BIT_MESA; i++)
                if (luz_alcanca(n, (Vector2){n->lampadas[i].x, n->lampadas[i].z}, c, l))
                    bits |= (unsigned short)(1u << i);
            if (luz_alcanca(n, (Vector2){n->diario.x, n->diario.z}, c, l))
                bits |= (unsigned short)(1u << LUZ_BIT_MESA);
            n->alcance_luz[l][c] = bits;
        }
}

void nivel_carrega(Nivel *n) {
    memset(n, 0, sizeof(*n));
    n->cela_min = (Vector3){1e9f, 0.0f, 1e9f};
    n->cela_max = (Vector3){-1e9f, PE_DIREITO, -1e9f};
    for (int l = 0; l < MAPA_ALTURA; l++)
        memcpy(n->celulas[l], MAPA[l], MAPA_LARGURA + 1);
    for (int l = 0; l < MAPA_ALTURA; l++)
        for (int c = 0; c < MAPA_LARGURA; c++)
            registra_celula(n, n->celulas[l][c], c, l);
    if (n->vidro_z > 0.0f) n->cela_max.z = n->vidro_z - 0.2f; // a névoa vai até o vidro
    int total = (int)(sizeof(LAMPADAS) / sizeof(LAMPADAS[0]));
    for (int i = 0; i < total && n->n_lampadas < MAX_LAMPADAS; i++) {
        int c = LAMPADAS[i][0], l = LAMPADAS[i][1];
        Vector3 p = nivel_centro_celula(c, l);
        p.y = PE_DIREITO - LUMINARIA_LUZ_ABAIXO_M; // a luz logo abaixo do painel do teto
        if (nivel_na_sala_de_observacao(n, p)) p = lampada_sala_bulbo(); // a sala tem a lâmpada pendurada
        // a luminária fica no sentido do corredor: ao longo de x se dá para andar para os lados
        n->lampada_em_x[n->n_lampadas] = nivel_caminhavel(n, c - 1, l) || nivel_caminhavel(n, c + 1, l);
        n->lampadas[n->n_lampadas++] = p;
    }
    calcula_alcance_luz(n);
}

// ================================================================== paredes e interativos
static Parede caixa(Vector3 centro, Vector3 tamanho, TipoParede tipo) {
    Vector3 meio = Vector3Scale(tamanho, 0.5f);
    return (Parede){Vector3Subtract(centro, meio), Vector3Add(centro, meio), tipo};
}

// Cada corrida horizontal de '#' vira uma caixa só (menos caixas = colisão e desenho mais leves).
static void constroi_muros(const Nivel *n, ColParedes *paredes) {
    for (int l = 0; l < MAPA_ALTURA; l++) {
        int c = 0;
        while (c < MAPA_LARGURA) {
            if (n->celulas[l][c] != '#') {
                c++;
                continue;
            }
            int inicio = c;
            while (c < MAPA_LARGURA && n->celulas[l][c] == '#')
                c++;
            Vector3 min = {inicio * TAMANHO_CELULA, 0.0f, l * TAMANHO_CELULA};
            Vector3 max = {c * TAMANHO_CELULA, PE_DIREITO, (l + 1) * TAMANHO_CELULA};
            col_paredes_adiciona(paredes, (Parede){min, max, PAREDE_MURO});
        }
    }
}

void nivel_constroi_paredes(const Nivel *n, ColParedes *paredes) {
    paredes->n = 0;
    constroi_muros(n, paredes);
    for (int lado = -1; lado <= 1;
         lado += 2) // os pilares e o batente da porta da sala: passa-se só pelo meio
        col_paredes_adiciona(paredes, caixa((Vector3){n->porta_sala.x, 1.1f, n->porta_sala.z + lado * 0.775f},
                                            (Vector3){0.34f, 2.2f, 0.45f}, PAREDE_MOVEL));
    float meio_x = (n->vidro_x0 + n->vidro_x1) * 0.5f, largura = n->vidro_x1 - n->vidro_x0;
    col_paredes_adiciona(
        paredes, caixa((Vector3){meio_x, 1.5f, n->vidro_z}, (Vector3){largura, 3.0f, 0.4f}, PAREDE_VIDRO));
    col_paredes_adiciona(paredes, caixa((Vector3){n->console.x, 0.5f, n->console.z - 0.6f},
                                        (Vector3){1.6f, 1.0f, 0.8f}, PAREDE_MOVEL));
    col_paredes_adiciona(paredes, caixa((Vector3){n->painel1.x, 0.9f, n->painel1.z},
                                        (Vector3){1.2f, 1.8f, 0.5f}, PAREDE_MOVEL));
    col_paredes_adiciona(paredes, caixa((Vector3){n->painel2.x, 0.9f, n->painel2.z},
                                        (Vector3){1.2f, 1.8f, 0.5f}, PAREDE_MOVEL));
    col_paredes_adiciona(paredes,
                         caixa((Vector3){n->diario.x, MESA_ALTURA_M * 0.5f, n->diario.z},
                               (Vector3){MESA_FUNDO_M, MESA_ALTURA_M, MESA_COMPRIMENTO_M}, PAREDE_MOVEL));
    caixas_adiciona_paredes(paredes);
    for (int i = 0; i < n->n_armarios; i++) {
        Vector3 d = Vector3Subtract(n->armarios[i], n->armario_saida[i]);
        Vector3 tam = fabsf(d.x) > fabsf(d.z) ? (Vector3){0.75f, 2.1f, 1.3f} : (Vector3){1.3f, 2.1f, 0.75f};
        col_paredes_adiciona(paredes,
                             caixa((Vector3){n->armarios[i].x, 1.05f, n->armarios[i].z}, tam, PAREDE_MOVEL));
    }
}

void nivel_tranca_porta(const Nivel *n, ColParedes *paredes) {
    Vector3 p = n->porta_sala;
    col_paredes_adiciona(paredes,
                         caixa((Vector3){p.x, 1.5f, p.z}, (Vector3){0.2f, 3.0f, 2.0f}, PAREDE_PORTA));
}

static Interativo interativo(TipoInterativo tipo, Vector3 pos, Vector3 meio, Vector3 frente) {
    return (Interativo){tipo, pos, meio, frente};
}

void nivel_constroi_interativos(const Nivel *n, ColInterativos *c) {
    c->n = 0;
    Vector3 con = n->console;
    col_interativos_adiciona(c, interativo(INTER_CONSOLE, (Vector3){con.x, 1.05f, con.z - 0.6f},
                                           (Vector3){0.8f, 0.3f, 0.45f}, (Vector3){0, 0, 1}));
    col_interativos_adiciona(c, interativo(INTER_PAINEL1, (Vector3){n->painel1.x, 1.2f, n->painel1.z},
                                           (Vector3){0.65f, 0.7f, 0.3f}, (Vector3){0, 0, 1}));
    col_interativos_adiciona(c, interativo(INTER_PAINEL2, (Vector3){n->painel2.x, 1.2f, n->painel2.z},
                                           (Vector3){0.65f, 0.7f, 0.3f}, (Vector3){0, 0, 1}));
    col_interativos_adiciona(c, interativo(INTER_DIARIO,
                                           (Vector3){n->diario.x, MESA_ALTURA_M + 0.05f, n->diario.z},
                                           (Vector3){0.45f, 0.12f, 0.6f}, (Vector3){1, 0, 0}));
    for (int i = 0; i < n->n_armarios; i++) {
        Vector3 fora = Vector3Normalize(Vector3Subtract(n->armario_saida[i], n->armarios[i]));
        Vector3 pos = {n->armarios[i].x, 1.05f, n->armarios[i].z};
        col_interativos_adiciona(c, interativo(INTER_ARMARIO, pos, (Vector3){0.6f, 1.0f, 0.6f}, fora));
    }
    Vector3 porta = {n->porta_sala.x, 1.1f, n->porta_sala.z};
    col_interativos_adiciona(
        c, interativo(INTER_PORTA, porta, (Vector3){0.2f, 1.1f, 1.0f}, (Vector3){-1, 0, 0}));
}

// ================================================================== luzes
static Vector3 cor_luz(Color c, float intensidade) {
    return (Vector3){c.r / 255.0f * intensidade, c.g / 255.0f * intensidade, c.b / 255.0f * intensidade};
}

// O brilho da luminária `i` agora (0..1): a maioria fica firme; umas poucas (sorteadas pelo índice) falham,
// com apagões curtos sorteados a cada 80 ms e, de vez em quando, uma queda mais longa. Sem rand(): repetível.
float nivel_luminaria_brilho(const Estado *e, int i) {
    if (regras_sorteio(i, 0x1a3u) > LUMINARIAS_FALHANDO) return 1.0f;
    float t = e->tempo_total;
    int ciclo = (int)(t / 0.08f), longo = (int)(t / 2.5f);
    if (regras_sorteio(longo, 0x2b7u + (unsigned)i) < 0.18f && fmodf(t, 2.5f) < 0.6f) // uma queda longa
        return regras_sorteio(ciclo, 0x3c1u + (unsigned)i) < 0.5f ? 0.05f : 0.35f;
    return regras_sorteio(ciclo, 0x4d9u + (unsigned)i) < 0.06f ? 0.1f : 1.0f;
}

// Intensidade da lâmpada da sala: apagão, falha depois de um erro e alarme.
static float brilho_sala(const Estado *e) {
    if (e->apagao_t > 0.0f) return 0.0f;
    if (e->falha_luz_t > 0.0f) return (sinf(e->tempo_total * 47.0f) > 0.2f) ? 0.15f : 1.0f;
    return 1.0f;
}

// Holofotes A e B: entre o Precursor (z = 5) e o vidro (z = 7), rente ao teto, um de cada lado dele.
// Antes ficavam atrás dele e só iluminavam as costas; daqui a luz cai no rosto que o jogador vê.
#define HOLOFOTE_A (Vector3){6.5f, PE_DIREITO - 0.2f, 6.4f}
#define HOLOFOTE_B (Vector3){11.5f, PE_DIREITO - 0.2f, 6.4f}

static Vector4 holofote(Vector3 de, Vector3 alvo) {
    Vector3 d = Vector3Normalize(Vector3Subtract(alvo, de));
    return (Vector4){d.x, d.y, d.z, cosf(HOLOFOTE_ABERTURA_GRAUS * DEG2RAD)};
}

static int luzes_sala(const Estado *e, Vector3 *pos, Vector3 *cor, Vector4 *cone) {
    bool menu = e->fase == FASE_MENU || e->fase == FASE_INTRO;
    bool luz_a = e->fase == FASE_MENU || (!menu && e->rod.luz_a); // na intro, a cela fica no escuro
    bool luz_b = menu ? false : e->rod.luz_b;
    float cela = (menu ? 0.9f : 2.2f) * HOLOFOTE_GANHO * (e->apagao_t > 0.0f ? 0.0f : 1.0f);
    float sala = brilho_sala(e) * (menu ? 0.7f : 1.5f);
    Vector3 peito = Vector3Add(e->nivel.cela_precursor, (Vector3){0.0f, 1.5f, 0.0f});
    pos[0] = lampada_sala_bulbo();
    cor[0] = e->alarme_t > 0.0f ? cor_luz(COR_ALARME, 0.6f + 0.5f * sinf(e->tempo_total * 6.0f))
                                : cor_luz(COR_LAMPADA, sala);
    pos[1] = HOLOFOTE_A;
    cor[1] = cor_luz(COR_VIDRO, luz_a ? cela : 0.0f);
    cone[1] = holofote(pos[1], peito);
    pos[2] = HOLOFOTE_B;
    cor[2] = cor_luz(COR_VIDRO, luz_b ? cela : 0.0f);
    cone[2] = holofote(pos[2], peito);
    pos[3] = (Vector3){e->nivel.console.x, 1.4f, e->nivel.console.z - 0.4f};
    cor[3] = cor_luz(COR_FOSFORO, e->jog.modo == MODO_TERMINAL ? 0.35f : 0.18f);
    return 4;
}

// Nas saídas: as LUMINARIAS_ACESAS luminárias mais próximas do jogador (frias, cada uma com a sua falha) e
// uma luz bem fraca no próprio jogador. Sem energia, as luminárias ficam APAGADAS (cor zero): sobram a
// lanterna e esse resto de luz no jogador. A lâmpada da mesa do bilhete (a pilha dela) entra como uma delas.
static int luzes_saida(const Estado *e, Vector3 *pos, Vector3 *cor, int *ids) {
    const Nivel *n = &e->nivel;
    Vector3 candidatos[MAX_LAMPADAS + 1];
    int indice[MAX_LAMPADAS + 1];
    int total = 0;
    for (int i = 0; i < n->n_lampadas; i++) { // padrão MAP: cada lâmpada vira uma candidata (com o índice)
        indice[total] = i;
        candidatos[total++] = n->lampadas[i];
    }
    indice[total] = -1; // a mesa
    candidatos[total++] = (Vector3){n->diario.x, MESA_LUZ_Y, n->diario.z};
    bool energia = e->painel.resolvido;
    float corte = e->apagao_t > 0.0f ? 0.08f : 1.0f; // apagão: as lâmpadas quase somem
    int k_max = total < LUMINARIAS_ACESAS ? total : LUMINARIAS_ACESAS;
    for (int k = 0; k < k_max; k++) { // ordenação por seleção: as k_max mais próximas, da mais perto à longe
        int melhor = k;
        for (int i = k + 1; i < total; i++)
            if (Vector3Distance(candidatos[i], e->jog.pos) < Vector3Distance(candidatos[melhor], e->jog.pos))
                melhor = i;
        Vector3 troca = candidatos[k];
        candidatos[k] = candidatos[melhor];
        candidatos[melhor] = troca;
        int t = indice[k];
        indice[k] = indice[melhor];
        indice[melhor] = t;
        pos[k] = candidatos[k];
        ids[k] = indice[k] >= 0 ? indice[k] : LUZ_BIT_MESA;
        float g = LUZ_CORREDOR_GANHO;
        float falha = indice[k] >= 0 ? nivel_luminaria_brilho(e, indice[k]) : 1.0f;
        cor[k] = indice[k] < 0 ? cor_luz(MESA_COR_LUZ, MESA_LUZ_FORCA * g * corte * mesa_bilhete_piscada(e))
                 : energia     ? cor_luz(COR_LUMINARIA, LUMINARIA_FORCA * g * falha)
                               : (Vector3){0.0f, 0.0f, 0.0f}; // sem energia: apagada
    }
    // A lâmpada da mesa precisa acender de longe: de dentro do Arquivo (ou da porta) ela entra no lugar da
    // última escolhida, mesmo que outras estejam mais perto. É o que chama o olho para a mesa.
    Vector3 mesa = {n->diario.x, MESA_LUZ_Y, n->diario.z};
    bool escolhida = false;
    for (int k = 0; k < k_max; k++) // padrão FOLD: a lâmpada da mesa já está entre as escolhidas?
        escolhida = escolhida || (pos[k].x == mesa.x && pos[k].z == mesa.z);
    if (!escolhida && Vector3Distance(mesa, e->jog.pos) < MESA_LUZ_ALCANCE_M) {
        pos[k_max - 1] = mesa;
        ids[k_max - 1] = LUZ_BIT_MESA;
        cor[k_max - 1] =
            cor_luz(MESA_COR_LUZ, MESA_LUZ_FORCA * LUZ_CORREDOR_GANHO * corte * mesa_bilhete_piscada(e));
    }
    pos[k_max] = (Vector3){e->jog.pos.x, ALTURA_OLHOS + 0.3f, e->jog.pos.z};
    cor[k_max] = cor_luz(COR_LUMINARIA, energia ? 0.32f : SEM_ENERGIA_JOGADOR);
    return k_max + 1;
}

int nivel_luzes(const Estado *e, Vector3 *posicoes, Vector3 *cores, Vector4 *cones, int *ids) {
    for (int i = 0; i < LUZES_SHADER; i++) { // padrão MAP: luz pontual que chega em todo lugar
        cones[i] = (Vector4){0.0f, -1.0f, 0.0f, -2.0f};
        ids[i] = -1;
    }
    if (regras_fase_eh_saida(e->fase)) return luzes_saida(e, posicoes, cores, ids);
    return luzes_sala(e, posicoes, cores, cones);
}

// ================================================================== atualiza
// Cruzar uma porta faz barulho (nas saídas, a criatura pode ouvir).
// Poeira: caindo do teto no apagão e, no resto do tempo, flutuando devagar na luz perto do jogador.
static void solta_poeira(Estado *e, float dt) {
    if (e->apagao_t <= 0.0f && e->particulas.n < 60 && col_sorteia_por_segundo(7.0f, dt)) {
        Vector3 p = {e->jog.pos.x + GetRandomValue(-250, 250) / 100.0f, GetRandomValue(30, 270) / 100.0f,
                     e->jog.pos.z + GetRandomValue(-250, 250) / 100.0f};
        Vector3 v = {GetRandomValue(-5, 5) / 100.0f, GetRandomValue(-3, 3) / 100.0f,
                     GetRandomValue(-5, 5) / 100.0f};
        col_particulas_adiciona(&e->particulas, (Particula){p, v, 6.0f, 6.0f, PARTICULA_POEIRA});
    }
    if (e->apagao_t <= 0.0f || !col_sorteia_por_segundo(25.0f, dt)) return;
    Vector3 p = {e->jog.pos.x + GetRandomValue(-300, 300) / 100.0f, PE_DIREITO - 0.05f,
                 e->jog.pos.z + GetRandomValue(-300, 300) / 100.0f};
    Vector3 v = {GetRandomValue(-10, 10) / 100.0f, -GetRandomValue(20, 60) / 100.0f,
                 GetRandomValue(-10, 10) / 100.0f};
    col_particulas_adiciona(&e->particulas, (Particula){p, v, 4.0f, 4.0f, PARTICULA_POEIRA});
}

void nivel_atualiza(Estado *e, float dt) {
    solta_poeira(e, dt);
    int c, l;
    if (!nivel_celula_de(e->jog.pos, &c, &l)) return;
    int celula = l * MAPA_LARGURA + c;
    if (celula == e->jog.ultima_celula) return;
    e->jog.ultima_celula = celula;
    if (regras_fase_eh_saida(e->fase) && nivel_celula(&e->nivel, c, l) == 'D') {
        Vector3 p = nivel_centro_celula(c, l);
        col_ruidos_adiciona(&e->ruidos, (Ruido){p, RUIDO_RAIO_PORTA, 1.0f, RUIDO_PORTA});
    }
}

// ================================================================== desenho
static Vector3 centro_de(const Parede *p) {
    return Vector3Scale(Vector3Add(p->min, p->max), 0.5f);
}

static Vector3 tamanho_de(const Parede *p) {
    return Vector3Subtract(p->max, p->min);
}

// Metal pintado: a textura é cinza neutro e a cor vem do objeto.
static void metal(const Estado *e, Vector3 centro, Vector3 tamanho, Color cor, int variacao) {
    texturas_caixa_mat(e, MATERIAL_METAL, centro, tamanho, texturas_modo(MATERIAL_METAL, variacao),
                   cor);
}

// `completo`: false nos passes de sombra e de reflexo, que só levam o que pesa no resultado (os armários e as
// peças grandes); os modelos pequenos (câmeras, quadro, console, caixotes, a mesa) ficam só na cena.
static void desenha_moveis(const Estado *e, bool completo) {
    const Nivel *n = &e->nivel;
    int armario_oculto = e->jog.modo == MODO_ARMARIO ? e->jog.armario : -1; // a câmera está dentro dele
    Color cinza = {0x22, 0x26, 0x24, 255};
    Vector3 con = n->console;
    metal(e, (Vector3){con.x, 0.5f, con.z - 0.6f}, (Vector3){1.6f, 1.0f, 0.8f}, COR_OLIVA, 1);
    metal(e, (Vector3){con.x, 1.02f, con.z - 0.75f}, (Vector3){1.5f, 0.06f, 0.5f}, cinza, 2);
    if (completo) {
        caixas_desenha(e);
        cameras_desenha(e);
        quadro_desenha(e);
        console_modelo_desenha(e); // o computador de época na mesa (sem o modelo, a tela verde abaixo)
        lampada_sala_desenha(e);   // a lâmpada pendurada da sala (fica fora do reflexo: é pequena)
    }
    metal(e, (Vector3){con.x + 0.62f, 1.18f, con.z - 0.88f}, (Vector3){0.22f, 0.3f, 0.1f}, cinza,
          3); // interfone
    painel_modelo_desenha(e, n->painel1, 4); // os quadros elétricos dos geradores (painel_modelo.c)
    painel_modelo_desenha(e, n->painel2, 5);
    if (completo) mesa_bilhete_desenha(e);    // a mesa do bilhete, a lâmpada pendurada e as folhas
    for (int i = 0; i < n->n_armarios; i++) { // laço de desenho: só percorre os armários
        Vector3 d = Vector3Subtract(n->armarios[i], n->armario_saida[i]);
        bool em_x = fabsf(d.x) > fabsf(d.z);
        Vector3 tam = em_x ? (Vector3){0.75f, 2.1f, 1.3f} : (Vector3){1.3f, 2.1f, 0.75f};
        Vector3 c = {n->armarios[i].x, 1.05f, n->armarios[i].z};
        if (i == armario_oculto) continue; // escondido: a camera esta dentro, nao desenha o armario
        if (armario_modelo_desenha(e, c, tam, Vector3Negate(Vector3Normalize(d))))
            continue; // o modelo já tem a porta com venezianas: as frestas desenhadas são só da caixa
        metal(e, c, tam, (Color){0x3b, 0x40, 0x31, 255}, 10 + i); // sem o modelo: a caixa de metal
        Vector3 frente = Vector3Scale(Vector3Normalize(d), -(em_x ? tam.x : tam.z) * 0.5f - 0.005f);
        for (int f = 0; f < 4; f++) { // frestas
            Vector3 p = Vector3Add(c, frente);
            p.y = 1.35f + f * 0.12f;
            DrawCubeV(p, em_x ? (Vector3){0.02f, 0.03f, 0.9f} : (Vector3){0.9f, 0.03f, 0.02f}, BLACK);
        }
    }
}

// A porta da sala de observação: a de bunker (porta_bunker.c), com dois pilares de concreto fechando o vão
// de 2 m em volta do batente; sem o modelo, a porta de correr de antes, que some na parede nas saídas.
static void desenha_porta_da_sala(const Estado *e) {
    Vector3 p = e->nivel.porta_sala;
    for (int lado = -1; lado <= 1; lado += 2) // padrão MAP: um pilar de cada lado do batente
        texturas_caixa_mat(e, MATERIAL_CONCRETO, (Vector3){p.x, 1.1f, p.z + lado * 0.83f},
                       (Vector3){0.3f, 2.2f, 0.34f}, texturas_modo(MATERIAL_CONCRETO, 7 + lado), WHITE);
    if (porta_bunker_desenha(e)) return;
    if (!regras_fase_eh_saida(e->fase)) return;
    float abre = fminf(1.0f, e->tempo_fase / 1.2f);
    texturas_caixa_mat(e, MATERIAL_PORTA, (Vector3){p.x, 1.1f, p.z + 1.4f * abre},
                   (Vector3){0.16f, 2.2f, 1.32f}, texturas_modo(MATERIAL_PORTA, 0), WHITE);
}

// A carcaça de cada luminária: uma caixa de metal rasa presa no teto, no sentido do corredor (o painel aceso,
// por baixo dela, sai em nivel_desenha_emissivos).
static void desenha_luminarias(const Estado *e) {
    const Nivel *n = &e->nivel;
    posfx_material(e, MATERIAL_METAL);
    for (int i = 0; i < n->n_lampadas; i++) { // laço de desenho: só percorre as luminárias
        if (nivel_na_sala_de_observacao(n, n->lampadas[i])) continue; // a sala tem a lâmpada pendurada
        Vector3 p = {n->lampadas[i].x, PE_DIREITO - 0.035f, n->lampadas[i].z};
        Vector3 t = n->lampada_em_x[i] ? (Vector3){1.3f, 0.07f, 0.34f} : (Vector3){0.34f, 0.07f, 1.3f};
        texturas_caixa(e->rec.material[MATERIAL_METAL], p, t, texturas_modo(MATERIAL_METAL, 60 + i),
                       (Color){0x3a, 0x40, 0x3c, 255});
    }
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
}

static void desenha_portas_e_duto(const Estado *e) {
    const Nivel *n = &e->nivel;
    for (int l = 0; l < MAPA_ALTURA; l++) {
        for (int c = 0; c < MAPA_LARGURA; c++) {
            if (n->celulas[l][c] != 'D') continue;
            Vector3 p = nivel_centro_celula(c, l);
            texturas_caixa_mat(e, MATERIAL_CONCRETO, (Vector3){p.x, 2.6f, p.z},
                           (Vector3){2.0f, 0.8f, 2.0f}, texturas_modo(MATERIAL_CONCRETO, c + l),
                           WHITE); // verga
        }
    }
    Vector3 d = n->duto;
    DrawCubeV((Vector3){d.x, 0.01f, d.z}, (Vector3){1.2f, 0.02f, 1.2f}, (Color){0x08, 0x09, 0x08, 255});
    for (int i = 0; i < 5; i++)
        DrawCubeV((Vector3){d.x - 0.5f + i * 0.25f, 0.03f, d.z}, (Vector3){0.05f, 0.03f, 1.2f},
                  (Color){0x30, 0x34, 0x30, 255});
}

// Paredes em passadas por material (uma troca de material por passada, não uma por parede): azulejo até
// BARRADO_ALTURA, com o arremate no topo e o rodapé, e o reboco pintado descascando. O azulejo vai primeiro:
// ele fica na frente do reboco, e o teste de profundidade descarta a parte escondida antes de sombrear.
// Nenhuma peça ocupa o mesmo lugar que outra (senão as faces brigam na profundidade e a borda serrilha).
static void desenha_paredes(const Estado *e) {
    const ColParedes *ps = &e->paredes;
    Texture2D concreto = e->rec.material[MATERIAL_CONCRETO], azulejo = e->rec.material[MATERIAL_AZULEJO];
    float barrado = BARRADO_ALTURA - RODAPE_ALTURA;
    posfx_material(e, MATERIAL_AZULEJO);
    // laço de desenho: o azulejo de cada muro, o arremate no topo dele e o rodapé (peças mais escuras)
    for (int i = 0; i < ps->n; i++) {
        if (ps->itens[i].tipo != PAREDE_MURO) continue;
        Vector3 c = centro_de(&ps->itens[i]), t = tamanho_de(&ps->itens[i]);
        ModoUV uv = texturas_modo(MATERIAL_AZULEJO, i);
        texturas_caixa(azulejo, (Vector3){c.x, RODAPE_ALTURA + barrado * 0.5f, c.z},
                       (Vector3){t.x + 0.02f, barrado, t.z + 0.02f}, uv, WHITE);
        // o arremate: uma fiada fina, mais saliente, com a junta vertical das peças e sem junta horizontal
        // (a UV vertical cobre 60 m: os 5 cm dele caem no meio de uma peça da textura)
        ModoUV arremate = {{uv.metros.x, 60.0f}, {uv.desloca.x, 0.0f}, false};
        texturas_caixa(azulejo, (Vector3){c.x, BARRADO_ALTURA + 0.025f, c.z},
                       (Vector3){t.x + 0.04f, 0.05f, t.z + 0.04f}, arremate, (Color){0xd0, 0xd8, 0xd2, 255});
        texturas_caixa(azulejo, (Vector3){c.x, RODAPE_ALTURA * 0.5f, c.z},
                       (Vector3){t.x + 0.03f, RODAPE_ALTURA, t.z + 0.03f}, uv,
                       (Color){0x70, 0x78, 0x74, 255});
    }
    posfx_material(e, MATERIAL_CONCRETO);
    // laço de desenho: só percorre a coleção para desenhar (não é map/filter/fold)
    for (int i = 0; i < ps->n; i++)
        if (ps->itens[i].tipo == PAREDE_MURO)
            texturas_caixa(concreto, centro_de(&ps->itens[i]), tamanho_de(&ps->itens[i]),
                           texturas_modo(MATERIAL_CONCRETO, i), WHITE);
    posfx_material(e, MATERIAL_PORTA);
    // laço de desenho: a porta trancada da sala
    for (int i = 0; i < ps->n; i++)
        if (ps->itens[i].tipo == PAREDE_PORTA)
            texturas_caixa(e->rec.material[MATERIAL_PORTA], centro_de(&ps->itens[i]),
                           tamanho_de(&ps->itens[i]), texturas_modo(MATERIAL_PORTA, 0), WHITE);
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
}

// `com_chao`: false no passe espelhado (o chão é o espelho: ele não entra no próprio reflexo).
static void desenha(const Estado *e, bool com_chao) {
    const Nivel *n = &e->nivel;
    float largura = MAPA_LARGURA * TAMANHO_CELULA, profundidade = MAPA_ALTURA * TAMANHO_CELULA;
    posfx_chao(e, true);
    if (com_chao)
        texturas_caixa_mat(e, MATERIAL_PISO, (Vector3){largura * 0.5f, -0.05f, profundidade * 0.5f},
                           (Vector3){largura, 0.1f, profundidade}, texturas_modo(MATERIAL_PISO, 0), WHITE);
    posfx_chao(e, false);
    texturas_caixa_mat(e, MATERIAL_TETO,
                   (Vector3){largura * 0.5f, PE_DIREITO + 0.05f, profundidade * 0.5f},
                   (Vector3){largura, 0.1f, profundidade}, texturas_modo(MATERIAL_TETO, 0), WHITE);
    desenha_paredes(e);
    float meio_x = (n->vidro_x0 + n->vidro_x1) * 0.5f, vl = n->vidro_x1 - n->vidro_x0;
    texturas_caixa_mat(e, MATERIAL_CONCRETO, (Vector3){meio_x, 0.45f, n->vidro_z},
                   (Vector3){vl, 0.9f, 0.4f}, texturas_modo(MATERIAL_CONCRETO, 99), WHITE); // mureta
    for (int i = 0; i <= 3; i++) // montantes do vidro
        metal(e, (Vector3){n->vidro_x0 + i * vl / 3.0f, 1.95f, n->vidro_z}, (Vector3){0.12f, 2.1f, 0.14f},
              (Color){0x1c, 0x20, 0x1e, 255}, 20 + i);
    desenha_moveis(e, com_chao);
    desenha_luminarias(e);
    desenha_portas_e_duto(e);
    desenha_porta_da_sala(e);
}

void nivel_desenha(const Estado *e) {
    desenha(e, true);
}

void nivel_desenha_reflexo(const Estado *e) {
    desenha(e, false);
}

static Color cor_lampada(const Estado *e) {
    if (regras_fase_eh_saida(e->fase)) // sem energia, a luminária apagada
        return e->painel.resolvido ? COR_LUMINARIA : (Color){0x18, 0x1c, 0x1a, 255};
    return COR_LAMPADA;
}

// Fagulhas dos glifos e poeira do apagão (a coleção de partículas).
static void desenha_particulas(const Estado *e) {
    // laço de desenho: só percorre a coleção para desenhar (não é map/filter/fold)
    for (int i = 0; i < e->particulas.n; i++) {
        const Particula *p = &e->particulas.itens[i];
        float vida = p->vida / p->vida_max;
        float brilho = fminf(1.0f, vida * 4.0f) * fminf(1.0f, (1.0f - vida) * 6.0f); // aparece e some suave
        Color cor = p->tipo == PARTICULA_FAGULHA  ? Fade(COR_FOSFORO, vida)
                    : p->tipo == PARTICULA_POEIRA ? Fade((Color){170, 160, 140, 255}, 0.35f * brilho)
                                                  : Fade(COR_PELE, vida);
        float lado = p->tipo == PARTICULA_POEIRA ? 0.012f : 0.025f;
        DrawCubeV(p->pos, (Vector3){lado, lado, lado}, cor);
    }
}

void nivel_desenha_emissivos(const Estado *e) {
    const Nivel *n = &e->nivel;
    desenha_particulas(e);
    bool saida = regras_fase_eh_saida(e->fase);
    Color lamp = cor_lampada(e);
    if (saida) {
        bool energia = e->painel.resolvido;
        float corte = e->apagao_t > 0.0f ? 0.08f : 1.0f;
        posfx_comeca_emissivo(e);
        for (int i = 0; i < n->n_lampadas; i++) { // laço de desenho: o painel aceso de cada luminária
            float k = energia ? LUMINARIA_EMISSAO * corte * nivel_luminaria_brilho(e, i) : 0.0f; // apagada
            posfx_emissivo(e, k);
            if (nivel_na_sala_de_observacao(n, n->lampadas[i])) { // a lâmpada pendurada: só o bulbo acende
                lampada_sala_desenha_bulbo(COR_LUMINARIA, energia ? 1.0f : 0.0f);
                continue;
            }
            Vector3 p = {n->lampadas[i].x, PE_DIREITO - 0.074f, n->lampadas[i].z};
            Vector3 painel = n->lampada_em_x[i] ? (Vector3){1.16f, 0.004f, 0.22f}
                                                : (Vector3){0.22f, 0.004f, 1.16f};
            DrawCubeV(p, painel, energia ? COR_LUMINARIA : lamp);
        }
        posfx_emissivo(e, 1.0f);
        posfx_termina_emissivo(e);
        DrawSphereEx((Vector3){n->diario.x, MESA_LUZ_Y, n->diario.z}, 0.06f, 6, 6,
                     ColorBrightness(MESA_COR_LUZ, mesa_bilhete_piscada(e) - 1.0f));
    }
    float sala = brilho_sala(e);
    Vector3 lampada_sala = lampada_sala_bulbo();
    Color cor_sala = e->alarme_t > 0 ? COR_ALARME : lamp;
    if (!saida && sala > 0.5f) lampada_sala_desenha_bulbo(cor_sala, 1.0f);
    cenario_comeca_halos();
    for (int i = 0; saida && e->painel.resolvido && i < n->n_lampadas; i++) // o halo de cada lâmpada acesa
        cenario_halo(e, n->lampadas[i], lamp);
    if (!saida && sala > 0.5f) cenario_halo(e, lampada_sala, cor_sala);
    mesa_bilhete_desenha_brilho(e);
    cenario_termina_halos();
    bool menu = e->fase == FASE_MENU || e->fase == FASE_INTRO;
    bool a_acesa = e->fase == FASE_MENU || (!menu && e->rod.luz_a); // na intro, apagado
    if (a_acesa && e->apagao_t <= 0.0f) DrawSphereEx(HOLOFOTE_A, 0.1f, 8, 8, COR_VIDRO);
    if (!menu && e->rod.luz_b && e->apagao_t <= 0.0f) DrawSphereEx(HOLOFOTE_B, 0.1f, 8, 8, COR_VIDRO);
    Color tela = e->jog.modo == MODO_TERMINAL ? COR_FOSFORO : (Color){0x2a, 0x6a, 0x3a, 255};
    if (!e->rec.tem_terminal)
        DrawCubeV((Vector3){n->console.x, 1.1f, n->console.z - 0.78f}, (Vector3){0.9f, 0.14f, 0.3f}, tela);
}

void nivel_desenha_vidro(const Estado *e) {
    const Nivel *n = &e->nivel;
    float meio_x = (n->vidro_x0 + n->vidro_x1) * 0.5f, vl = n->vidro_x1 - n->vidro_x0;
    float opacidade = 0.09f + (VIDAS_VIDRO - e->vidas) * 0.05f;
    if (e->fase == FASE_FIM_RUIM)
        opacidade *= fmaxf(0.0f, 1.0f - (e->fim.tempo - 4.5f) / 1.5f); // o vidro se dissolve
    Vector3 centro = {meio_x, 1.95f, n->vidro_z};
    if (opacidade <= 0.001f) return; // dissolvido (final ruim): nem o reflexo fica
    if (!posfx_comeca_vidro(e)) { // sem o shader de luz: a caixa translúcida de antes
        DrawCubeV(centro, (Vector3){vl, 2.1f, 0.05f}, Fade(COR_VIDRO, opacidade));
        return;
    }
    // vidro de verdade (posfx: Fresnel, reflexo das luzes, sujeira): as duas faces, uma para cada lado
    unsigned char alfa = (unsigned char)(255.0f * Clamp(opacidade, 0.0f, 1.0f));
    Color tinta = {COR_VIDRO.r, COR_VIDRO.g, COR_VIDRO.b, alfa};
    Texture2D sujeira = e->rec.decalque[DECALQUE_VIDRO];
    texturas_quadro(sujeira, centro, (Vector3){vl, 0, 0}, (Vector3){0, 2.1f, 0}, tinta);
    texturas_quadro(sujeira, centro, (Vector3){-vl, 0, 0}, (Vector3){0, 2.1f, 0}, tinta);
    posfx_termina_vidro(e);
}
