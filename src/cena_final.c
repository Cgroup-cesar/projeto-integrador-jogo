// cena_final.c — o final bom como cena, no mesmo esquema da volta à sala (retorno.c): o jogo assume o
// jogador e a coreografia vem das regras puras (regras_cena.c). Ele levanta do console, vai até a escada ao
// lado dos caixotes, sobe e empurra a escotilha do teto (escotilha.c): ela emperra, cede e abre, o clarão lá
// de cima desce pelo vão e a tela estoura em branco. Lá fora, a fita do Exército, na mão de alguém: ele sai
// pela entrada de tijolo da base (base.glb), anda até o pátio e olha em volta, à noite, com a mata em volta
// (arvores.glb), os holofotes das torres varrendo o chão e, no fim, a luz da nave subindo por cima das
// árvores. Sem os modelos, a fita fica só com o chão e as luzes, e o resto da cena não muda.
#include "cena_final.h"
#include "efeitos.h"
#include "jogador.h"
#include "lampada_sala.h"
#include "audio_sintese.h"
#include "posfx.h"
#include "rlgl.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>

// Onde ficam as coisas no modelo (em metros do modelo, antes da BASE_ESCALA), medidos nas malhas do .glb
// (Military Base #2): o alto da torre de vigia (canto sudoeste) e o telhado do galpão, de onde saem os dois
// holofotes; os dois postes de luz do pátio; a porta na parede oeste do galpão comprido, de onde ele sai,
// e o vão entre as barracas, até onde ele anda. O meio da base é o centro do anel da mata (o muro fica a
// uns 50 m dele).
static const Vector3 TORRE_A = {-37.1f, 11.2f, 44.3f}, TORRE_B = {12.4f, 11.0f, -30.0f};
static const Vector3 POSTE_A = {-9.3f, 8.6f, -29.4f}, POSTE_B = {-31.6f, 8.6f, -29.4f};
static const Vector3 ENTRADA = {11.4f, 3.1f, -8.4f};
static const Vector3 PORTA_BASE = {11.0f, 0.0f, -8.4f}, PATIO = {4.6f, 0.0f, -8.2f};
static const Vector3 MEIO_BASE = {0.5f, 0.0f, -9.0f};

static void com_luz(Model *m, const Recursos *rec) {
    for (int i = 0; i < m->materialCount && rec->luz_ok; i++) // padrão MAP: luz do jogo
        m->materials[i].shader = rec->luz;
}

static float altura_de(Mesh m) {
    BoundingBox b = GetMeshBoundingBox(m);
    return b.max.y - b.min.y;
}

// A cena das árvores vem com cada árvore em duas malhas seguidas (casca e folhas, materiais diferentes, as
// duas altas) e o capim em milhares de tufos (o material com mais malhas). Guarda as duas primeiras árvores
// de tipos diferentes (contagens de triângulos diferentes) e alguns tufos espalhados pela lista.
static void separa_mata(Recursos *rec) {
    const Model *m = &rec->modelo_mata;
    rec->n_arvores = rec->n_capim = 0;
    for (int i = 0; i + 1 < m->meshCount && rec->n_arvores < 2; i++) { // padrão FILTER: os pares altos
        bool par = m->meshMaterial[i] != m->meshMaterial[i + 1] && altura_de(m->meshes[i]) > 5.0f &&
                   altura_de(m->meshes[i + 1]) > 5.0f;
        bool outro =
            rec->n_arvores == 0 || m->meshes[rec->mata_casca[0]].triangleCount != m->meshes[i].triangleCount;
        if (!par || !outro) continue;
        BoundingBox b = GetMeshBoundingBox(m->meshes[i]);
        rec->mata_casca[rec->n_arvores] = i;
        rec->mata_folha[rec->n_arvores] = i + 1;
        rec->mata_pe[rec->n_arvores++] =
            (Vector3){(b.min.x + b.max.x) * 0.5f, b.min.y, (b.min.z + b.max.z) * 0.5f};
        i++;
    }
    int contagem[32] = {0}, capim = 0;
    for (int i = 0; i < m->meshCount; i++) // padrão FOLD: o material com mais malhas é o do capim
        if (m->meshMaterial[i] < 32) contagem[m->meshMaterial[i]]++;
    for (int k = 1; k < 32; k++)
        if (contagem[k] > contagem[capim]) capim = k;
    int passo = contagem[capim] / 8 > 0 ? contagem[capim] / 8 : 1, visto = 0;
    for (int i = 0; i < m->meshCount && rec->n_capim < 8; i++) // um tufo a cada `passo` do material
        if (m->meshMaterial[i] == capim && altura_de(m->meshes[i]) < 1.6f && visto++ % passo == 0)
            rec->mata_capim[rec->n_capim++] = i;
}

