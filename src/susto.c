// susto.c — o jumpscare. Com o modelo, o rosto do Precursor pula de longe até colado na câmera e fica
// tremendo; sem ele, o rosto procedural (a fenda) faz o mesmo em 2D. O movimento vem de regras_quadro_susto.
#include "susto.h"
#include "audio_sintese.h"
#include "posfx.h"
#include <math.h>

// ================================================================== estado
float susto_duracao(const Estado *e) {
    return e->rec.susto_s > 0.0f ? e->rec.susto_s : JUMPSCARE_S;
}

void susto_inicia(Estado *e, EventoFase depois) {
    if (susto_ativo(e)) return;
    e->silencio_t = JUMPSCARE_SILENCIO_S; // silêncio antes do grito
    e->jumpscare_t = susto_duracao(e);
    e->evento_pos_susto = depois;
    audio_silencia(e);
}

bool susto_ativo(const Estado *e) {
    return e->silencio_t > 0.0f || e->jumpscare_t > 0.0f;
}

// O grito começa junto com o rosto; o eco que sobra no fim do som continua na tela seguinte.
void susto_atualiza(Estado *e, float dt) {
    if (e->silencio_t > 0.0f) {
        e->silencio_t -= dt;
        if (e->silencio_t <= 0.0f) audio_toca(e, SOM_JUMPSCARE, 1.0f, 1.0f);
        return;
    }
    if (e->jumpscare_t <= 0.0f) return;
    e->jumpscare_t -= dt;
    if (e->jumpscare_t > 0.0f) return;
    e->jumpscare_t = 0.0f;
    if (e->evento_pos_susto != EV_NENHUM) e->evento = e->evento_pos_susto;
    e->evento_pos_susto = EV_NENHUM;
}

// ================================================================== desenho
bool susto_rosto_na_tela(const Estado *e) {
    return e->jumpscare_t > 0.0f && e->silencio_t <= 0.0f;
}

// Segundos desde que o rosto apareceu (o silêncio não conta).
static float tempo_do_rosto(const Estado *e) {
    return susto_duracao(e) - e->jumpscare_t;
}

// Luz branca piscando sem ritmo de relógio: acesa, fraca, acesa...
static float pisca(float t) {
    return sinf(t * 53.0f) * sinf(t * 31.0f) > -0.2f ? 1.0f : 0.3f;
}

// Bordas mais escuras, a luz falhando (tudo escurece junto) e o clarão quando ele chega.
static void desenha_efeitos(const Estado *e, QuadroSusto q, float t) {
    int w = e->rec.alvo_largura, h = e->rec.alvo_altura;
    DrawRectangleGradientV(0, 0, w, h / 4, Fade(BLACK, 0.9f), Fade(BLACK, 0.0f));
    DrawRectangleGradientV(0, h - h / 4, w, h / 4, Fade(BLACK, 0.0f), Fade(BLACK, 0.9f));
    DrawRectangleGradientH(0, 0, w / 5, h, Fade(BLACK, 0.85f), Fade(BLACK, 0.0f));
    DrawRectangleGradientH(w - w / 5, 0, w / 5, h, Fade(BLACK, 0.0f), Fade(BLACK, 0.85f));
    if (pisca(t) < 1.0f) DrawRectangle(0, 0, w, h, Fade(BLACK, 0.35f));
    if (q.clarao > 0.0f) DrawRectangle(0, 0, w, h, Fade(WHITE, 0.5f * q.clarao));
}

// ================================================================== o modelo (render target próprio)
// O rosto fica na origem, olhando para +Z; a câmera treme e gira em volta dele.
static Camera camera_do_susto(QuadroSusto q) {
    float giro = q.giro_graus * DEG2RAD;
    Camera c = {0};
    c.position = (Vector3){q.tremor_x, q.tremor_y, q.distancia};
    c.target = (Vector3){q.tremor_x * 0.4f, q.tremor_y * 0.4f, 0.0f};
    c.up = (Vector3){sinf(giro), cosf(giro), 0.0f};
    c.fovy = 70.0f; // a mesma do jogador
    c.projection = CAMERA_PERSPECTIVE;
    return c;
}

// Luz branca de baixo (piscando), um fio branco por trás e quase nada de ambiente: o rosto sai do escuro.
static void acende_luzes(const Estado *e, Vector3 olho, float t) {
    float p = JUMPSCARE_LUZ * pisca(t);
    Vector3 pos[3] = {{0.0f, -0.45f, 0.55f}, {0.35f, 0.7f, -0.4f}, {0.0f, 0.15f, 1.2f}};
    Vector3 cor[3] = {{p, p, p * 0.95f}, {0.35f, 0.35f, 0.38f}, {0.12f, 0.12f, 0.13f}};
    Vector4 cone[3] = {{0, -1, 0, -2.0f}, {0, -1, 0, -2.0f}, {0, -1, 0, -2.0f}}; // pontuais
    float queda[3] = {1.0f, 1.0f, 1.0f};
    posfx_comeca_luz(e, pos, cor, cone, queda, NULL, 3, -1, 0.02f);
    posfx_saida_direta(e); // o rosto vai para um alvo LDR próprio: sem passar pelo tone mapping da cena
    posfx_olho(e, olho);
}

