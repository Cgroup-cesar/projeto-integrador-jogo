// nivel.c — o Posto 7: mapa ASCII → paredes, móveis, interativos e luzes.
// O mapa é texto para a equipe editar. 1 célula = 2 m; pé-direito 3 m.
#include "nivel.h"
#include <math.h>
#include <string.h>

// # parede · C cela · V Precursor (na cela) ou duto (no corredor) · G vidro · O sala de observação
// S início · T console · D porta · . piso · L armário · 1 painel 1 · 2 painel 2 · N diário
static const char *const MAPA[MAPA_ALTURA] = {
    "########################",
    "#CCCCCC##########.....##",
    "#CCCVCC##########..1..##",
    "#GGGGGG##########L....##",
    "#OOOTOO############D####",
    "#OOSOOOD....L.......L.##",
    "#OOOOOO#.############.##",
    "########.############.##",
    "########L############.##",
    "#......#.############.##",
    "#.N.2..D.############L##",
    "#......#......V.......##",
    "#L.....#################",
    "########################",
};

// Lâmpadas dos corredores e das salas (coluna, linha). Edite à vontade.
static const int LAMPADAS[][2] = {
    {10, 5}, {15, 5}, {20, 5}, {21, 8}, {8, 7}, {8, 10}, {11, 11}, {16, 11}, {20, 11}, {19, 2}, {4, 11}, {3, 5},
};

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

static void registra_celula(Nivel *n, char ch, int c, int l) {
    Vector3 centro = nivel_centro_celula(c, l);
    switch (ch) {
    case 'S': n->inicio = centro; break;
    case 'T': n->console = centro; break;
    case 'V':
        if (nivel_celula(n, c, l - 1) == 'C' || l <= 2) n->cela_precursor = centro;
        else n->duto = centro;
        break;
    case '1': n->painel1 = centro; break;
    case '2': n->painel2 = centro; break;
    case 'N': n->diario = centro; break;
    case 'D':
        if (nivel_celula(n, c - 1, l) == 'O') n->porta_sala = centro;
        break;
    case 'L': registra_armario(n, c, l); break;
    case 'G':
        if (n->vidro_x1 <= 0.0f) n->vidro_x0 = c * TAMANHO_CELULA;
        n->vidro_x1 = (c + 1) * TAMANHO_CELULA;
        n->vidro_z = centro.z;
        break;
    default: break;
    }
}

void nivel_carrega(Nivel *n) {
    memset(n, 0, sizeof(*n));
    for (int l = 0; l < MAPA_ALTURA; l++) memcpy(n->celulas[l], MAPA[l], MAPA_LARGURA + 1);
    for (int l = 0; l < MAPA_ALTURA; l++)
        for (int c = 0; c < MAPA_LARGURA; c++) registra_celula(n, n->celulas[l][c], c, l);
    int total = (int)(sizeof(LAMPADAS) / sizeof(LAMPADAS[0]));
    for (int i = 0; i < total && n->n_lampadas < MAX_LAMPADAS; i++) {
        Vector3 p = nivel_centro_celula(LAMPADAS[i][0], LAMPADAS[i][1]);
        p.y = PE_DIREITO - 0.25f;
        n->lampadas[n->n_lampadas++] = p;
    }
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
            while (c < MAPA_LARGURA && n->celulas[l][c] == '#') c++;
            Vector3 min = {inicio * TAMANHO_CELULA, 0.0f, l * TAMANHO_CELULA};
            Vector3 max = {c * TAMANHO_CELULA, PE_DIREITO, (l + 1) * TAMANHO_CELULA};
            col_paredes_adiciona(paredes, (Parede){min, max, PAREDE_MURO});
        }
    }
}

