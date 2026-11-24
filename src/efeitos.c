// efeitos.c — a sombra da lanterna e os passes de tela da cena HDR (ver efeitos.h). Os recursos de GPU
// (alvos e shaders) são estado deste módulo: criados em efeitos_carrega, refeitos quando o tamanho muda.
#include "efeitos.h"
#include "posfx.h"
#include "qualidade.h"
#include "sombreadores.h"
#include "rlgl.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

#define NIVEIS_BLOOM   5
#define UNIDADE_SOMBRA 12 // a unidade de textura fixa do mapa de sombra no shader de luz

typedef struct {
    Shader s;
    bool ok;
} Passe;

static struct {
    bool ativo; // false: sem HDR (OpenGL ES 2) ou algum shader não compilou
    Passe ssao, volume, desce, sobe, compoe;
    RenderTexture2D alvo_ssao, alvo_volume, bloom[NIVEIS_BLOOM], sombra;
    int sombra_px;
    // o quadro: câmera e luzes
    Matrix proj, vista;
    Vector3 camera;
    Vector3 pos[LUZES_SHADER], cor[LUZES_SHADER];
    Vector4 cone[LUZES_SHADER];
    float queda[LUZES_SHADER];
    int n, lanterna;
    Matrix mat_sombra;
    bool tem_sombra;
    // locais no shader de luz
    int loc_modo, loc_luz_sombra, loc_mat_sombra, loc_usa_sombra;
} fx;

static Passe carrega(const char *fs) {
    Passe p = {LoadShaderFromMemory(NULL, fs), false};
    p.ok = p.s.id != 0 && IsShaderValid(p.s);
    if (!p.ok) TraceLog(LOG_WARNING, "PRECURSOR: um passe de tela não compilou; os efeitos ficam desligados");
    return p;
}

static int loc(Passe p, const char *nome) {
    return GetShaderLocation(p.s, nome);
}

// ------------------------------------------------------------------ alvos
static void libera_alvos(void) {
    posfx_libera_alvo_float(&fx.alvo_ssao);
    posfx_libera_alvo_float(&fx.alvo_volume);
    for (int i = 0; i < NIVEIS_BLOOM; i++) // liberação: um alvo por nível
        posfx_libera_alvo_float(&fx.bloom[i]);
}

static void cria_alvos(const Recursos *rec) {
    const Qualidade *q = qualidade();
    int w = rec->alvo_largura, h = rec->alvo_altura;
    const int f16 = PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;
    if (q->ssao) fx.alvo_ssao = posfx_cria_alvo_float(w / 2, h / 2, f16, 0);
    int vw = (int)(w * q->volume_escala), vh = (int)(h * q->volume_escala);
    if (q->volume_passos > 0)
        fx.alvo_volume = posfx_cria_alvo_float(vw > 8 ? vw : 8, vh > 8 ? vh : 8, f16, 0);
    for (int i = 0; q->bloom && i < NIVEIS_BLOOM; i++) { // padrão MAP: cada nível com a metade do anterior
        int bw = w >> (i + 1), bh = h >> (i + 1);
        fx.bloom[i] = posfx_cria_alvo_float(bw > 2 ? bw : 2, bh > 2 ? bh : 2, f16, 0);
    }
}

static void cria_sombra(void) {
    int px = qualidade()->sombra_px;
    if (fx.sombra.id != 0 && fx.sombra_px == px) return;
    posfx_libera_alvo_float(&fx.sombra);
    fx.sombra_px = px;
    if (px > 0) fx.sombra = posfx_cria_alvo_float(px, px, PIXELFORMAT_UNCOMPRESSED_R32, 2);
}

void efeitos_carrega(Recursos *rec) {
    memset(&fx, 0, sizeof(fx));
    if (rec->hdr.id == 0 || !rec->luz_ok) return;
    fx.ssao = carrega(sombreador_ssao_fs());
    fx.volume = carrega(sombreador_volume_fs());
    fx.desce = carrega(sombreador_bloom_desce_fs());
    fx.sobe = carrega(sombreador_bloom_sobe_fs());
    fx.compoe = carrega(sombreador_compoe_fs());
    fx.ativo = fx.ssao.ok && fx.volume.ok && fx.desce.ok && fx.sobe.ok && fx.compoe.ok;
    fx.loc_modo = GetShaderLocation(rec->luz, "modoSombra");
    fx.loc_luz_sombra = GetShaderLocation(rec->luz, "luzSombra");
    fx.loc_mat_sombra = GetShaderLocation(rec->luz, "matSombra");
    fx.loc_usa_sombra = GetShaderLocation(rec->luz, "usaSombra");
    int unidade = UNIDADE_SOMBRA;
    SetShaderValue(rec->luz, GetShaderLocation(rec->luz, "mapaSombra"), &unidade, SHADER_UNIFORM_INT);
    if (!fx.ativo) return;
    cria_alvos(rec);
    cria_sombra();
}

