// criatura.c — o Precursor: corpo procedural, a cela, a IA dos corredores (A*), captura e o susto.
// Estados: PATRULHA → INVESTIGA (ruído mais forte) → PERSEGUE (vê o jogador) → PROCURA (6 s) → PATRULHA.
// A decisão de estado é pura (regras_proximo_estado_criatura); aqui ficam percepção, caminho e movimento.
#include "criatura.h"
#include "susto.h"
#include "nivel.h"
#include "jogador.h"
#include "audio_sintese.h"
#include "ui.h"
#include "posfx.h"
#include "rlgl.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CELULAS              (MAPA_LARGURA * MAPA_ALTURA)
#define ALTURA_OLHO_CRIATURA 2.4f

// Pontos de patrulha (coluna, linha): os cantos do loop de corredores, o gerador e o Arquivo.
static const int PATRULHA[][2] = {{8, 5}, {21, 5}, {19, 2}, {21, 11}, {8, 11}, {3, 11}};
#define N_PATRULHA ((int)(sizeof(PATRULHA) / sizeof(PATRULHA[0])))

// ================================================================== modelo opcional
// [REQ:RNF4] asset ausente: sem o .glb, o placeholder procedural entra e só fica um TraceLog
void criatura_carrega_modelo(Recursos *rec) {
    char caminho[512];
    rec->tem_modelo = false;
    if (!ui_caminho_asset("modelos/precursor.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/precursor.glb; usando o placeholder procedural");
        return;
    }
    rec->modelo_precursor = LoadModel(caminho);
    rec->tem_modelo = IsModelValid(rec->modelo_precursor) && rec->modelo_precursor.meshCount > 0;
    if (!rec->tem_modelo) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; usando o placeholder", caminho);
        return;
    }
    for (int i = 0; i < rec->modelo_precursor.materialCount && rec->luz_ok; i++)
        rec->modelo_precursor.materials[i].shader = rec->luz;
}

void criatura_descarrega_modelo(Recursos *rec) {
    if (rec->tem_modelo) UnloadModel(rec->modelo_precursor);
}

// O modelo é escalado para CRIATURA_ALTURA_M (2,6 m), com os pés no chão.
static void desenha_modelo(const Estado *e, Color cor) {
    const Model *m = &e->rec.modelo_precursor;
    BoundingBox caixa = GetModelBoundingBox(*m);
    float altura = fmaxf(0.01f, caixa.max.y - caixa.min.y);
    float escala = CRIATURA_ALTURA_M / altura;
    DrawModelEx(*m, (Vector3){0, -caixa.min.y * escala, 0}, (Vector3){0, 1, 0}, 0.0f,
                (Vector3){escala, escala, escala}, cor);
}

// ================================================================== estados iniciais
void criatura_contem(Estado *e) {
    Criatura *c = &e->cri;
    memset(c, 0, sizeof(*c));
    c->contido = true;
    c->pos = e->nivel.cela_precursor;
    c->yaw = 0.0f; // olhando para o vidro (sul, +z)
}

void criatura_inicia_saida(Estado *e) {
    Criatura *c = &e->cri;
    memset(c, 0, sizeof(*c));
    c->pos = e->nivel.duto;
    c->tempo_para_surgir = CRIATURA_SURGE_S;
}

// A energia voltou: o campo de contenção "puxa" a criatura (ela some numa distorção) e ela não volta.
void criatura_puxa_para_contencao(Estado *e) {
    Criatura *c = &e->cri;
    c->tempo_para_surgir = 1e9f;
    if (!c->ativo) return;
    c->distorcao = 0.001f;
    audio_toca(e, SOM_DISTORCAO, 0.9f, 1.0f);
}

// ================================================================== A* na grade do mapa
static int celula_de(Vector3 p) {
    int c, l;
    if (!nivel_celula_de(p, &c, &l)) return -1;
    return l * MAPA_LARGURA + c;
}

static bool livre(const Nivel *n, int celula) {
    return celula >= 0 && celula < CELULAS &&
           nivel_caminhavel(n, celula % MAPA_LARGURA, celula / MAPA_LARGURA);
}