static void carrega_mata(Recursos *rec) {
    char caminho[512];
    rec->tem_mata = false;
    if (!ui_caminho_asset("modelos/arvores.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/arvores.glb; a base do final fica sem a mata");
        return;
    }
    rec->modelo_mata = LoadModel(caminho);
    rec->tem_mata = IsModelValid(rec->modelo_mata) && rec->modelo_mata.meshCount > 0;
    if (!rec->tem_mata) return;
    com_luz(&rec->modelo_mata, rec);
    separa_mata(rec);
    TraceLog(LOG_INFO, "PRECURSOR: mata do final: %d árvores e %d tufos de capim", rec->n_arvores,
             rec->n_capim);
}

void cena_final_carrega(Recursos *rec) {
    char caminho[512];
    rec->tem_base = false;
    if (!ui_caminho_asset("modelos/base.glb", caminho, sizeof(caminho))) {
        TraceLog(LOG_INFO, "PRECURSOR: sem assets/modelos/base.glb; a fita do final mostra só o descampado");
        carrega_mata(rec);
        return;
    }
    rec->modelo_base = LoadModel(caminho);
    rec->tem_base = IsModelValid(rec->modelo_base) && rec->modelo_base.meshCount > 0;
    if (!rec->tem_base) {
        TraceLog(LOG_WARNING, "PRECURSOR: não deu para carregar %s; a fita mostra só o descampado", caminho);
        carrega_mata(rec);
        return;
    }
    com_luz(&rec->modelo_base, rec);
    carrega_mata(rec);
}

void cena_final_descarrega(Recursos *rec) {
    if (rec->tem_base) UnloadModel(rec->modelo_base);
    if (rec->tem_mata) UnloadModel(rec->modelo_mata);
}

static bool no_final_bom(const Estado *e) {
    return e->fase == FASE_FIM_BOM && !e->fim.tela_final;
}

static Vector3 na_base(Vector3 p) {
    return Vector3Add(BASE_ORIGEM, Vector3Scale(p, BASE_ESCALA));
}

bool cena_final_fora(const Estado *e) {
    EtapaCena etapa = regras_cena_etapa(e->fim.tempo);
    return no_final_bom(e) && (etapa == CENA_FORA || etapa == CENA_PRETO);
}

bool cena_final_dentro(const Estado *e) {
    EtapaCena etapa = regras_cena_etapa(e->fim.tempo);
    return no_final_bom(e) && etapa >= CENA_VIRA && etapa <= CENA_BRANCO;
}

void cena_final_inicia(Estado *e) {
    Fim *f = &e->fim;
    f->cena_de = (PoseJogador){e->jog.pos, e->jog.yaw, e->jog.pitch};
    f->cena_pe = (PoseJogador){{ESCOTILHA_X, 0.0f, CENA_PE_Z}, 0.0f, 0.0f}; // de frente para os degraus (sul)
}

static bool cruzou(float antes, float t, float marca) {
    return antes < marca && t >= marca;
}

// O meio do vão da escotilha, um pouco acima da laje: de onde a luz lá de fora desce.
static Vector3 vao(void) {
    return (Vector3){ESCOTILHA_X, PE_DIREITO + 0.4f, ESCOTILHA_Z};
}

// Um som a cada degrau pisado e, no empurrão, a trava cedendo e a tampa rangendo. O ENTER pula 3 s: no
// máximo um som por pulo.
static void sons_da_subida(Estado *e, float antes, float t) {
    int degrau = regras_cena_degrau(t);
    if (degrau >= 0 && degrau != regras_cena_degrau(antes)) {
        Vector3 pe = {ESCOTILHA_X, (degrau + 1) * ESCADA_DEGRAU_M, ESCADA_PLANO_Z};
        audio_toca_em(e, SOM_PASSO, pe, 6.0f, 0.75f);
    }
    if (cruzou(antes, t, CENA_ABRE_INICIO_S + 0.15f)) audio_toca_em(e, SOM_ARMARIO_ABRE, vao(), 8.0f, 0.8f);
}