// Onde fica o rosto do modelo (fração da caixa, em modelos_criatura.c) e a escala do jogo.
static Vector3 rosto_do_modelo(const Model *m, float *escala) {
    BoundingBox b = GetModelBoundingBox(*m);
    Vector3 tam = Vector3Subtract(b.max, b.min), f = esqueleto_modelo()->rosto;
    *escala = CRIATURA_ALTURA_M / fmaxf(0.01f, tam.y);
    return (Vector3){b.min.x + tam.x * f.x, b.min.y + tam.y * f.y, b.min.z + tam.z * f.z};
}

// O modelo na altura do jogo, deslocado para o rosto cair na origem.
static void desenha_modelo_no_rosto(const Estado *e) {
    const Model *m = &e->rec.modelo_precursor;
    float escala;
    Vector3 rosto = rosto_do_modelo(m, &escala);
    DrawModelEx(*m, Vector3Scale(rosto, -escala), (Vector3){0, 1, 0}, 0.0f, (Vector3){escala, escala, escala},
                COR_PELE);
}

// Os olhos do rosto liso (meios-discos, modelos_criatura.c), sem o fio de contraluz do crânio.
static void desenha_marcas_no_rosto(const Estado *e) {
    const ModeloCriatura *mc = esqueleto_modelo();
    float escala;
    Vector3 rosto = rosto_do_modelo(&e->rec.modelo_precursor, &escala), p[MARCA_PONTOS];
    for (int i = 0; i < mc->n_marcas; i++) { // padrão MAP: cada olho, na pose de repouso
        esqueleto_marca_pontos(&mc->marcas[i], p);
        for (int k = 0; k < MARCA_PONTOS; k++) // padrão MAP: cada ponto do meio-disco
            p[k] = Vector3Scale(Vector3Subtract(p[k], rosto), escala);
        for (int k = 1; k + 1 < MARCA_PONTOS; k++) { // laço de desenho: o leque, nos dois sentidos
            DrawTriangle3D(p[0], p[k], p[k + 1], COR_MARCA_ROSTO);
            DrawTriangle3D(p[0], p[k + 1], p[k], COR_MARCA_ROSTO);
        }
    }
}

// O rosto num render target só dele, transparente e com a profundidade própria: depois ele é colado por
// cima do cenário sem atravessar as paredes. Chamado antes de posfx_inicia_cena.
void susto_prepara(const Estado *e) {
    if (!susto_rosto_na_tela(e) || !e->rec.tem_modelo) return;
    float t = tempo_do_rosto(e);
    Camera c = camera_do_susto(regras_quadro_susto(t));
    BeginTextureMode(e->rec.alvo_susto);
    ClearBackground(BLANK);
    BeginMode3D(c);
    acende_luzes(e, c.position, t);
    posfx_contraluz(e, WHITE, 0.2f); // recorta o crânio com um fio de luz branca
    desenha_modelo_no_rosto(e);
    posfx_contraluz(e, WHITE, 0.0f);
    desenha_marcas_no_rosto(e);
    posfx_termina_luz(e);
    EndMode3D();
    EndTextureMode();
}

// ================================================================== sem o modelo
// O rosto procedural: elipse com a fenda aberta, crescendo com o pulo e tremendo junto.
static void desenha_rosto_2d(const Estado *e, QuadroSusto q) {
    float w = (float)e->rec.alvo_largura, h = (float)e->rec.alvo_altura;
    float perto = JUMPSCARE_PERTO_M / q.distancia; // ~0,1 longe, 1 colado
    int cx = (int)(w * 0.5f + q.tremor_x / JUMPSCARE_TREMOR_M * w * 0.015f);
    int cy = (int)(h * 0.58f + q.tremor_y / JUMPSCARE_TREMOR_M * h * 0.015f);
    float rx = w * 0.37f * perto, ry = h * 0.86f * perto;
    for (int i = 0; i < 6; i++) { // sombreado: as bordas do rosto mais escuras
        float f = 1.0f - i * 0.12f;
        DrawEllipse(cx, cy, rx * f, ry * f, ColorBrightness(COR_PELE, -0.72f + i * 0.07f));
    }
    float abre = w * 0.085f * perto;
    DrawEllipse(cx, cy, abre * 1.6f, h * 0.46f * perto, (Color){60, 8, 14, 255}); // borda viva da fenda
    DrawEllipse(cx, cy, abre, h * 0.43f * perto, (Color){10, 0, 2, 255});         // a fenda aberta
    for (int i = -4; i <= 4; i++) {
        float x = cx + i * abre * 0.22f;
        DrawLineEx((Vector2){x, cy - h * 0.34f * perto},
                   (Vector2){x + sinf(i * 1.7f) * 3.0f, cy + h * 0.34f * perto}, 1.0f,
                   (Color){120, 30, 34, 170});
    }
}

// Dentro do render target da cena: o cenário onde o jogador estava, escurecido, e o rosto por cima.
void susto_desenha(const Estado *e) {
    if (!susto_rosto_na_tela(e)) return;
    float t = tempo_do_rosto(e);
    QuadroSusto q = regras_quadro_susto(t);
    int w = e->rec.alvo_largura, h = e->rec.alvo_altura;
    DrawRectangle(0, 0, w, h, Fade(BLACK, JUMPSCARE_ESCURECE));
    if (e->rec.tem_modelo) // a textura do render target fica de cabeça para baixo: altura negativa desvira
        DrawTextureRec(e->rec.alvo_susto.texture, (Rectangle){0, 0, (float)w, -(float)h}, (Vector2){0, 0},
                       WHITE);
    else
        desenha_rosto_2d(e, q);
    desenha_efeitos(e, q, t);
}