void efeitos_descarrega(void) {
    libera_alvos();
    posfx_libera_alvo_float(&fx.sombra);
    Passe *p[] = {&fx.ssao, &fx.volume, &fx.desce, &fx.sobe, &fx.compoe};
    for (int i = 0; i < 5; i++) // liberação: cada shader que compilou
        if (p[i]->ok) UnloadShader(p[i]->s);
    fx.ativo = false;
}

void efeitos_redimensiona(const Recursos *rec) {
    if (!fx.ativo) return;
    libera_alvos();
    cria_alvos(rec);
    cria_sombra();
}

// ------------------------------------------------------------------ o quadro
void efeitos_guarda_luzes(const Vector3 *pos, const Vector3 *cor, const Vector4 *cone, const float *queda,
                          int n, int lanterna) {
    fx.n = n < LUZES_SHADER ? n : LUZES_SHADER;
    for (int i = 0; i < fx.n; i++) { // padrão MAP: cópia das luzes do quadro para a névoa
        fx.pos[i] = pos[i];
        fx.cor[i] = cor[i];
        fx.cone[i] = cone[i];
        fx.queda[i] = queda[i];
    }
    fx.lanterna = lanterna;
}

void efeitos_guarda_camera(void) {
    fx.proj = rlGetMatrixProjection();
    fx.vista = rlGetMatrixModelview();
    Matrix inv = MatrixInvert(fx.vista);
    fx.camera = (Vector3){inv.m12, inv.m13, inv.m14};
}

// ------------------------------------------------------------------ sombra da lanterna
bool efeitos_comeca_sombra(const Estado *e, Vector3 pos, Vector3 direcao) {
    const Recursos *r = &e->rec;
    if (!fx.ativo || fx.sombra.id == 0) return false;
    EndTextureMode();
    rlActiveTextureSlot(UNIDADE_SOMBRA); // o mapa não pode ser lido enquanto é o alvo (o driver para tudo)
    rlEnableTexture(0);
    rlActiveTextureSlot(0);
    BeginTextureMode(fx.sombra);
    ClearBackground(WHITE); // longe: nada faz sombra
    Vector3 cima = fabsf(direcao.y) > 0.95f ? (Vector3){1, 0, 0} : (Vector3){0, 1, 0};
    float abertura = 2.0f * LANTERNA_ABERTURA_GRAUS + 12.0f; // o cone e uma folga
    Camera3D luz = {pos, Vector3Add(pos, direcao), cima, abertura, CAMERA_PERSPECTIVE};
    BeginMode3D(luz);
    fx.mat_sombra = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    float sim = 1.0f;
    SetShaderValue(r->luz, fx.loc_modo, &sim, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, fx.loc_luz_sombra, &pos, SHADER_UNIFORM_VEC3);
    BeginShaderMode(r->luz);
    return true;
}

void efeitos_termina_sombra(const Estado *e) {
    const Recursos *r = &e->rec;
    EndShaderMode();
    EndMode3D();
    EndTextureMode();
    float nao = 0.0f, sim = 1.0f;
    SetShaderValue(r->luz, fx.loc_modo, &nao, SHADER_UNIFORM_FLOAT);
    SetShaderValueMatrix(r->luz, fx.loc_mat_sombra, fx.mat_sombra);
    SetShaderValue(r->luz, fx.loc_usa_sombra, &sim, SHADER_UNIFORM_FLOAT);
    rlActiveTextureSlot(UNIDADE_SOMBRA);
    rlEnableTexture(fx.sombra.texture.id);
    rlActiveTextureSlot(0);
    fx.tem_sombra = true;
    BeginTextureMode(r->hdr);
}

void efeitos_sem_sombra(const Estado *e) {
    float nao = 0.0f;
    if (e->rec.luz_ok) SetShaderValue(e->rec.luz, fx.loc_usa_sombra, &nao, SHADER_UNIFORM_FLOAT);
    fx.tem_sombra = false;
}