void cena_final_atualiza(Estado *e, float antes, float t) {
    if (e->fase != FASE_FIM_BOM) return;
    Fim *f = &e->fim;
    if (cruzou(antes, t, CENA_COMECA_S)) e->jog.modo = MODO_ANDANDO; // sai do console
    sons_da_subida(e, antes, t);
    EtapaCena etapa = regras_cena_etapa(t);
    if (etapa == CENA_ANTES || etapa == CENA_FIM) return;
    PoseJogador saida = {na_base(PORTA_BASE), -PI * 0.5f, 0.0f}; // na porta do galpão, olhando para o oeste
    PoseJogador p = etapa <= CENA_BRANCO ? regras_cena_pose(t, f->cena_de, f->cena_pe)
                                         : regras_cena_fora(t - CENA_FORA_INICIO_S, saida, na_base(PATIO));
    float dt = Clamp(t - antes, 0.0001f, 0.1f);           // o ENTER pula 3 s: os passos não contam o pulo
    bool chao = etapa <= CENA_ANDA || etapa >= CENA_FORA; // na escada, sem passos nem balanço de andar
    bool andando = chao && Vector3Distance(e->jog.pos, p.pos) > 0.3f * dt;
    e->jog.pos = p.pos;
    e->jog.yaw = p.yaw;
    e->jog.pitch = p.pitch;
    e->jog.mira = -1;
    e->in.mover = (Vector2){0.0f, andando ? 1.0f : 0.0f};
    jogador_passos(e, andando, dt);
}

// ------------------------------------------------------------------ luzes
static Vector3 cor_luz(Color c, float k) {
    return (Vector3){c.r / 255.0f * k, c.g / 255.0f * k, c.b / 255.0f * k};
}

int cena_final_luzes(const Estado *e, Vector3 *pos, Vector3 *cor, Vector4 *cone, int *ids) {
    (void)ids; // nenhuma das três depende das paredes (já vêm −1)
    int k = 0;
    pos[k] = lampada_sala_bulbo(); // a lâmpada da sala fica acesa atrás dele
    cor[k++] = cor_luz(COR_LAMPADA, 1.2f);
    pos[k] = vao(); // o clarão: desce pelo vão em cone, conforme a tampa abre
    cone[k] = (Vector4){0.0f, -1.0f, 0.0f, cosf(38.0f * DEG2RAD)};
    cor[k++] = cor_luz(COR_LANTERNA, CENA_CLARAO_FORCA * regras_cena_escotilha(e->fim.tempo));
    float alto = fminf(e->jog.pos.y + ALTURA_OLHOS + 0.3f, PE_DIREITO - 0.1f); // a luz fraca no jogador
    pos[k] = (Vector3){e->jog.pos.x, alto, e->jog.pos.z};
    cor[k++] = cor_luz(COR_LUMINARIA, 0.32f);
    return k;
}

void cena_final_desenha_emissivos(const Estado *e) {
    if (!cena_final_dentro(e)) return;
    float abertura = regras_cena_escotilha(e->fim.tempo);
    if (abertura <= 0.0f) return;
    posfx_comeca_emissivo(e);
    posfx_emissivo(e, CENA_CLARAO_EMISSAO * abertura); // o fundo do poço estoura no bloom
    DrawCubeV((Vector3){ESCOTILHA_X, PE_DIREITO + ESCOTILHA_POCO_M - 0.01f, ESCOTILHA_Z},
              (Vector3){ESCOTILHA_LADO_M, 0.004f, ESCOTILHA_LADO_M}, COR_LANTERNA);
    posfx_emissivo(e, 1.0f);
    posfx_termina_emissivo(e);
}

// ------------------------------------------------------------------ lá fora
#define LUZES_FORA 7

// Os holofotes das torres varrem o chão em volta da base, cada um no seu ritmo.
static Vector4 holofote(Vector3 de, Vector3 alvo) {
    Vector3 d = Vector3Normalize(Vector3Subtract(alvo, de));
    return (Vector4){d.x, d.y, d.z, cosf(15.0f * DEG2RAD)};
}