void nivel_constroi_paredes(const Nivel *n, ColParedes *paredes) {
    paredes->n = 0;
    constroi_muros(n, paredes);
    float meio_x = (n->vidro_x0 + n->vidro_x1) * 0.5f, largura = n->vidro_x1 - n->vidro_x0;
    col_paredes_adiciona(paredes, caixa((Vector3){meio_x, 1.5f, n->vidro_z}, (Vector3){largura, 3.0f, 0.4f}, PAREDE_VIDRO));
    col_paredes_adiciona(paredes, caixa((Vector3){n->console.x, 0.5f, n->console.z - 0.6f}, (Vector3){1.6f, 1.0f, 0.8f}, PAREDE_MOVEL));
    col_paredes_adiciona(paredes, caixa((Vector3){n->painel1.x, 0.9f, n->painel1.z}, (Vector3){1.2f, 1.8f, 0.5f}, PAREDE_MOVEL));
    col_paredes_adiciona(paredes, caixa((Vector3){n->painel2.x, 0.9f, n->painel2.z}, (Vector3){0.5f, 1.8f, 1.2f}, PAREDE_MOVEL));
    col_paredes_adiciona(paredes, caixa((Vector3){n->diario.x, 0.4f, n->diario.z}, (Vector3){0.9f, 0.8f, 1.4f}, PAREDE_MOVEL));
    for (int i = 0; i < n->n_armarios; i++) {
        Vector3 d = Vector3Subtract(n->armarios[i], n->armario_saida[i]);
        Vector3 tam = fabsf(d.x) > fabsf(d.z) ? (Vector3){0.75f, 2.1f, 1.3f} : (Vector3){1.3f, 2.1f, 0.75f};
        col_paredes_adiciona(paredes, caixa((Vector3){n->armarios[i].x, 1.05f, n->armarios[i].z}, tam, PAREDE_MOVEL));
    }
}

void nivel_tranca_porta(const Nivel *n, ColParedes *paredes) {
    Vector3 p = n->porta_sala;
    col_paredes_adiciona(paredes, caixa((Vector3){p.x, 1.5f, p.z}, (Vector3){0.2f, 3.0f, 2.0f}, PAREDE_PORTA));
}

static Interativo interativo(TipoInterativo tipo, Vector3 pos, Vector3 meio, Vector3 frente) {
    return (Interativo){tipo, pos, meio, frente};
}

void nivel_constroi_interativos(const Nivel *n, ColInterativos *c) {
    c->n = 0;
    Vector3 con = n->console;
    col_interativos_adiciona(c, interativo(INTER_CONSOLE, (Vector3){con.x, 1.05f, con.z - 0.6f}, (Vector3){0.8f, 0.3f, 0.45f}, (Vector3){0, 0, 1}));
    col_interativos_adiciona(c, interativo(INTER_PAINEL1, (Vector3){n->painel1.x, 1.2f, n->painel1.z}, (Vector3){0.65f, 0.7f, 0.3f}, (Vector3){0, 0, 1}));
    col_interativos_adiciona(c, interativo(INTER_PAINEL2, (Vector3){n->painel2.x, 1.2f, n->painel2.z}, (Vector3){0.3f, 0.7f, 0.65f}, (Vector3){1, 0, 0}));
    col_interativos_adiciona(c, interativo(INTER_DIARIO, (Vector3){n->diario.x, 0.85f, n->diario.z}, (Vector3){0.45f, 0.12f, 0.6f}, (Vector3){1, 0, 0}));
    for (int i = 0; i < n->n_armarios; i++) {
        Vector3 fora = Vector3Normalize(Vector3Subtract(n->armario_saida[i], n->armarios[i]));
        Vector3 pos = {n->armarios[i].x, 1.05f, n->armarios[i].z};
        col_interativos_adiciona(c, interativo(INTER_ARMARIO, pos, (Vector3){0.6f, 1.0f, 0.6f}, fora));
    }
    Vector3 porta = {n->porta_sala.x, 1.1f, n->porta_sala.z};
    col_interativos_adiciona(c, interativo(INTER_PORTA, porta, (Vector3){0.2f, 1.1f, 1.0f}, (Vector3){-1, 0, 0}));
}