// Se o alvo cair num móvel (console, painel) ou na parede, usa uma célula livre vizinha.
static int celula_livre_perto(const Nivel *n, int celula) {
    if (celula < 0 || livre(n, celula)) return celula;
    const int viz[4] = {-1, 1, -MAPA_LARGURA, MAPA_LARGURA};
    for (int i = 0; i < 4; i++)
        if (livre(n, celula + viz[i])) return celula + viz[i];
    return -1;
}

static int distancia_manhattan(int a, int b) {
    return abs(a % MAPA_LARGURA - b % MAPA_LARGURA) + abs(a / MAPA_LARGURA - b / MAPA_LARGURA);
}

static int menor_f_aberto(const bool *aberto, const int *g, int destino) {
    int melhor = -1, melhor_f = 1 << 30;
    // padrão FOLD: a célula aberta com o menor f = g + h
    for (int i = 0; i < CELULAS; i++) {
        if (!aberto[i]) continue;
        int f = g[i] + distancia_manhattan(i, destino);
        if (f < melhor_f) {
            melhor_f = f;
            melhor = i;
        }
    }
    return melhor;
}

static int reconstroi(const int *veio, int origem, int destino, int *caminho, int maximo) {
    int tamanho = 0;
    for (int c = destino; c != origem && tamanho <= CELULAS; c = veio[c])
        tamanho++;
    if (tamanho > maximo) return 0;
    int i = tamanho;
    for (int c = destino; c != origem; c = veio[c])
        caminho[--i] = c;
    return tamanho;
}

// Vizinhança de 4: andando de centro a centro de células vizinhas, ele nunca corta a quina de uma parede.
static int a_estrela(const Nivel *n, int origem, int destino, int *caminho, int maximo) {
    if (!livre(n, origem) || !livre(n, destino)) return 0;
    int g[CELULAS], veio[CELULAS];
    bool aberto[CELULAS] = {false}, fechado[CELULAS] = {false};
    for (int i = 0; i < CELULAS; i++)
        g[i] = 1 << 29;
    g[origem] = 0;
    veio[origem] = -1;
    aberto[origem] = true;
    const int viz[4] = {-1, 1, -MAPA_LARGURA, MAPA_LARGURA};
    int atual;
    while ((atual = menor_f_aberto(aberto, g, destino)) >= 0 && atual != destino) {
        aberto[atual] = false;
        fechado[atual] = true;
        for (int k = 0; k < 4; k++) {
            int v = atual + viz[k];
            bool mesma_linha = k >= 2 || v / MAPA_LARGURA == atual / MAPA_LARGURA;
            if (!mesma_linha || !livre(n, v) || fechado[v] || g[atual] + 1 >= g[v]) continue;
            g[v] = g[atual] + 1;
            veio[v] = atual;
            aberto[v] = true;
        }
    }
    return atual == destino ? reconstroi(veio, origem, destino, caminho, maximo) : 0;
}

static void planeja(Estado *e, Vector3 destino) {
    Criatura *c = &e->cri;
    int alvo = celula_livre_perto(&e->nivel, celula_de(destino));
    c->destino = alvo >= 0 ? nivel_centro_celula(alvo % MAPA_LARGURA, alvo / MAPA_LARGURA) : destino;
    c->caminho_n = alvo >= 0 ? a_estrela(&e->nivel, celula_de(c->pos), alvo, c->caminho, MAX_CAMINHO) : 0;
    c->caminho_i = 0;
    c->tempo_replanejar = 0.0f;
}

// ================================================================== percepção
static bool ve_o_jogador(const Estado *e) {
    const Criatura *c = &e->cri;
    Vector3 olho = {c->pos.x, ALTURA_OLHO_CRIATURA, c->pos.z};
    Vector3 alvo = jogador_olho(e);
    Vector3 para = Vector3Subtract(alvo, olho);
    float distancia = Vector3Length(para);
    // [REQ:RF4] visão da criatura: raycast contra as paredes (GetRayCollisionBox) antes da regra pura
    float parede = col_paredes_distancia_raio(&e->paredes, olho, para, CRIATURA_VISAO_DIST);
    bool caminho_livre = parede >= distancia - 0.3f;
    bool escondido = e->jog.modo == MODO_ARMARIO;
    return regras_criatura_ve_jogador(olho, regras_frente(c->yaw), alvo, caminho_livre, escondido);
}

