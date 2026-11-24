// rig.c — os pesos do skinning, calculados uma vez ao carregar o precursor.glb (o arquivo não tem ossos):
//   1. solda os vértices repetidos nas costuras de UV (mesma posição = mesmo ponto da pele);
//   2. cada ponto pega o osso mais perto (distância ao segmento menos a espessura da carne) e mistura com o
//      pai e os filhos dele, perto das articulações;
//   3. os pesos são suavizados pela SUPERFÍCIE (vizinhos na malha), só entre ossos vizinhos na hierarquia;
//   4. o queixo passa para a mandíbula; ficam os 4 ossos mais fortes de cada vértice, que vão para a GPU;
//   5. no modelo, a ponta dos dedos esquerdos está fundida à canela: os triângulos que ligam partes longe
//      uma da outra no esqueleto viram triângulos vazios (senão esticariam como uma teia quando a mão sobe).
// Alocações só aqui (no carregamento) e liberadas no fim; nada por quadro.
#include "rig.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INFLUENCIAS      4
#define PASSADAS_SUAVIZA 6
#define PESO_ESPALHA     0.025f // unidades do modelo: quanto a mistura entre dois ossos se espalha
#define ALTURA_NOMINAL   1.993f // o precursor.glb vai de y = −0,997 a 0,996
#define BASE_NOMINAL     -0.997f

typedef struct {
    int *inicio; // CSR: os vizinhos do ponto i ficam em viz[inicio[i] .. inicio[i + 1])
    int *viz;
} Grafo;

// ================================================================== 1. soldar
static const Vector3 *ordena_v; // o qsort não recebe contexto (só no carregamento)

static long chave(float x) {
    return lroundf(x * 1e4f);
}

static int compara(const void *pa, const void *pb) {
    Vector3 a = ordena_v[*(const int *)pa], b = ordena_v[*(const int *)pb];
    long d[3] = {chave(a.x) - chave(b.x), chave(a.y) - chave(b.y), chave(a.z) - chave(b.z)};
    for (int k = 0; k < 3; k++) // padrão FOLD: o primeiro eixo que difere decide
        if (d[k] != 0) return d[k] < 0 ? -1 : 1;
    return 0;
}

// id[i] = o ponto soldado do vértice i; rep[p] = um vértice do ponto p. Devolve quantos pontos.
static int solda(const Vector3 *v, int n, int *id, int *rep) {
    int *ordem = malloc((size_t)n * sizeof(int));
    if (ordem == NULL) return -1;
    for (int i = 0; i < n; i++) // padrão MAP: a ordem inicial
        ordem[i] = i;
    ordena_v = v;
    qsort(ordem, (size_t)n, sizeof(int), compara);
    int pontos = 0;
    for (int k = 0; k < n; k++) { // padrão MAP: posição nova = ponto novo
        if (k == 0 || compara(&ordem[k - 1], &ordem[k]) != 0) rep[pontos++] = ordem[k];
        id[ordem[k]] = pontos - 1;
    }
    free(ordem);
    return pontos;
}

// ================================================================== 2. grafo da superfície
static int vertice(const Mesh *m, int t, int k) {
    return m->indices != NULL ? m->indices[3 * t + k] : 3 * t + k;
}

static bool monta_grafo(const Mesh *m, const int *id, int pontos, Grafo *g) {
    g->inicio = calloc((size_t)pontos + 1, sizeof(int));
    g->viz = malloc((size_t)m->triangleCount * 6 * sizeof(int));
    int *cursor = malloc((size_t)pontos * sizeof(int));
    if (g->inicio == NULL || g->viz == NULL || cursor == NULL) {
        free(cursor);
        return false;
    }
    for (int t = 0; t < m->triangleCount; t++) // padrão MAP: cada canto do triângulo ganha 2 vizinhos
        for (int k = 0; k < 3; k++)            // padrão MAP: os três cantos
            g->inicio[id[vertice(m, t, k)] + 1] += 2;
    for (int p = 0; p < pontos; p++) // padrão FOLD: soma acumulada
        g->inicio[p + 1] += g->inicio[p];
    memcpy(cursor, g->inicio, (size_t)pontos * sizeof(int));
    for (int t = 0; t < m->triangleCount; t++) { // padrão MAP: as três arestas, nos dois sentidos
        int c[3] = {id[vertice(m, t, 0)], id[vertice(m, t, 1)], id[vertice(m, t, 2)]};
        for (int k = 0; k < 3; k++) { // padrão MAP: os dois vizinhos de cada canto
            g->viz[cursor[c[k]]++] = c[(k + 1) % 3];
            g->viz[cursor[c[k]]++] = c[(k + 2) % 3];
        }
    }
    free(cursor);
    return true;
}