static int luzes_fora(const Estado *e, Vector3 *pos, Vector3 *cor, Vector4 *cone, float *queda) {
    float t = e->fim.tempo - CENA_FORA_INICIO_S;
    Vector3 c = BASE_ORIGEM;
    for (int i = 0; i < LUZES_FORA; i++) { // padrão MAP: pontual, sem paredes
        cone[i] = (Vector4){0.0f, -1.0f, 0.0f, -2.0f};
        queda[i] = 1.0f;
    }
    pos[0] = (Vector3){c.x - 70.0f, 90.0f, c.z + 50.0f}; // a lua: longe e sem queda (quase direcional)
    cor[0] = (Vector3){0.045f, 0.06f, 0.10f};            // o asfalto claro do pátio pedia uma lua fraca
    queda[0] = 0.0f;
    Vector3 torres[2] = {na_base(TORRE_A), na_base(TORRE_B)}, patio = na_base(PATIO);
    for (int i = 0; i < 2; i++) { // os holofotes da torre e do galpão varrem o pátio, perto de quem filma
        float a = t * (0.45f + 0.15f * i) + i * 2.4f;
        Vector3 alvo = {patio.x + sinf(a) * 11.0f, 0.0f, patio.z + cosf(a * 0.8f) * 11.0f};
        pos[1 + i] = torres[i];
        cone[1 + i] = holofote(torres[i], alvo);
        cor[1 + i] = cor_luz(COR_LANTERNA, 15.0f);
        queda[1 + i] = i == 0 ? 0.012f : 0.08f; // a torre fica a uns 65 m do pátio; o galpão, a 25 m
    }
    pos[3] = na_base(ENTRADA); // a lâmpada sobre a porta do galpão, de onde ele saiu
    cor[3] = cor_luz(COR_LAMPADA, 2.2f);
    queda[3] = 0.6f;
    // A nave: por cima das árvores, à frente de quem sai pela porta (é o que ele vê primeiro e, no fim da
    // volta, o que ele procura no céu), parada e depois subindo.
    float brilho = regras_cena_nave_brilho(t);
    pos[4] = (Vector3){patio.x - 70.0f, regras_cena_nave_altura(t), patio.z - 6.0f};
    cor[4] = (Vector3){1.1f * brilho, 1.7f * brilho, 1.5f * brilho};
    queda[4] = 0.012f;
    pos[5] = na_base(POSTE_A); // os postes de luz do pátio (sódio, alaranjados)
    cor[5] = cor_luz((Color){255, 170, 90, 255}, 2.6f);
    queda[5] = 0.25f;
    pos[6] = na_base(POSTE_B);
    cor[6] = cor[5];
    queda[6] = 0.25f;
    return LUZES_FORA;
}

// Um "sorteio" repetível pelo índice (sem rand()): a mesma mata em toda partida.
static float sorteio(int i, int k) {
    float s = sinf((float)i * 12.9898f + (float)k * 78.233f) * 43758.5453f;
    return s - floorf(s);
}

static void desenha_copia(const Model *m, int malha, Vector3 pe, Vector3 onde, float giro, float escala) {
    Matrix t = MatrixMultiply(MatrixTranslate(-pe.x, -pe.y, -pe.z), MatrixScale(escala, escala, escala));
    t = MatrixMultiply(MatrixMultiply(t, MatrixRotateY(giro)), MatrixTranslate(onde.x, onde.y, onde.z));
    DrawMesh(m->meshes[malha], m->materials[m->meshMaterial[malha]], t);
}