static PercepcaoCriatura percebe(const Estado *e, Vector3 *ruido_pos) {
    const Criatura *c = &e->cri;
    PercepcaoCriatura p = {0};
    p.ve_jogador = ve_o_jogador(e);
    float intensidade = 0.0f;
    int mais_forte = col_ruidos_mais_forte(&e->ruidos, c->pos, &intensidade);
    p.ouviu_ruido = mais_forte >= 0 && intensidade > RUIDO_LIMIAR_PERCEBIDO;
    if (p.ouviu_ruido) *ruido_pos = e->ruidos.itens[mais_forte].pos;
    p.chegou_ao_destino = c->caminho_i >= c->caminho_n && Vector3Distance(c->pos, c->destino) < 1.2f;
    p.tempo_no_estado = c->tempo_estado;
    return p;
}

// ================================================================== destino de cada estado
static void escolhe_destino(Estado *e, PercepcaoCriatura p, Vector3 ruido, float dt) {
    Criatura *c = &e->cri;
    bool replaneja = c->tempo_replanejar > (c->estado == CRIATURA_PERSEGUE ? 0.4f : 1.5f);
    switch (c->estado) {
    case CRIATURA_PATRULHA:
        if (p.chegou_ao_destino) c->patrulha_i = (c->patrulha_i + 1) % N_PATRULHA;
        if (p.chegou_ao_destino || c->caminho_n == 0 || replaneja)
            planeja(e, nivel_centro_celula(PATRULHA[c->patrulha_i][0], PATRULHA[c->patrulha_i][1]));
        break;
    case CRIATURA_INVESTIGA:
        if (p.ouviu_ruido && (replaneja || Vector3Distance(ruido, c->destino) > 2.0f)) planeja(e, ruido);
        break;
    case CRIATURA_PERSEGUE:
        c->ultima_vista = e->jog.pos;
        if (replaneja) planeja(e, e->jog.pos);
        break;
    case CRIATURA_PROCURA:
        c->procura_t -= dt;
        if (c->procura_t > 0.0f && c->caminho_i < c->caminho_n) break;
        c->procura_t = 1.6f;
        planeja(e, Vector3Add(c->ultima_vista,
                              (Vector3){(float)GetRandomValue(-3, 3), 0, (float)GetRandomValue(-3, 3)}));
        break;
    }
}

// ================================================================== movimento
// No bote (perseguindo, a menos de CRIATURA_BOTE_M), ele não pausa: vem direto e mais rápido.
static float velocidade(const Criatura *c, float t, bool bote) {
    if (bote) return CRIATURA_VEL_PERSEGUE * CRIATURA_BOTE_VEL;
    float base = c->estado == CRIATURA_PERSEGUE    ? CRIATURA_VEL_PERSEGUE
                 : c->estado == CRIATURA_INVESTIGA ? CRIATURA_VEL_INVESTIGA
                                                   : CRIATURA_VEL_PATRULHA;
    // Pausas e trancos: a cada 1,4 s ele quase para e depois avança um pouco mais rápido.
    return base * (fmodf(t, 1.4f) < 0.22f ? 0.15f : 1.13f);
}

static void passos_da_criatura(Estado *e, float dt) {
    Criatura *c = &e->cri;
    c->som_passo_t += dt;
    if (c->som_passo_t < 0.75f) return;
    c->som_passo_t = 0.0f;
    float longe = Vector3Distance(c->pos, e->jog.pos);
    if (longe < 16.0f) audio_toca(e, SOM_PASSO_CRIATURA, 1.0f - longe / 16.0f, 0.7f);
}