// ================================================================== 3. pesos
static float distancia_segmento(Vector3 p, Vector3 a, Vector3 b) {
    Vector3 ab = Vector3Subtract(b, a);
    float t =
        Clamp(Vector3DotProduct(Vector3Subtract(p, a), ab) / fmaxf(1e-8f, Vector3DotProduct(ab, ab)), 0, 1);
    return Vector3Distance(p, Vector3Add(a, Vector3Scale(ab, t)));
}

static bool vizinhos_de_osso(int a, int b) {
    return a == b || esqueleto_osso(a)->pai == b || esqueleto_osso(b)->pai == a;
}

// Devolve o osso mais perto (o "dono" do ponto).
static int pesos_do_ponto(Vector3 p, float *w) {
    float d[OSSOS_TOTAL], menor = 1e9f, soma = 0.0f;
    int perto = 0;
    for (int b = 0; b < OSSOS_TOTAL; b++) { // padrão FOLD: o osso mais perto (a mandíbula vem depois)
        const OssoRepouso *o = esqueleto_osso(b);
        d[b] = b == OSSO_MANDIBULA ? 1e9f : distancia_segmento(p, o->cabeca, o->cauda) - o->raio;
        if (d[b] < menor) menor = d[b], perto = b;
    }
    for (int b = 0; b < OSSOS_TOTAL; b++) { // padrão MAP: mistura só com o pai e os filhos dele
        w[b] =
            vizinhos_de_osso(b, perto) && b != OSSO_MANDIBULA ? expf(-(d[b] - menor) / PESO_ESPALHA) : 0.0f;
        soma += w[b];
    }
    for (int b = 0; b < OSSOS_TOTAL; b++) // padrão MAP: normaliza
        w[b] /= soma;
    return perto;
}

// Só fica peso dos ossos vizinhos do dono: a suavização não atravessa de uma parte para outra.
static void mascara(float *w, int dono) {
    float soma = 0.0f;
    for (int b = 0; b < OSSOS_TOTAL; b++) { // padrão MAP: zera o que não é vizinho do dono
        if (!vizinhos_de_osso(b, dono)) w[b] = 0.0f;
        soma += w[b];
    }
    for (int b = 0; b < OSSOS_TOTAL; b++) // padrão MAP: normaliza
        w[b] = soma > 0.0f ? w[b] / soma : (b == dono ? 1.0f : 0.0f);
}

static void suaviza(const Grafo *g, int pontos, const int *dono, float *w, float *aux) {
    for (int passada = 0; passada < PASSADAS_SUAVIZA; passada++) { // padrão MAP: a malha inteira, N vezes
        for (int p = 0; p < pontos; p++) { // padrão MAP: metade dele, metade da média dos vizinhos
            int de = g->inicio[p], ate = g->inicio[p + 1];
            float *saida = &aux[p * OSSOS_TOTAL];
            for (int b = 0; b < OSSOS_TOTAL; b++) // padrão MAP: zera a soma
                saida[b] = 0.0f;
            for (int k = de; k < ate; k++)            // padrão FOLD: soma os vizinhos...
                for (int b = 0; b < OSSOS_TOTAL; b++) // ...osso a osso
                    saida[b] += w[g->viz[k] * OSSOS_TOTAL + b];
            for (int b = 0; b < OSSOS_TOTAL; b++) // padrão MAP: metade dele, metade da média
                saida[b] = ate > de ? 0.5f * w[p * OSSOS_TOTAL + b] + 0.5f * saida[b] / (float)(ate - de)
                                    : w[p * OSSOS_TOTAL + b];
            mascara(saida, dono[p]);
        }
        memcpy(w, aux, (size_t)pontos * OSSOS_TOTAL * sizeof(float));
    }
}