// A mata: árvores num anel em volta da cerca (de 11 a 30 m do meio) e tufos de capim, mais densos encostados
// na cerca, por dentro e por fora. As folhas e o capim são recortados como o alambrado.
static void desenha_mata(const Estado *e) {
    const Recursos *r = &e->rec;
    if (!r->tem_mata) return;
    const Model *m = &r->modelo_mata;
    Vector3 c = na_base(MEIO_BASE), patio = na_base(PATIO);
    for (int i = 0; i < MATA_ARVORES && r->n_arvores > 0; i++) { // laço de desenho: cada árvore
        int tipo = i % r->n_arvores;
        float a = (i + 0.6f * sorteio(i, 1)) / MATA_ARVORES * 2.0f * PI, raio = 58.0f + 30.0f * sorteio(i, 2);
        Vector3 onde = {c.x + sinf(a) * raio, 0.0f, c.z + cosf(a) * raio};
        float giro = sorteio(i, 3) * 2.0f * PI,
              escala = 0.9f + 0.35f * sorteio(i, 4) + (raio - 58.0f) * 0.01f; // as do fundo, maiores
        desenha_copia(m, r->mata_casca[tipo], r->mata_pe[tipo], onde, giro, escala);
        desenha_copia(m, r->mata_folha[tipo], r->mata_pe[tipo], onde, giro, escala);
    }
    for (int i = 0; i < MATA_CAPIM && r->n_capim > 0; i++) { // laço de desenho: cada tufo
        int tufo = r->mata_capim[i % r->n_capim];
        BoundingBox b = GetMeshBoundingBox(m->meshes[tufo]);
        Vector3 pe = {(b.min.x + b.max.x) * 0.5f, b.min.y, (b.min.z + b.max.z) * 0.5f};
        float a = sorteio(i, 5) * 2.0f * PI;
        // no pátio, nas rachaduras do chão, de 4 a 24 m de onde ele para (o caminho dele fica livre)
        float raio = 4.0f + 20.0f * sorteio(i, 6) * sorteio(i, 7);
        Vector3 onde = {patio.x + sinf(a) * raio, 0.0f, patio.z + cosf(a) * raio};
        desenha_copia(m, tufo, pe, onde, sorteio(i, 8) * 2.0f * PI, 0.4f + 0.4f * sorteio(i, 9));
    }
}

bool cena_final_desenha_fora(const Estado *e) {
    if (!cena_final_fora(e)) return false;
    const Recursos *r = &e->rec;
    Vector3 pos[LUZES_SHADER], cor[LUZES_SHADER];
    Vector4 cone[LUZES_SHADER];
    float queda[LUZES_SHADER];
    int ids[LUZES_SHADER];
    for (int i = 0; i < LUZES_SHADER; i++)
        ids[i] = -1; // padrão MAP: fora do mapa, nada segura a luz
    int n = luzes_fora(e, pos, cor, cone, queda);
    efeitos_guarda_luzes(pos, cor, cone, queda, ids, n, -1);
    efeitos_sem_sombra(e);
    BeginMode3D(e->camera);
    efeitos_guarda_camera();
    posfx_comeca_luz(e, pos, cor, cone, queda, ids, n, -1, BASE_AMBIENTE);
    float nevoa = BASE_NEVOA;
    if (r->luz_ok) SetShaderValue(r->luz, r->loc_luz_nevoa, &nevoa, SHADER_UNIFORM_FLOAT);
    Vector3 c = BASE_ORIGEM;
    DrawPlane((Vector3){c.x, -0.02f, c.z}, (Vector2){400.0f, 400.0f},
              (Color){38, 42, 28, 255}); // o chão de mato
    float recorte = 0.5f; // o alambrado, as folhas e o capim: os furos da textura são transparentes
    rlDrawRenderBatchActive();
    if (r->luz_ok) SetShaderValue(r->luz, r->loc_luz_recorte, &recorte, SHADER_UNIFORM_FLOAT);
    rlDisableBackfaceCulling(); // a cerca, a rede das torres, as folhas e o capim são planos de uma face só
    if (r->tem_base)
        DrawModelEx(r->modelo_base, c, (Vector3){0, 1, 0}, 0.0f,
                    (Vector3){BASE_ESCALA, BASE_ESCALA, BASE_ESCALA}, WHITE);
    desenha_mata(e);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    recorte = 0.0f;
    if (r->luz_ok) SetShaderValue(r->luz, r->loc_luz_recorte, &recorte, SHADER_UNIFORM_FLOAT);
    posfx_termina_luz(e);
    posfx_comeca_emissivo(e); // o que brilha: as lentes dos holofotes, a lâmpada da entrada e a nave
    posfx_emissivo(e, 30.0f);
    DrawSphereEx(pos[1], 0.18f, 8, 8, COR_LANTERNA);
    DrawSphereEx(pos[2], 0.18f, 8, 8, COR_LANTERNA);
    posfx_emissivo(e, 12.0f);
    DrawSphereEx(pos[3], 0.08f, 6, 6, COR_LAMPADA);
    DrawSphereEx(pos[5], 0.2f, 6, 6, (Color){255, 170, 90, 255});
    DrawSphereEx(pos[6], 0.2f, 6, 6, (Color){255, 170, 90, 255});
    posfx_emissivo(e, 25.0f * regras_cena_nave_brilho(e->fim.tempo - CENA_FORA_INICIO_S));
    DrawSphereEx(pos[4], 0.9f, 12, 12, (Color){200, 255, 235, 255});
    posfx_emissivo(e, 1.0f);
    posfx_termina_emissivo(e);
    EndMode3D();
    return true;
}