// Ir direto ao jogador só se o passo não bater em nada (a visão sai a 2,4 m e passa por cima de móveis
// baixos).
static bool pode_ir_direto(const Estado *e) {
    const Criatura *c = &e->cri;
    if (c->estado != CRIATURA_PERSEGUE || !c->ve_jogador) return false;
    Vector3 para = {e->jog.pos.x - c->pos.x, 0.0f, e->jog.pos.z - c->pos.z};
    float dist = Vector3Length(para);
    if (dist < 0.01f) return true;
    Vector3 adiante = Vector3Add(c->pos, Vector3Scale(para, fminf(0.4f, dist) / dist));
    return !col_paredes_colide(&e->paredes, adiante, 0.25f, 2.5f);
}

static void anda(Estado *e, float dt) {
    Criatura *c = &e->cri;
    Vector3 alvo;
    if (pode_ir_direto(e))
        alvo = e->jog.pos; // linha de visão livre e nada no caminho: vai direto
    else if (c->caminho_i < c->caminho_n)
        alvo = nivel_centro_celula(c->caminho[c->caminho_i] % MAPA_LARGURA,
                                   c->caminho[c->caminho_i] / MAPA_LARGURA);
    else
        return;
    Vector3 para = {alvo.x - c->pos.x, 0.0f, alvo.z - c->pos.z};
    float dist = Vector3Length(para);
    if (dist < 0.12f) {
        c->caminho_i++;
        return;
    }
    bool bote = c->estado == CRIATURA_PERSEGUE && Vector3Distance(c->pos, e->jog.pos) < CRIATURA_BOTE_M;
    // [REQ:RF1] o movimento da criatura também é multiplicado por dt
    float passo = fminf(dist, velocidade(c, e->tempo_total, bote) * dt);
    c->pos = Vector3Add(c->pos, Vector3Scale(para, passo / dist));
    float yaw_alvo = atan2f(para.x, para.z);
    c->yaw += atan2f(sinf(yaw_alvo - c->yaw), cosf(yaw_alvo - c->yaw)) * fminf(1.0f, 7.0f * dt);
    passos_da_criatura(e, dt);
}

// Quanto ele andou neste quadro alimenta a postura do corpo (regras_postura_criatura).
static void mede_passada(Criatura *c, float andou, float dt) {
    c->passada += andou;
    c->vel_suave += (andou / fmaxf(dt, 0.0001f) - c->vel_suave) * fminf(1.0f, 6.0f * dt);
}

// [REQ:RF4] captura com CheckCollisionSpheres: duas esferas de 0,8 m (pego quando a distância < 1,6 m)
static void tenta_capturar(Estado *e) {
    const Criatura *c = &e->cri;
    if (e->jog.modo == MODO_ARMARIO || susto_ativo(e)) return;
    Vector3 corpo_c = {c->pos.x, 1.0f, c->pos.z}, corpo_j = {e->jog.pos.x, 1.0f, e->jog.pos.z};
    if (CheckCollisionSpheres(corpo_c, CRIATURA_RAIO_CAPTURA, corpo_j, CRIATURA_RAIO_CAPTURA))
        criatura_captura(e);
}

// A porta do armário em que o jogador está: dela para fora (unitário).
static Vector3 para_fora_do_armario(const Estado *e) {
    int a = e->jog.armario;
    Vector3 fora = Vector3Subtract(e->nivel.armario_saida[a], e->nivel.armarios[a]);
    fora.y = 0.0f;
    return Vector3Normalize(fora);
}

void criatura_na_porta_do_armario(Estado *e) {
    Criatura *c = &e->cri;
    Vector3 fora = para_fora_do_armario(e);
    c->contido = false;
    c->ativo = true;
    c->surgindo = 1.0f;
    c->distorcao = 0.0f;
    c->estado = CRIATURA_PROCURA;
    c->caminho_n = 0;
    // Anda para fora a partir da porta enquanto o caminho está livre (nunca atravessa uma parede), até
    // RESP_DIST_PORTA: longe o bastante para o corpo inteiro aparecer pelas frestas.
    Vector3 armario = e->nivel.armarios[e->jog.armario];
    float d = 1.3f;
    while (d + 0.1f <= RESP_DIST_PORTA &&
           !col_paredes_colide(&e->paredes, Vector3Add(armario, Vector3Scale(fora, d + 0.1f)), 0.35f, 2.5f))
        d += 0.1f;
    c->pos = Vector3Add(armario, Vector3Scale(fora, d));
    c->pos.y = 0.0f;
    c->yaw = atan2f(-fora.x, -fora.z); // de frente para a porta
    audio_toca(e, SOM_PASSO_CRIATURA, 1.0f, 0.6f);
}