// ------------------------------------------------------------------ passes de tela
// Um retângulo na tela inteira de `destino` com o shader `p` e `entrada` em texture0 (os samplers extras
// entram em `extras`, depois de abrir o shader: a raylib liga eles no envio do lote).
static void desenha_passe(Passe p, Texture2D entrada, RenderTexture2D destino, int n_extras, const int *locs,
                          const Texture2D *extras) {
    BeginTextureMode(destino);
    ClearBackground(BLACK);
    BeginShaderMode(p.s);
    for (int i = 0; i < n_extras; i++) // padrão MAP: cada sampler extra na sua unidade
        SetShaderValueTexture(p.s, locs[i], extras[i]);
    Rectangle todo = {0, 0, (float)destino.texture.width, (float)destino.texture.height};
    Rectangle origem = {0, 0, (float)entrada.width, -(float)entrada.height};
    DrawTexturePro(entrada, origem, todo, (Vector2){0, 0}, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
}

static void passe_ssao(const Recursos *r) {
    Matrix proj_inv = MatrixInvert(fx.proj);
    float raio = 0.45f;
    SetShaderValueMatrix(fx.ssao.s, loc(fx.ssao, "projInv"), proj_inv);
    SetShaderValueMatrix(fx.ssao.s, loc(fx.ssao, "proj"), fx.proj);
    SetShaderValue(fx.ssao.s, loc(fx.ssao, "raio"), &raio, SHADER_UNIFORM_FLOAT);
    desenha_passe(fx.ssao, r->hdr.depth, fx.alvo_ssao, 0, NULL, NULL);
}

static void passe_volume(const Estado *e) {
    const Recursos *r = &e->rec;
    Shader s = fx.volume.s;
    Matrix vp_inv = MatrixInvert(MatrixMultiply(fx.vista, fx.proj));
    float densidade = NEVOA_VOLUME_DENSIDADE, tempo = fmodf(e->tempo_total, 1000.0f), usa = fx.tem_sombra;
    bool energia = !regras_fase_eh_saida(e->fase) || e->painel.resolvido;
    float a = NEVOA_VOLUME_AMBIENTE; // com energia, a névoa clareia fria; sem ela, um resto de vermelho
    Vector3 ambiente = energia ? (Vector3){a * 0.6f, a, a * 0.9f} : (Vector3){a * 0.3f, a * 0.15f, a * 0.15f};
    int passos = qualidade()->volume_passos;
    SetShaderValueMatrix(s, loc(fx.volume, "vpInv"), vp_inv);
    SetShaderValue(s, loc(fx.volume, "camera"), &fx.camera, SHADER_UNIFORM_VEC3);
    SetShaderValueV(s, loc(fx.volume, "luzPos"), fx.pos, SHADER_UNIFORM_VEC3, LUZES_SHADER);
    SetShaderValueV(s, loc(fx.volume, "luzCor"), fx.cor, SHADER_UNIFORM_VEC3, LUZES_SHADER);
    SetShaderValueV(s, loc(fx.volume, "luzCone"), fx.cone, SHADER_UNIFORM_VEC4, LUZES_SHADER);
    SetShaderValueV(s, loc(fx.volume, "luzQueda"), fx.queda, SHADER_UNIFORM_FLOAT, LUZES_SHADER);
    SetShaderValue(s, loc(fx.volume, "nLuzes"), &fx.n, SHADER_UNIFORM_INT);
    SetShaderValue(s, loc(fx.volume, "indiceLanterna"), &fx.lanterna, SHADER_UNIFORM_INT);
    SetShaderValueMatrix(s, loc(fx.volume, "matSombra"), fx.mat_sombra);
    SetShaderValue(s, loc(fx.volume, "usaSombra"), &usa, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.volume, "densidade"), &densidade, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.volume, "tempo"), &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.volume, "corAmbiente"), &ambiente, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, loc(fx.volume, "passos"), &passos, SHADER_UNIFORM_INT);
    int l = loc(fx.volume, "mapaSombra");
    Texture2D sombra = fx.sombra.texture;
    desenha_passe(fx.volume, r->hdr.depth, fx.alvo_volume, fx.sombra.id != 0 ? 1 : 0, &l, &sombra);
}