// ------------------------------------------------------------------ por cima (no quadro, antes do VHS)
// A gravação: ● REC no alto, o Exército e o lugar embaixo, a data e a hora correndo.
static void osd(const Estado *e, float h) {
    float s = h / 720.0f, t = e->fim.tempo - CENA_FORA_INICIO_S, m = 36 * s, tam = 22 * s;
    Color cor = {232, 255, 240, 255};
    if (fmodf(t, 1.0f) < 0.6f)
        DrawCircleV((Vector2){m + 8 * s, m + tam * 0.5f}, 8 * s, (Color){230, 40, 40, 255});
    ui_texto(e, "REC", (Vector2){m + 24 * s, m}, tam, cor);
    ui_texto(e, dialogo_texto(TXT_CENA_FITA_EXERCITO), (Vector2){m, h - m - 2 * tam - 8 * s}, tam, cor);
    int seg = CENA_FITA_HORA + (int)t;
    char linha[64];
    snprintf(linha, sizeof(linha), "%s  %02d:%02d:%02d", dialogo_texto(TXT_FITA_DATA), seg / 3600 % 24,
             seg / 60 % 60, seg % 60);
    ui_texto(e, linha, (Vector2){m, h - m - tam}, tam, cor);
}

// O clarão do vão aberto, visto de baixo: o halo e o risco horizontal, por cima da cena.
static void ofusca(const Estado *e, float w, float h) {
    float k = regras_cena_escotilha(e->fim.tempo);
    Vector3 lente = vao(), frente = Vector3Subtract(e->camera.target, e->camera.position);
    if (k <= 0.0f || Vector3DotProduct(frente, Vector3Subtract(lente, e->camera.position)) <= 0.0f) return;
    Vector2 p = GetWorldToScreenEx(lente, e->camera, (int)w, (int)h);
    float perto = Clamp(1.0f - Vector3Distance(lente, e->camera.position) / 16.0f, 0.25f, 1.0f);
    float raio = h * 0.45f * k * perto;
    Color luz = {255, 246, 225, 255};
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(p, raio, Fade(luz, 0.55f * k), Fade(luz, 0.0f));
    DrawCircleGradient(p, raio * 0.18f, Fade(WHITE, 0.95f * k), Fade(luz, 0.0f));
    float risco = w * 0.5f * k * perto, alto = h * 0.006f;
    DrawRectangleGradientH((int)(p.x - risco), (int)(p.y - alto), (int)risco, (int)(alto * 2),
                           Fade(luz, 0.0f), Fade(luz, 0.5f * k));
    DrawRectangleGradientH((int)p.x, (int)(p.y - alto), (int)risco, (int)(alto * 2), Fade(luz, 0.5f * k),
                           Fade(luz, 0.0f));
    EndBlendMode();
}

void cena_final_desenha(const Estado *e) {
    if (!no_final_bom(e)) return;
    float t = e->fim.tempo, w = (float)e->rec.alvo_largura, h = (float)e->rec.alvo_altura;
    float faixa = h * RETORNO_FAIXA_ALTURA * regras_cena_faixa(t);
    if (faixa > 0.0f) {
        DrawRectangle(0, 0, (int)w, (int)faixa, BLACK);
        DrawRectangle(0, (int)(h - faixa), (int)w, (int)faixa + 1, BLACK);
    }
    if (cena_final_dentro(e)) ofusca(e, w, h);
    if (cena_final_fora(e)) osd(e, h);
    float branco = regras_cena_branco(t);
    if (branco > 0.0f) DrawRectangle(0, 0, (int)w, (int)h, Fade((Color){255, 250, 238, 255}, branco));
    float preto = regras_cena_preto(t);
    if (preto > 0.0f) DrawRectangle(0, 0, (int)w, (int)h, Fade(BLACK, preto));
}
