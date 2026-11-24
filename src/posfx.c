// posfx.c — os render targets (cena HDR com profundidade e o quadro LDR), o shader de luz (PBR, textos em
// sombreadores.c), os mapas de material e o tone mapping. O VHS fica em vhs.c.
#include "posfx.h"
#include "vhs.h"
#include "sombreadores.h"
#include "rlgl.h"
#include "raymath.h"
#include <stddef.h>

static bool usa_glsl_100(void) {
    return rlGetVersion() == RL_OPENGL_ES_20;
}

// Unidades de textura que a raylib não usa (ela usa a 0 e, no lote, da 1 à 4): os mapas ficam ligados nelas.
#define UNIDADE_NORMAL 9
#define UNIDADE_ORM    10

// A cena HDR: cor em ponto flutuante (16 bits por canal: o que passa de 1 não satura) e profundidade numa
// textura (os passes de tela leem a distância de cada pixel).
static RenderTexture2D cria_hdr(int w, int h) {
    RenderTexture2D t = {0};
    t.id = rlLoadFramebuffer();
    if (t.id == 0) return t;
    rlEnableFramebuffer(t.id);
    t.texture = (Texture2D){rlLoadTexture(NULL, w, h, PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, 1), w, h, 1,
                            PIXELFORMAT_UNCOMPRESSED_R16G16B16A16};
    t.depth = (Texture2D){rlLoadTextureDepth(w, h, false), w, h, 1, 19};
    rlFramebufferAttach(t.id, t.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlFramebufferAttach(t.id, t.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    bool completo = rlFramebufferComplete(t.id);
    rlDisableFramebuffer();
    if (!completo) {
        TraceLog(LOG_WARNING, "PRECURSOR: a cena HDR não montou; seguindo em LDR");
        rlUnloadFramebuffer(t.id);
        return (RenderTexture2D){0};
    }
    rlTextureParameters(t.texture.id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_LINEAR);
    rlTextureParameters(t.texture.id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_LINEAR);
    return t;
}

static void libera_hdr(RenderTexture2D *t) {
    if (t->id == 0) return;
    rlUnloadTexture(t->texture.id);
    rlUnloadTexture(t->depth.id);
    rlUnloadFramebuffer(t->id);
    *t = (RenderTexture2D){0};
}

static void cria_alvo(Recursos *rec, int largura_janela, int altura_janela) {
    int w = (int)(largura_janela * ESCALA_RENDER);
    int h = (int)(altura_janela * ESCALA_RENDER);
    if (w < 64) w = 64;
    if (h < 36) h = 36;
    rec->alvo = LoadRenderTexture(w, h);
    rec->alvo_susto = LoadRenderTexture(w, h);
    SetTextureFilter(rec->alvo.texture, TEXTURE_FILTER_BILINEAR);
    rec->hdr = usa_glsl_100() ? (RenderTexture2D){0} : cria_hdr(w, h);
    rec->alvo_largura = w;
    rec->alvo_altura = h;
}

static void libera_alvo(Recursos *rec) {
    UnloadRenderTexture(rec->alvo);
    UnloadRenderTexture(rec->alvo_susto);
    libera_hdr(&rec->hdr);
}

static void carrega_tom(Recursos *rec) {
    rec->tom = usa_glsl_100() ? (Shader){0} : LoadShaderFromMemory(NULL, sombreador_tom_fs());
    rec->tom_ok = rec->tom.id != 0 && IsShaderValid(rec->tom);
    if (rec->tom_ok) rec->loc_tom_exposicao = GetShaderLocation(rec->tom, "exposicao");
}

static void carrega_luz(Recursos *rec) {
    bool v100 = usa_glsl_100();
    rec->luz = LoadShaderFromMemory(sombreador_luz_vs(v100), sombreador_luz_fs(v100));
    rec->luz_ok = IsShaderValid(rec->luz);
    if (!rec->luz_ok) {
        TraceLog(LOG_WARNING, "PRECURSOR: shader de luz não compilou; seguindo com cores chapadas");
        return;
    }
    rec->loc_luz_pos = GetShaderLocation(rec->luz, "luzPos");
    rec->loc_luz_cor = GetShaderLocation(rec->luz, "luzCor");
    rec->loc_luz_ambiente = GetShaderLocation(rec->luz, "ambiente");
    rec->loc_luz_nevoa = GetShaderLocation(rec->luz, "nevoa");
    rec->loc_luz_camera = GetShaderLocation(rec->luz, "camera");
    rec->loc_luz_modelo = GetShaderLocation(rec->luz, "matModel");
    rec->loc_luz_cone = GetShaderLocation(rec->luz, "luzCone");
    rec->loc_luz_queda = GetShaderLocation(rec->luz, "luzQueda");
    rec->loc_luz_contraluz = GetShaderLocation(rec->luz, "contraluz");
    rec->loc_luz_molhado = GetShaderLocation(rec->luz, "molhado");
    rec->loc_luz_usa_mapas = GetShaderLocation(rec->luz, "usaMapas");
    rec->loc_luz_rugosidade = GetShaderLocation(rec->luz, "rugosidade");
    rec->loc_luz_metalico = GetShaderLocation(rec->luz, "metalico");
    int normal = UNIDADE_NORMAL, orm = UNIDADE_ORM; // os samplers apontam sempre para as mesmas unidades
    SetShaderValue(rec->luz, GetShaderLocation(rec->luz, "mapaNormal"), &normal, SHADER_UNIFORM_INT);
    SetShaderValue(rec->luz, GetShaderLocation(rec->luz, "mapaOrm"), &orm, SHADER_UNIFORM_INT);
    rec->loc_luz_cela_min = GetShaderLocation(rec->luz, "celaMin");
    rec->loc_luz_cela_max = GetShaderLocation(rec->luz, "celaMax");
    rec->loc_luz_cela_densidade = GetShaderLocation(rec->luz, "celaDensidade");
    rec->loc_luz_cela_cor = GetShaderLocation(rec->luz, "celaCor");
    rec->loc_luz_cela_espalha = GetShaderLocation(rec->luz, "celaEspalha");
    rec->loc_luz_cela_lanterna = GetShaderLocation(rec->luz, "celaEspalhaLanterna");
}

void posfx_inicia(Recursos *rec) {
    cria_alvo(rec, GetScreenWidth(), GetScreenHeight());
    vhs_carrega(rec);
    carrega_luz(rec);
    carrega_tom(rec);
}

void posfx_finaliza(Recursos *rec) {
    libera_alvo(rec);
    vhs_descarrega(rec);
    if (rec->luz_ok) UnloadShader(rec->luz);
    if (rec->tom_ok) UnloadShader(rec->tom);
}

// A névoa da cela: a caixa vem do mapa (nivel.c); densidade e cor em config.h.
static void envia_nevoa_da_cela(const Estado *e) {
    const Recursos *r = &e->rec;
    float densidade = NEVOA_CELA_DENSIDADE, espalha = NEVOA_CELA_ESPALHA,
          lanterna = NEVOA_CELA_ESPALHA_LANTERNA;
    Vector3 cor = {NEVOA_CELA_COR_R, NEVOA_CELA_COR_G, NEVOA_CELA_COR_B};
    SetShaderValue(r->luz, r->loc_luz_cela_min, &e->nivel.cela_min, SHADER_UNIFORM_VEC3);
    SetShaderValue(r->luz, r->loc_luz_cela_max, &e->nivel.cela_max, SHADER_UNIFORM_VEC3);
    SetShaderValue(r->luz, r->loc_luz_cela_densidade, &densidade, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_cela_cor, &cor, SHADER_UNIFORM_VEC3);
    SetShaderValue(r->luz, r->loc_luz_cela_espalha, &espalha, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_cela_lanterna, &lanterna, SHADER_UNIFORM_FLOAT);
}

void posfx_comeca_luz(const Estado *e, const Vector3 *posicoes, const Vector3 *cores, const Vector4 *cones,
                      const float *quedas, int n, float ambiente) {
    const Recursos *r = &e->rec;
    if (!r->luz_ok) return;
    Vector3 pos[LUZES_SHADER] = {0}, cor[LUZES_SHADER] = {0}, sem_contraluz = {0};
    Vector4 cone[LUZES_SHADER];
    float queda[LUZES_SHADER];
    for (int i = 0; i < LUZES_SHADER; i++) { // padrão MAP: as luzes do quadro → os vetores do shader
        bool tem = i < n;
        pos[i] = tem ? posicoes[i] : (Vector3){0};
        cor[i] = tem ? cores[i] : (Vector3){0};
        cone[i] = tem ? cones[i] : (Vector4){0, -1, 0, -2.0f};
        queda[i] = tem ? quedas[i] : 1.0f;
    }
    float nevoa = NEVOA_DENSIDADE;
    SetShaderValueV(r->luz, r->loc_luz_pos, pos, SHADER_UNIFORM_VEC3, LUZES_SHADER);
    SetShaderValueV(r->luz, r->loc_luz_cor, cor, SHADER_UNIFORM_VEC3, LUZES_SHADER);
    SetShaderValueV(r->luz, r->loc_luz_cone, cone, SHADER_UNIFORM_VEC4, LUZES_SHADER);
    SetShaderValueV(r->luz, r->loc_luz_queda, queda, SHADER_UNIFORM_FLOAT, LUZES_SHADER);
    SetShaderValue(r->luz, r->loc_luz_contraluz, &sem_contraluz, SHADER_UNIFORM_VEC3);
    float seco = 0.0f;
    SetShaderValue(r->luz, r->loc_luz_molhado, &seco, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_ambiente, &ambiente, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_nevoa, &nevoa, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_camera, &e->camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(r->luz, r->loc_luz_modelo, MatrixIdentity());
    envia_nevoa_da_cela(e);
    posfx_superficie(e, POSFX_RUGOSIDADE_PADRAO, 0.0f);
    BeginShaderMode(r->luz);
}

void posfx_termina_luz(const Estado *e) {
    if (e->rec.luz_ok) EndShaderMode();
}

void posfx_retoma_luz(const Estado *e) {
    if (e->rec.luz_ok) BeginShaderMode(e->rec.luz);
}

void posfx_olho(const Estado *e, Vector3 olho) {
    const Recursos *r = &e->rec;
    if (r->luz_ok) SetShaderValue(r->luz, r->loc_luz_camera, &olho, SHADER_UNIFORM_VEC3);
}

// A geometria imediata (DrawCube...) fica num lote até ser enviada; esvaziar o lote antes de trocar o
// uniforme garante que a contraluz pegue só o que for desenhado depois desta chamada.
// Chame FORA de rlPushMatrix/rlPopMatrix: ao enviar o lote, a raylib manda a transformação atual como
// matModel.
void posfx_contraluz(const Estado *e, Color cor, float intensidade) {
    const Recursos *r = &e->rec;
    if (!r->luz_ok) return;
    rlDrawRenderBatchActive();
    Vector3 v = {cor.r / 255.0f * intensidade, cor.g / 255.0f * intensidade, cor.b / 255.0f * intensidade};
    SetShaderValue(r->luz, r->loc_luz_contraluz, &v, SHADER_UNIFORM_VEC3);
}

// O material da geometria que vem a seguir: liga a normal e o ORM dele nas unidades fixas. Sai o lote
// pendente antes (ele foi montado com o material anterior).
void posfx_material(const Estado *e, TipoMaterial m) {
    const Recursos *r = &e->rec;
    if (!r->luz_ok || r->material_normal[m].id == 0) return;
    rlDrawRenderBatchActive();
    rlActiveTextureSlot(UNIDADE_NORMAL);
    rlEnableTexture(r->material_normal[m].id);
    rlActiveTextureSlot(UNIDADE_ORM);
    rlEnableTexture(r->material_orm[m].id);
    rlActiveTextureSlot(0);
    float sim = 1.0f;
    SetShaderValue(r->luz, r->loc_luz_usa_mapas, &sim, SHADER_UNIFORM_FLOAT);
}

// Sem mapas: a superfície inteira com a mesma rugosidade e o mesmo metal (modelos, decalques, caixas soltas).
void posfx_superficie(const Estado *e, float rugosidade, float metal) {
    const Recursos *r = &e->rec;
    if (!r->luz_ok) return;
    rlDrawRenderBatchActive();
    float nao = 0.0f;
    SetShaderValue(r->luz, r->loc_luz_usa_mapas, &nao, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_rugosidade, &rugosidade, SHADER_UNIFORM_FLOAT);
    SetShaderValue(r->luz, r->loc_luz_metalico, &metal, SHADER_UNIFORM_FLOAT);
}

// Mesmo cuidado da contraluz: o lote pendente sai antes, com o brilho antigo.
void posfx_molhado(const Estado *e, float brilho) {
    const Recursos *r = &e->rec;
    if (!r->luz_ok) return;
    rlDrawRenderBatchActive();
    SetShaderValue(r->luz, r->loc_luz_molhado, &brilho, SHADER_UNIFORM_FLOAT);
}

void posfx_nevoa_cela(const Estado *e, float densidade) {
    const Recursos *r = &e->rec;
    if (!r->luz_ok) return;
    rlDrawRenderBatchActive();
    SetShaderValue(r->luz, r->loc_luz_cela_densidade, &densidade, SHADER_UNIFORM_FLOAT);
}

void posfx_atualiza(Estado *e, float dt) {
    (void)dt;
    int w = (int)(GetScreenWidth() * ESCALA_RENDER);
    int h = (int)(GetScreenHeight() * ESCALA_RENDER);
    if (w != e->rec.alvo_largura || h != e->rec.alvo_altura) {
        libera_alvo(&e->rec);
        cria_alvo(&e->rec, GetScreenWidth(), GetScreenHeight());
    }
}

// A cena 3D vai para o alvo HDR (sem ele, direto para o quadro LDR, como antes).
void posfx_inicia_cena(const Estado *e) {
    BeginTextureMode(e->rec.hdr.id != 0 ? e->rec.hdr : e->rec.alvo);
}

// Fecha a cena HDR e a passa para o quadro LDR com o tone mapping. Depois daqui, o que for desenhado (ripas
// do armário, o susto, os escurecimentos) cai direto no quadro final, sem tone mapping.
void posfx_resolve(const Estado *e) {
    const Recursos *r = &e->rec;
    if (r->hdr.id == 0) return;
    EndTextureMode();
    BeginTextureMode(r->alvo);
    ClearBackground(BLACK);
    float exposicao = EXPOSICAO;
    if (r->tom_ok) {
        SetShaderValue(r->tom, r->loc_tom_exposicao, &exposicao, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(r->tom);
    }
    Rectangle origem = {0, 0, (float)r->hdr.texture.width, -(float)r->hdr.texture.height};
    DrawTexturePro(r->hdr.texture, origem, (Rectangle){0, 0, (float)r->alvo_largura, (float)r->alvo_altura},
                   (Vector2){0, 0}, 0.0f, WHITE);
    if (r->tom_ok) EndShaderMode();
}

void posfx_termina_cena(const Estado *e) {
    (void)e;
    EndTextureMode();
}

void posfx_desenha(const Estado *e) {
    const Recursos *r = &e->rec;
    // A textura do render target fica de cabeça para baixo: altura negativa desvira.
    Rectangle origem = {0, 0, (float)r->alvo.texture.width, -(float)r->alvo.texture.height};
    Rectangle destino = {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
    bool com_vhs = vhs_comeca(e);
    DrawTexturePro(r->alvo.texture, origem, destino, (Vector2){0, 0}, 0.0f, WHITE);
    if (com_vhs) vhs_termina(e);
}