// Bloom: desce (com o corte só no primeiro nível) e sobe somando cada nível ao de cima.
static void passe_bloom(const Recursos *r) {
    Texture2D de = r->hdr.texture;
    for (int i = 0; i < NIVEIS_BLOOM; i++) { // laço de passes: um por nível (não é map/filter/fold)
        Vector2 texel = {0.5f / (float)de.width, 0.5f / (float)de.height};
        float corte = i == 0 ? BLOOM_CORTE : 0.0f;
        SetShaderValue(fx.desce.s, loc(fx.desce, "texel"), &texel, SHADER_UNIFORM_VEC2);
        SetShaderValue(fx.desce.s, loc(fx.desce, "corte"), &corte, SHADER_UNIFORM_FLOAT);
        desenha_passe(fx.desce, de, fx.bloom[i], 0, NULL, NULL);
        de = fx.bloom[i].texture;
    }
    for (int i = NIVEIS_BLOOM - 2; i >= 0; i--) { // laço de passes: soma o nível de baixo neste
        Texture2D baixo = fx.bloom[i + 1].texture;
        Vector2 texel = {1.0f / (float)baixo.width, 1.0f / (float)baixo.height};
        SetShaderValue(fx.sobe.s, loc(fx.sobe, "texel"), &texel, SHADER_UNIFORM_VEC2);
        BeginTextureMode(fx.bloom[i]);
        BeginBlendMode(BLEND_ADDITIVE);
        BeginShaderMode(fx.sobe.s);
        DrawTexturePro(baixo, (Rectangle){0, 0, (float)baixo.width, -(float)baixo.height},
                       (Rectangle){0, 0, (float)fx.bloom[i].texture.width, (float)fx.bloom[i].texture.height},
                       (Vector2){0, 0}, 0.0f, WHITE);
        EndShaderMode();
        EndBlendMode();
        EndTextureMode();
    }
}

static void envia_composicao(const Estado *e) {
    const Qualidade *q = qualidade();
    Shader s = fx.compoe.s;
    float exposicao = EXPOSICAO, usa_ao = q->ssao, usa_vol = q->volume_passos > 0;
    float bloom = q->bloom ? BLOOM_FORCA : 0.0f;
    float sat = q->pos ? GRADACAO_SATURACAO : 1.0f, contraste = q->pos ? GRADACAO_CONTRASTE : 1.0f;
    float vinheta = q->pos ? GRADACAO_VINHETA : 0.0f, grao = q->pos ? GRADACAO_GRAO : 0.0f;
    float aberracao = q->pos ? GRADACAO_ABERRACAO : 0.0f, tempo = fmodf(e->tempo_total, 100.0f);
    Vector3 sombra = q->pos ? (Vector3)GRADACAO_SOMBRA : (Vector3){1, 1, 1};
    Vector3 luz = q->pos ? (Vector3)GRADACAO_LUZ : (Vector3){1, 1, 1};
    Vector2 tela = {(float)e->rec.alvo_largura, (float)e->rec.alvo_altura};
    SetShaderValue(s, loc(fx.compoe, "exposicao"), &exposicao, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "usaAo"), &usa_ao, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "usaVolume"), &usa_vol, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "bloom"), &bloom, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "saturacao"), &sat, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "contraste"), &contraste, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "vinheta"), &vinheta, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "grao"), &grao, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "aberracao"), &aberracao, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "tempo"), &tempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, loc(fx.compoe, "sombraCor"), &sombra, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, loc(fx.compoe, "luzCor"), &luz, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, loc(fx.compoe, "tela"), &tela, SHADER_UNIFORM_VEC2);
}

bool efeitos_compoe(const Estado *e) {
    const Recursos *r = &e->rec;
    if (!fx.ativo) return false;
    const Qualidade *q = qualidade();
    EndTextureMode(); // fecha a cena HDR
    if (q->ssao && fx.alvo_ssao.id != 0) passe_ssao(r);
    if (q->volume_passos > 0 && fx.alvo_volume.id != 0) passe_volume(e);
    if (q->bloom && fx.bloom[0].id != 0) passe_bloom(r);
    BeginTextureMode(r->alvo);
    ClearBackground(BLACK);
    BeginShaderMode(fx.compoe.s);
    envia_composicao(e);
    SetShaderValueTexture(fx.compoe.s, loc(fx.compoe, "mapaAo"), fx.alvo_ssao.texture);
    SetShaderValueTexture(fx.compoe.s, loc(fx.compoe, "mapaVolume"), fx.alvo_volume.texture);
    SetShaderValueTexture(fx.compoe.s, loc(fx.compoe, "mapaBloom"), fx.bloom[0].texture);
    Rectangle origem = {0, 0, (float)r->hdr.texture.width, -(float)r->hdr.texture.height};
    Rectangle destino = {0, 0, (float)r->alvo_largura, (float)r->alvo_altura};
    DrawTexturePro(r->hdr.texture, origem, destino, (Vector2){0, 0}, 0.0f, WHITE);
    EndShaderMode();
    return true;
}