// ================================================================== luzes
static Vector3 cor_luz(Color c, float intensidade) {
    return (Vector3){c.r / 255.0f * intensidade, c.g / 255.0f * intensidade, c.b / 255.0f * intensidade};
}

// Intensidade da lâmpada da sala: apagão, falha depois de um erro e alarme.
static float brilho_sala(const Estado *e) {
    if (e->apagao_t > 0.0f) return 0.0f;
    if (e->falha_luz_t > 0.0f) return (sinf(e->tempo_total * 47.0f) > 0.2f) ? 0.15f : 1.0f;
    return 1.0f;
}

static int luzes_sala(const Estado *e, Vector3 *pos, Vector3 *cor) {
    bool menu = e->fase == FASE_MENU || e->fase == FASE_INTRO;
    bool luz_a = menu ? true : e->rod.luz_a;
    bool luz_b = menu ? false : e->rod.luz_b;
    float cela = (menu ? 0.9f : 2.2f) * (e->apagao_t > 0.0f ? 0.0f : 1.0f);
    float sala = brilho_sala(e) * (menu ? 0.7f : 1.5f);
    pos[0] = (Vector3){8.0f, PE_DIREITO - 0.25f, 11.0f};
    cor[0] = e->alarme_t > 0.0f ? cor_luz(COR_ALARME, 0.6f + 0.5f * sinf(e->tempo_total * 6.0f)) : cor_luz(COR_LAMPADA, sala);
    pos[1] = (Vector3){5.0f, PE_DIREITO - 0.4f, 4.0f};
    cor[1] = cor_luz(COR_VIDRO, luz_a ? cela : 0.0f);
    pos[2] = (Vector3){11.0f, PE_DIREITO - 0.4f, 4.0f};
    cor[2] = cor_luz(COR_VIDRO, luz_b ? cela : 0.0f);
    pos[3] = (Vector3){e->nivel.console.x, 1.4f, e->nivel.console.z - 0.4f};
    cor[3] = cor_luz(COR_FOSFORO, e->jog.modo == MODO_TERMINAL ? 0.35f : 0.18f);
    return 4;
}

// Nas saídas: as 3 lâmpadas mais próximas do jogador (vermelhas e fracas até a energia voltar) e uma luz
// bem fraca no próprio jogador, só para ele não ficar cego no corredor.
static int luzes_saida(const Estado *e, Vector3 *pos, Vector3 *cor) {
    const Nivel *n = &e->nivel;
    Vector3 candidatos[MAX_LAMPADAS + 1];
    int total = 0;
    for (int i = 0; i < n->n_lampadas; i++) candidatos[total++] = n->lampadas[i];
    candidatos[total++] = (Vector3){n->diario.x, PE_DIREITO - 0.3f, n->diario.z};
    bool energia = e->painel.resolvido;
    float corte = e->apagao_t > 0.0f ? 0.08f : 1.0f; // apagão: as lâmpadas quase somem
    for (int k = 0; k < 3; k++) {
        int melhor = k;
        for (int i = k + 1; i < total; i++)
            if (Vector3Distance(candidatos[i], e->jog.pos) < Vector3Distance(candidatos[melhor], e->jog.pos)) melhor = i;
        Vector3 troca = candidatos[k];
        candidatos[k] = candidatos[melhor];
        candidatos[melhor] = troca;
        pos[k] = candidatos[k];
        bool diario = candidatos[k].x == n->diario.x && candidatos[k].z == n->diario.z;
        cor[k] = diario ? cor_luz(COR_LAMPADA, 1.4f * corte) : energia ? cor_luz(COR_LAMPADA, 1.5f) : cor_luz(COR_ALARME, 1.7f * corte);
    }
    pos[3] = (Vector3){e->jog.pos.x, ALTURA_OLHOS + 0.3f, e->jog.pos.z};
    cor[3] = cor_luz(COR_LAMPADA, 0.32f);
    return 4;
}