// ================================================================== 4. os 4 mais fortes
static void quatro_maiores(const Vector3 v, const float *w, float *ids, float *pesos) {
    float q[OSSOS_TOTAL], soma = 0.0f, mandibula = esqueleto_peso_mandibula(v);
    for (int b = 0; b < OSSOS_TOTAL; b++) // padrão MAP: o queixo passa da cabeça para a mandíbula
        q[b] = w[b] * (1.0f - mandibula);
    q[OSSO_MANDIBULA] += mandibula;
    for (int k = 0; k < INFLUENCIAS; k++) { // padrão FOLD: a maior que sobrou, quatro vezes
        int maior = 0;
        for (int b = 1; b < OSSOS_TOTAL; b++) // padrão FOLD: o maior peso
            if (q[b] > q[maior]) maior = b;
        ids[k] = (float)maior;
        pesos[k] = q[maior];
        soma += q[maior];
        q[maior] = -1.0f;
    }
    for (int k = 0; k < INFLUENCIAS; k++) // padrão MAP: os quatro somam 1
        pesos[k] = soma > 0.0f ? pesos[k] / soma : (k == 0 ? 1.0f : 0.0f);
}

// ================================================================== cálculo e envio
static void libera(void *a, void *b, void *c, void *d, void *e, Grafo *g) {
    free(a), free(b), free(c), free(d), free(e), free(g->inicio), free(g->viz);
}

// Preenche ids/pesos (4 por vértice). false = faltou memória.
static bool calcula_pesos(const Mesh *m, float *ids, float *pesos) {
    const Vector3 *v = (const Vector3 *)m->vertices;
    int n = m->vertexCount, *id = malloc((size_t)n * sizeof(int)), *rep = malloc((size_t)n * sizeof(int));
    Grafo g = {0};
    int pontos = (id && rep) ? solda(v, n, id, rep) : -1;
    float *w = pontos > 0 ? malloc((size_t)pontos * OSSOS_TOTAL * sizeof(float)) : NULL;
    float *aux = pontos > 0 ? malloc((size_t)pontos * OSSOS_TOTAL * sizeof(float)) : NULL;
    int *dono = pontos > 0 ? malloc((size_t)pontos * sizeof(int)) : NULL;
    if (w == NULL || aux == NULL || dono == NULL || !monta_grafo(m, id, pontos, &g)) {
        libera(id, rep, w, aux, dono, &g);
        return false;
    }
    for (int p = 0; p < pontos; p++) // padrão MAP: pesos iniciais de cada ponto da pele
        dono[p] = pesos_do_ponto(v[rep[p]], &w[p * OSSOS_TOTAL]);
    suaviza(&g, pontos, dono, w, aux);
    for (int i = 0; i < n; i++) // padrão MAP: cada vértice herda os pesos do seu ponto
        quatro_maiores(v[i], &w[id[i] * OSSOS_TOTAL], &ids[i * INFLUENCIAS], &pesos[i * INFLUENCIAS]);
    TraceLog(LOG_INFO, "PRECURSOR: rig com %d ossos em %d vértices (%d pontos da pele)", OSSOS_TOTAL, n,
             pontos);
    libera(id, rep, w, aux, dono, &g);
    return true;
}

// ================================================================== 5. as emendas que não existem
static int profundidade(int b) {
    int d = 0;
    while (esqueleto_osso(b)->pai >= 0)
        b = esqueleto_osso(b)->pai, d++;
    return d;
}

// Quantos ossos separam a de b na hierarquia (0 = o mesmo; 2 = irmãos ou avô).
static int distancia_na_arvore(int a, int b) {
    int da = profundidade(a), db = profundidade(b), n = 0;
    for (; da > db; da--, n++) // sobe o mais fundo até a mesma altura...
        a = esqueleto_osso(a)->pai;
    for (; db > da; db--, n++)
        b = esqueleto_osso(b)->pai;
    for (; a != b; n += 2) // ...e sobem juntos até o ancestral comum
        a = esqueleto_osso(a)->pai, b = esqueleto_osso(b)->pai;
    return n;
}