void criatura_chega_mais_perto(Estado *e) {
    Criatura *c = &e->cri;
    c->pos = Vector3Subtract(c->pos, Vector3Scale(para_fora_do_armario(e), 0.3f));
    audio_toca(e, SOM_DISTORCAO, 0.6f, 0.5f); // ouviu alguma coisa: um rosnado grave
}

static int ponto_mais_longe(const Estado *e) {
    int longe = 0;
    float maior = -1.0f;
    for (int i = 0; i < N_PATRULHA; i++) { // padrão FOLD: o ponto de patrulha mais longe do jogador
        float d = Vector3Distance(nivel_centro_celula(PATRULHA[i][0], PATRULHA[i][1]), e->jog.pos);
        if (d > maior) maior = d, longe = i;
    }
    return longe;
}

void criatura_desiste(Estado *e) {
    Criatura *c = &e->cri;
    c->estado = CRIATURA_PATRULHA;
    c->patrulha_i = ponto_mais_longe(e);
    c->ve_jogador = false;
    c->tempo_replanejar = 99.0f; // replaneja já, rumo ao ponto longe
}

void criatura_vai_para_longe(Estado *e) {
    Criatura *c = &e->cri;
    int i = ponto_mais_longe(e);
    c->pos = nivel_centro_celula(PATRULHA[i][0], PATRULHA[i][1]);
    c->caminho_n = 0;
    c->tempo_replanejar = 99.0f;
}

void criatura_captura(Estado *e) {
    if (susto_ativo(e)) return;
    e->pegos_na_fase++;
    e->jog.modo = MODO_ANDANDO;
    susto_inicia(e, EV_PEGO); // volta ao início da saída atual (o checkpoint)
}

// ================================================================== atualiza
static void atualiza_contido(Estado *e, float dt) {
    Criatura *c = &e->cri;
    Vector3 base = e->nivel.cela_precursor;
    float alvo_z = base.z + 1.3f * c->avanco_vidro + (c->mao_no_vidro ? 0.15f : 0.0f);
    c->pos = (Vector3){base.x, 0.0f, alvo_z};
    c->passo_t += dt;
}

// Ele sobe pelo duto 15 s depois do início da saída (dá tempo do jogador se orientar).
static bool atualiza_surgimento(Estado *e, float dt) {
    Criatura *c = &e->cri;
    if (c->ativo) {
        c->surgindo = fminf(1.0f, c->surgindo + dt / 1.5f);
        return c->surgindo < 1.0f;
    }
    c->tempo_para_surgir -= dt;
    if (c->tempo_para_surgir > 0.0f) return true;
    c->ativo = true;
    c->estado = CRIATURA_PATRULHA;
    c->tempo_replanejar = 99.0f;
    audio_toca(e, SOM_DISTORCAO, 0.8f, 0.55f); // sair do duto não é fala: um rugido grave
    return true;
}

static void atualiza_nos_corredores(Estado *e, float dt) {
    Criatura *c = &e->cri;
    if (c->distorcao > 0.0f) { // puxada pela contenção
        c->distorcao += dt / 1.5f;
        if (c->distorcao >= 1.0f) c->ativo = false;
        return;
    }
    if (atualiza_surgimento(e, dt)) return;
    Vector3 ruido = c->destino;
    PercepcaoCriatura p = percebe(e, &ruido);
    c->ve_jogador = p.ve_jogador;
    EstadoCriatura novo = regras_proximo_estado_criatura(c->estado, p);
    c->tempo_estado = novo == c->estado ? c->tempo_estado + dt : 0.0f;
    if (novo != c->estado) c->tempo_replanejar = 99.0f; // mudou de ideia: replaneja já
    c->estado = novo;
    c->tempo_replanejar += dt;
    escolhe_destino(e, p, ruido, dt);
    Vector3 antes = c->pos;
    anda(e, dt);
    mede_passada(c, Vector3Distance(antes, c->pos), dt);
    tenta_capturar(e);
}