int nivel_luzes(const Estado *e, Vector3 *posicoes, Vector3 *cores) {
    if (regras_fase_eh_saida(e->fase)) return luzes_saida(e, posicoes, cores);
    return luzes_sala(e, posicoes, cores);
}

// ================================================================== atualiza
// Cruzar uma porta faz barulho (nas saídas, a criatura pode ouvir).
// Poeira: caindo do teto no apagão e, no resto do tempo, flutuando devagar na luz perto do jogador.
static void solta_poeira(Estado *e) {
    if (e->apagao_t <= 0.0f && e->particulas.n < 60 && GetRandomValue(0, 100) < 12) {
        Vector3 p = {e->jog.pos.x + GetRandomValue(-250, 250) / 100.0f, GetRandomValue(30, 270) / 100.0f, e->jog.pos.z + GetRandomValue(-250, 250) / 100.0f};
        Vector3 v = {GetRandomValue(-5, 5) / 100.0f, GetRandomValue(-3, 3) / 100.0f, GetRandomValue(-5, 5) / 100.0f};
        col_particulas_adiciona(&e->particulas, (Particula){p, v, 6.0f, 6.0f, PARTICULA_POEIRA});
    }
    if (e->apagao_t <= 0.0f || GetRandomValue(0, 100) > 40) return;
    Vector3 p = {e->jog.pos.x + GetRandomValue(-300, 300) / 100.0f, PE_DIREITO - 0.05f, e->jog.pos.z + GetRandomValue(-300, 300) / 100.0f};
    Vector3 v = {GetRandomValue(-10, 10) / 100.0f, -GetRandomValue(20, 60) / 100.0f, GetRandomValue(-10, 10) / 100.0f};
    col_particulas_adiciona(&e->particulas, (Particula){p, v, 4.0f, 4.0f, PARTICULA_POEIRA});
}