// Um triângulo cujos cantos são de ossos a mais de 3 passos um do outro (a mão e a canela) só existe porque
// as duas partes se encostam no modelo: vira um triângulo vazio (os três índices iguais). Devolve quantos.
static int corta_emendas(Mesh *m, const float *ids) {
    if (m->indices == NULL) return 0;
    int cortados = 0;
    for (int t = 0; t < m->triangleCount; t++) { // padrão MAP: confere cada triângulo
        unsigned short *c = &m->indices[3 * t];
        int a = (int)ids[c[0] * INFLUENCIAS], b = (int)ids[c[1] * INFLUENCIAS],
            d = (int)ids[c[2] * INFLUENCIAS];
        if (distancia_na_arvore(a, b) <= 3 && distancia_na_arvore(b, d) <= 3 &&
            distancia_na_arvore(a, d) <= 3)
            continue;
        c[1] = c[2] = c[0];
        cortados++;
    }
    return cortados;
}

// Os dois atributos entram no VAO do próprio modelo: o DrawMesh da raylib os liga junto com o resto.
static void envia(Recursos *rec, Mesh *m, int loc_ids, int loc_pesos, const float *ids, const float *pesos) {
    int tamanho = m->vertexCount * INFLUENCIAS * (int)sizeof(float);
    rlEnableVertexArray(m->vaoId);
    rec->rig_vbo[0] = rlLoadVertexBuffer(ids, tamanho, false);
    rlSetVertexAttribute((unsigned int)loc_ids, INFLUENCIAS, RL_FLOAT, false, 0, 0);
    rlEnableVertexAttribute((unsigned int)loc_ids);
    rec->rig_vbo[1] = rlLoadVertexBuffer(pesos, tamanho, false);
    rlSetVertexAttribute((unsigned int)loc_pesos, INFLUENCIAS, RL_FLOAT, false, 0, 0);
    rlEnableVertexAttribute((unsigned int)loc_pesos);
    int cortados = corta_emendas(m, ids); // com o VAO dele ligado: os índices continuam dele
    if (cortados > 0)
        rlUpdateVertexBufferElements(m->vboId[RL_DEFAULT_SHADER_ATTRIB_LOCATION_INDICES], m->indices,
                                     m->triangleCount * 3 * (int)sizeof(unsigned short), 0);
    rlDisableVertexArray();
    TraceLog(LOG_INFO, "PRECURSOR: %d triângulos de emenda (partes encostadas) removidos", cortados);
}

static bool acha_uniformes(Recursos *rec) {
    char nome[16];
    for (int i = 0; i < OSSOS_TOTAL; i++) { // padrão MAP: ossos[0] .. ossos[21]
        snprintf(nome, sizeof(nome), "ossos[%d]", i);
        rec->loc_luz_ossos[i] = GetShaderLocation(rec->luz, nome);
    }
    rec->loc_luz_usa_ossos = GetShaderLocation(rec->luz, "usaOssos");
    return rec->loc_luz_usa_ossos >= 0 && rec->loc_luz_ossos[0] >= 0;
}