void criatura_atualiza(Estado *e, float dt) {
    Criatura *c = &e->cri;
    c->fala_t = fmaxf(0.0f, c->fala_t - dt);
    if (c->contido)
        atualiza_contido(e, dt);
    else if (regras_fase_eh_saida(e->fase))
        atualiza_nos_corredores(e, dt);
}

// ================================================================== corpo procedural
// Humanoide de ~2,6 m: magro demais, cabeça elipsoide, dedos longos. Move-se com pausas e trancos.
static void membro(Vector3 a, Vector3 b, float raio, Color cor) {
    DrawCylinderEx(a, b, raio, raio * 0.8f, 5, cor);
}

static void mao(Vector3 pulso, Vector3 direcao, Color cor) {
    for (int d = -1; d <= 1; d++) {
        Vector3 ponta =
            Vector3Add(pulso, Vector3Add(Vector3Scale(direcao, 0.32f), (Vector3){d * 0.05f, -0.02f, 0}));
        membro(pulso, ponta, 0.012f, cor);
    }
}

static void cabeca(float altura, float fenda, Color cor) {
    rlPushMatrix();
    rlTranslatef(0.0f, altura, 0.0f);
    rlScalef(0.16f, 0.25f, 0.18f);
    DrawSphereEx((Vector3){0, 0, 0}, 1.0f, 8, 10, cor);
    rlPopMatrix();
    // A fenda vertical no rosto liso: se abre quando ele "fala".
    DrawCubeV((Vector3){0.0f, altura - 0.02f, 0.175f}, (Vector3){0.012f + fenda * 0.05f, 0.26f, 0.02f},
              (Color){18, 4, 6, 255});
}

// Brilho do corpo na cela: com as luzes A e B apagadas, ele é quase invisível.
static float visibilidade(const Estado *e) {
    if (!e->cri.contido) return 1.0f - e->cri.distorcao;
    if (e->apagao_t > 0.0f) return 0.05f;
    bool menu = e->fase == FASE_MENU || e->fase == FASE_INTRO;
    float v = 0.12f + (menu || e->rod.luz_a ? 0.4f : 0.0f) + (e->rod.luz_b ? 0.6f : 0.0f);
    v += nivel_piscada_cela(e) * 0.6f; // aparece no flash da lâmpada com defeito
    if (e->fase == FASE_RESPOSTA_FINAL || e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM) v = 1.0f;
    return fminf(1.0f, v);
}

static Color escurece(Color c, float f) {
    return (Color){(unsigned char)(c.r * f), (unsigned char)(c.g * f), (unsigned char)(c.b * f), c.a};
}

static void desenha_corpo(const Estado *e, Color pele) {
    const Criatura *c = &e->cri;
    float t = e->tempo_total;
    float tranco = (fmodf(t * 0.7f, 1.0f) > 0.85f) ? sinf(t * 40.0f) * 0.04f : 0.0f; // pausa... tranco
    float cabeca_y = 2.42f + sinf(t * 1.3f) * 0.015f + tranco;
    membro((Vector3){-0.1f, 0, 0}, (Vector3){-0.09f, 1.25f, 0}, 0.05f, pele);
    membro((Vector3){0.1f, 0, 0}, (Vector3){0.09f, 1.25f, 0}, 0.05f, pele);
    membro((Vector3){0, 1.2f, 0}, (Vector3){0, 2.05f, 0}, 0.13f, pele);
    membro((Vector3){0, 2.0f, 0}, (Vector3){0, 2.22f, 0}, 0.04f, pele);
    Vector3 ombro_e = {-0.2f, 2.0f, 0}, ombro_d = {0.2f, 2.0f, 0};
    Vector3 pulso_e = {-0.26f, 0.85f, 0.05f}, pulso_d = {0.26f, 0.85f, 0.05f};
    if (c->mao_no_vidro) pulso_d = (Vector3){0.3f, 1.75f, 0.25f}; // a mão encostada no vidro
    if (c->estado == CRIATURA_PERSEGUE && c->ativo)
        pulso_e = pulso_d = (Vector3){0, 1.6f, 0.55f}; // braços estendidos
    membro(ombro_e, pulso_e, 0.035f, pele);
    membro(ombro_d, pulso_d, 0.035f, pele);
    mao(pulso_e, (Vector3){0, -1, 0}, pele);
    mao(pulso_d, c->mao_no_vidro ? (Vector3){0, 0.3f, 1} : (Vector3){0, -1, 0}, pele);
    cabeca(cabeca_y,
           c->fala_t > 0.0f || c->estado == CRIATURA_PERSEGUE ? 0.6f + 0.4f * sinf(t * 18.0f) : 0.0f, pele);
}