void nivel_atualiza(Estado *e, float dt) {
    (void)dt;
    solta_poeira(e);
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

static void desenha_moveis(const Nivel *n) {
    Color metal = {0x22, 0x26, 0x24, 255};
    Vector3 con = n->console;
    DrawCubeV((Vector3){con.x, 0.5f, con.z - 0.6f}, (Vector3){1.6f, 1.0f, 0.8f}, COR_OLIVA);
    DrawCubeV((Vector3){con.x, 1.02f, con.z - 0.75f}, (Vector3){1.5f, 0.06f, 0.5f}, metal);
    DrawCubeV((Vector3){con.x + 0.62f, 1.18f, con.z - 0.88f}, (Vector3){0.22f, 0.3f, 0.1f}, metal); // interfone
    DrawCubeV((Vector3){n->painel1.x, 0.9f, n->painel1.z}, (Vector3){1.2f, 1.8f, 0.5f}, COR_OLIVA);
    DrawCubeV((Vector3){n->painel2.x, 0.9f, n->painel2.z}, (Vector3){0.5f, 1.8f, 1.2f}, COR_OLIVA);
    DrawCubeV((Vector3){n->diario.x, 0.4f, n->diario.z}, (Vector3){0.9f, 0.8f, 1.4f}, (Color){0x3a, 0x2c, 0x20, 255});
    DrawCubeV((Vector3){n->diario.x, 0.81f, n->diario.z}, (Vector3){0.32f, 0.02f, 0.44f}, (Color){0xc8, 0xc0, 0xa8, 255});
    for (int i = 0; i < n->n_armarios; i++) {
        Vector3 d = Vector3Subtract(n->armarios[i], n->armario_saida[i]);
        bool em_x = fabsf(d.x) > fabsf(d.z);
        Vector3 tam = em_x ? (Vector3){0.75f, 2.1f, 1.3f} : (Vector3){1.3f, 2.1f, 0.75f};
        Vector3 c = {n->armarios[i].x, 1.05f, n->armarios[i].z};
        DrawCubeV(c, tam, (Color){0x3b, 0x40, 0x31, 255});
        Vector3 frente = Vector3Scale(Vector3Normalize(d), -(em_x ? tam.x : tam.z) * 0.5f - 0.005f);
        for (int f = 0; f < 4; f++) { // frestas
            Vector3 p = Vector3Add(c, frente);
            p.y = 1.35f + f * 0.12f;
            DrawCubeV(p, em_x ? (Vector3){0.02f, 0.03f, 0.9f} : (Vector3){0.9f, 0.03f, 0.02f}, BLACK);
        }
    }
}

// Nas saídas, a porta da sala de observação desliza para dentro da parede no primeiro segundo.
static void desenha_porta_da_sala(const Estado *e) {
    if (!regras_fase_eh_saida(e->fase)) return;
    float abre = fminf(1.0f, e->tempo_fase / 1.2f);
    Vector3 p = e->nivel.porta_sala;
    DrawCubeV((Vector3){p.x, 1.1f, p.z + 2.0f * abre}, (Vector3){0.16f, 2.2f, 2.0f}, (Color){0x35, 0x38, 0x2a, 255});
}

static void desenha_portas_e_duto(const Nivel *n) {
    for (int l = 0; l < MAPA_ALTURA; l++) {
        for (int c = 0; c < MAPA_LARGURA; c++) {
            if (n->celulas[l][c] != 'D') continue;
            Vector3 p = nivel_centro_celula(c, l);
            DrawCubeV((Vector3){p.x, 2.6f, p.z}, (Vector3){2.0f, 0.8f, 2.0f}, COR_CONCRETO); // verga
        }
    }
    Vector3 d = n->duto;
    DrawCubeV((Vector3){d.x, 0.01f, d.z}, (Vector3){1.2f, 0.02f, 1.2f}, (Color){0x08, 0x09, 0x08, 255});
    for (int i = 0; i < 5; i++)
        DrawCubeV((Vector3){d.x - 0.5f + i * 0.25f, 0.03f, d.z}, (Vector3){0.05f, 0.03f, 1.2f}, (Color){0x30, 0x34, 0x30, 255});
}

void nivel_desenha(const Estado *e) {
    const Nivel *n = &e->nivel;
    float largura = MAPA_LARGURA * TAMANHO_CELULA, profundidade = MAPA_ALTURA * TAMANHO_CELULA;
    DrawCubeV((Vector3){largura * 0.5f, -0.05f, profundidade * 0.5f}, (Vector3){largura, 0.1f, profundidade}, (Color){0x24, 0x2b, 0x26, 255});
    DrawCubeV((Vector3){largura * 0.5f, PE_DIREITO + 0.05f, profundidade * 0.5f}, (Vector3){largura, 0.1f, profundidade}, (Color){0x18, 0x1c, 0x19, 255});
    for (int i = 0; i < e->paredes.n; i++) {
        const Parede *p = &e->paredes.itens[i];
        if (p->tipo == PAREDE_PORTA) DrawCubeV(centro_de(p), tamanho_de(p), (Color){0x35, 0x38, 0x2a, 255});
        if (p->tipo != PAREDE_MURO) continue;
        Vector3 c = centro_de(p), t = tamanho_de(p);
        DrawCubeV(c, t, COR_CONCRETO);
        // Rodapé escuro e a faixa de tinta oliva a 1,1 m: dão escala e leitura às paredes no escuro.
        DrawCubeV((Vector3){c.x, 0.08f, c.z}, (Vector3){t.x + 0.02f, 0.16f, t.z + 0.02f}, (Color){0x16, 0x1c, 0x18, 255});
        DrawCubeV((Vector3){c.x, 1.15f, c.z}, (Vector3){t.x + 0.02f, 0.12f, t.z + 0.02f}, COR_OLIVA);
    }
    float meio_x = (n->vidro_x0 + n->vidro_x1) * 0.5f, vl = n->vidro_x1 - n->vidro_x0;
    DrawCubeV((Vector3){meio_x, 0.45f, n->vidro_z}, (Vector3){vl, 0.9f, 0.4f}, COR_CONCRETO); // mureta
    for (int i = 0; i <= 3; i++)
        DrawCubeV((Vector3){n->vidro_x0 + i * vl / 3.0f, 1.95f, n->vidro_z}, (Vector3){0.12f, 2.1f, 0.14f}, (Color){0x1c, 0x20, 0x1e, 255});
    desenha_moveis(n);
    desenha_portas_e_duto(n);
    desenha_porta_da_sala(e);
}

static Color cor_lampada(const Estado *e) {
    if (regras_fase_eh_saida(e->fase)) return e->painel.resolvido ? COR_LAMPADA : COR_ALARME;
    return COR_LAMPADA;
}

// Fagulhas dos glifos e poeira do apagão (a coleção de partículas).
static void desenha_particulas(const Estado *e) {
    for (int i = 0; i < e->particulas.n; i++) {
        const Particula *p = &e->particulas.itens[i];
        float vida = p->vida / p->vida_max;
        float brilho = fminf(1.0f, vida * 4.0f) * fminf(1.0f, (1.0f - vida) * 6.0f); // aparece e some suave
        Color cor = p->tipo == PARTICULA_FAGULHA ? Fade(COR_FOSFORO, vida) : p->tipo == PARTICULA_POEIRA ? Fade((Color){170, 160, 140, 255}, 0.35f * brilho) : Fade(COR_PELE, vida);
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
        for (int i = 0; i < n->n_lampadas; i++) DrawSphereEx(n->lampadas[i], 0.09f, 6, 6, lamp);
        DrawSphereEx((Vector3){n->diario.x, PE_DIREITO - 0.3f, n->diario.z}, 0.08f, 6, 6, COR_LAMPADA);
    }
    float sala = brilho_sala(e);
    if (!saida && sala > 0.5f) DrawSphereEx((Vector3){8.0f, PE_DIREITO - 0.25f, 11.0f}, 0.11f, 8, 8, e->alarme_t > 0 ? COR_ALARME : lamp);
    bool menu = e->fase == FASE_MENU || e->fase == FASE_INTRO;
    if ((menu || e->rod.luz_a) && e->apagao_t <= 0.0f) DrawSphereEx((Vector3){5.0f, PE_DIREITO - 0.4f, 4.0f}, 0.1f, 8, 8, COR_VIDRO);
    if (!menu && e->rod.luz_b && e->apagao_t <= 0.0f) DrawSphereEx((Vector3){11.0f, PE_DIREITO - 0.4f, 4.0f}, 0.1f, 8, 8, COR_VIDRO);
    Color tela = e->jog.modo == MODO_TERMINAL ? COR_FOSFORO : (Color){0x2a, 0x6a, 0x3a, 255};
    DrawCubeV((Vector3){n->console.x, 1.1f, n->console.z - 0.78f}, (Vector3){0.9f, 0.14f, 0.3f}, tela);
}

void nivel_desenha_vidro(const Estado *e) {
    const Nivel *n = &e->nivel;
    float meio_x = (n->vidro_x0 + n->vidro_x1) * 0.5f, vl = n->vidro_x1 - n->vidro_x0;
    float opacidade = 0.09f + (VIDAS_VIDRO - e->vidas) * 0.05f;
    if (e->fase == FASE_FIM_RUIM) opacidade *= fmaxf(0.0f, 1.0f - (e->fim.tempo - 4.5f) / 1.5f); // o vidro se dissolve
    DrawCubeV((Vector3){meio_x, 1.95f, n->vidro_z}, (Vector3){vl, 2.1f, 0.05f}, Fade(COR_VIDRO, opacidade));
}