void rig_carrega(Recursos *rec) {
    rec->rig_ok = false;
    rec->rig_escala = CRIATURA_ALTURA_M / ALTURA_NOMINAL;
    rec->rig_base_y = BASE_NOMINAL;
    if (!rec->tem_modelo) return;
    BoundingBox caixa = GetModelBoundingBox(rec->modelo_precursor);
    rec->rig_escala = CRIATURA_ALTURA_M / fmaxf(0.01f, caixa.max.y - caixa.min.y);
    rec->rig_base_y = caixa.min.y;
    Mesh *m = &rec->modelo_precursor.meshes[0];
    int loc_ids = rec->luz_ok ? rlGetLocationAttrib(rec->luz.id, "ossoIds") : -1;
    int loc_pesos = rec->luz_ok ? rlGetLocationAttrib(rec->luz.id, "ossoPesos") : -1;
    if (loc_ids < 0 || loc_pesos < 0 || !acha_uniformes(rec) || m->vaoId == 0 || m->vertices == NULL ||
        rec->modelo_precursor.meshCount != 1) {
        TraceLog(LOG_WARNING,
                 "PRECURSOR: sem rig (shader ou malha sem suporte); o modelo fica na pose de repouso");
        return;
    }
    double inicio = GetTime();
    float *ids = malloc((size_t)m->vertexCount * INFLUENCIAS * sizeof(float));
    float *pesos = malloc((size_t)m->vertexCount * INFLUENCIAS * sizeof(float));
    if (ids != NULL && pesos != NULL && calcula_pesos(m, ids, pesos)) {
        envia(rec, m, loc_ids, loc_pesos, ids, pesos);
        rec->rig_ok = true;
        TraceLog(LOG_INFO, "PRECURSOR: pesos do rig calculados em %.0f ms", (GetTime() - inicio) * 1000.0);
    } else {
        TraceLog(LOG_WARNING, "PRECURSOR: sem memória para o rig; o modelo fica na pose de repouso");
    }
    free(ids);
    free(pesos);
}

void rig_descarrega(Recursos *rec) {
    if (!rec->rig_ok) return;
    rlUnloadVertexBuffer(rec->rig_vbo[0]);
    rlUnloadVertexBuffer(rec->rig_vbo[1]);
    rec->rig_ok = false;
}

// ================================================================== por quadro
static void usa_ossos(const Recursos *r, float sim) {
    rlDrawRenderBatchActive(); // o lote pendente foi montado sem os ossos
    rlEnableShader(r->luz.id);
    rlSetUniform(r->loc_luz_usa_ossos, &sim, RL_SHADER_UNIFORM_FLOAT, 1);
}

void rig_liga(const Estado *e) {
    const Recursos *r = &e->rec;
    if (!r->rig_ok) return;
    usa_ossos(r, 1.0f);
    for (int i = 0; i < OSSOS_TOTAL; i++) // padrão MAP: uma matriz por osso (pose deste quadro)
        rlSetUniformMatrix(r->loc_luz_ossos[i], e->cri.anim.ossos[i]);
}

void rig_desliga(const Estado *e) {
    if (e->rec.rig_ok) usa_ossos(&e->rec, 0.0f);
}

// ================================================================== desenho sem modelo
Vector3 rig_ponto_no_corpo(const Estado *e, int osso, Vector3 repouso) {
    Vector3 p = esqueleto_ponto(&e->cri.anim.global, osso, repouso);
    float s = e->rec.rig_escala;
    return (Vector3){p.x * s, (p.y - e->rec.rig_base_y) * s, p.z * s};
}

// Humanoide magro de ~2,6 m: um cilindro por osso, da articulação até a ponta, e a cabeça.
void rig_desenha_procedural(const Estado *e, Color cor) {
    for (int i = 0; i < OSSOS_TOTAL; i++) { // padrão MAP: um cilindro por osso
        const OssoRepouso *o = esqueleto_osso(i);
        if (i == OSSO_MANDIBULA || i == OSSO_CABECA) continue;
        float r = fmaxf(0.012f, o->raio * 0.7f) * e->rec.rig_escala;
        DrawCylinderEx(rig_ponto_no_corpo(e, i, o->cabeca), rig_ponto_no_corpo(e, i, o->cauda), r, r * 0.8f,
                       6, cor);
    }
    const OssoRepouso *c = esqueleto_osso(OSSO_CABECA);
    Vector3 meio = rig_ponto_no_corpo(e, OSSO_CABECA, Vector3Lerp(c->cabeca, c->cauda, 0.5f));
    DrawSphereEx(meio, 0.11f * e->rec.rig_escala, 8, 10, cor);
    const OssoRepouso *m = esqueleto_osso(OSSO_MANDIBULA);
    DrawCylinderEx(rig_ponto_no_corpo(e, OSSO_MANDIBULA, m->cabeca),
                   rig_ponto_no_corpo(e, OSSO_MANDIBULA, m->cauda), 0.05f * e->rec.rig_escala,
                   0.03f * e->rec.rig_escala, 6, cor);
}