// Com F3: o caminho do A* em amarelo.
static void desenha_caminho(const Estado *e) {
    const Criatura *c = &e->cri;
    if (!e->debug_criatura || !c->ativo) return;
    for (int i = c->caminho_i; i < c->caminho_n; i++) {
        Vector3 p = nivel_centro_celula(c->caminho[i] % MAPA_LARGURA, c->caminho[i] / MAPA_LARGURA);
        DrawCubeV((Vector3){p.x, 0.05f, p.z}, (Vector3){0.3f, 0.05f, 0.3f}, YELLOW);
    }
}

// Sem animação no modelo: corcunda, balanço e respiração por transformação, com o pivô nos pés.
static void aplica_postura(const Estado *e) {
    PosturaCriatura p = regras_postura_criatura(e->cri.vel_suave, e->cri.passada, e->tempo_total);
    rlTranslatef(0.0f, p.sobe, 0.0f);
    rlRotatef(p.inclina_graus, 1.0f, 0.0f, 0.0f); // +X inclina a cabeça para +Z (a frente)
    rlRotatef(p.balanco_graus, 0.0f, 0.0f, 1.0f);
    rlScalef(p.respira, 1.0f, p.respira);
}

void criatura_desenha(const Estado *e) {
    const Criatura *c = &e->cri;
    desenha_caminho(e);
    if ((!c->contido && !c->ativo) || susto_rosto_na_tela(e)) return; // no susto, só o rosto aparece
    float t = e->tempo_total;
    float jitter = (fmodf(t * 0.9f, 1.0f) > 0.9f) ? sinf(t * 55.0f) * 4.0f : sinf(t * 0.8f) * 1.5f;
    if (e->fase == FASE_FIM_BOM)
        jitter = c->congelado_t > 0.0f ? 0.0f : sinf(t * 80.0f) * 22.0f; // convulsiona e congela
    Color pele = escurece(COR_PELE, visibilidade(e));
    // A contraluz liga e desliga FORA do rlPushMatrix: ela envia o lote pendente, e a raylib manda junto a
    // transformação atual como matModel. Dentro do push, as paredes eram iluminadas na posição dele (pretas).
    float contraluz = (c->contido ? CONTRALUZ_CELA : CONTRALUZ_CORREDOR) * (1.0f - c->distorcao);
    posfx_contraluz(e, COR_CONTRALUZ, contraluz); // recorta a silhueta; desligada logo depois do corpo
    rlPushMatrix();
    float subida = c->contido ? 0.0f : (c->surgindo - 1.0f) * 2.7f; // sobe pelo duto
    rlTranslatef(c->pos.x + (c->distorcao > 0.0f ? sinf(t * 70.0f) * c->distorcao * 0.4f : 0.0f),
                 c->pos.y + subida, c->pos.z);
    rlRotatef(c->yaw * RAD2DEG + jitter, 0.0f, 1.0f, 0.0f);
    aplica_postura(e);
    if (c->distorcao > 0.0f)
        rlScalef(1.0f - c->distorcao * 0.8f, 1.0f + c->distorcao * 1.5f, 1.0f - c->distorcao * 0.8f);
    if (e->rec.tem_modelo)
        desenha_modelo(e, pele);
    else
        desenha_corpo(e, pele);
    rlPopMatrix();
    posfx_contraluz(e, COR_CONTRALUZ, 0.0f); // envia o corpo procedural (lote) ainda com a contraluz
}
