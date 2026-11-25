// clique.c — as áreas clicáveis do quadro (registradas ao desenhar) e as perguntas do atualiza sobre elas.
// Vetor de capacidade fixa e contador, zerado a cada desenho: nada de malloc por quadro.
#include "clique.h"

#define MAX_AREAS 64

typedef struct {
    GrupoClique grupo;
    int indice;
    Rectangle area;
} AreaClique;

static AreaClique areas[MAX_AREAS];
static int n_areas = 0;

void clique_limpa(void) {
    n_areas = 0;
}

void clique_area(GrupoClique grupo, int indice, Rectangle area) {
    if (n_areas >= MAX_AREAS) return; // não acontece nas telas de hoje; se acontecer, fica sem clique
    areas[n_areas++] = (AreaClique){grupo, indice, area};
}

int clique_sob_ponteiro(const Entrada *in, GrupoClique grupo) {
    if (!in->ponteiro_livre) return -1;
    // padrão FOLD: a última área registrada vence (é a que foi desenhada por cima)
    int achou = -1;
    for (int i = 0; i < n_areas; i++)
        if (areas[i].grupo == grupo && CheckCollisionPointRec(in->ponteiro, areas[i].area))
            achou = areas[i].indice;
    return achou;
}

bool clique_sobre_algo(const Entrada *in) {
    if (!in->ponteiro_livre) return false;
    for (int i = 0; i < n_areas; i++) // padrão FOLD: alguma área sob o ponteiro?
        if (CheckCollisionPointRec(in->ponteiro, areas[i].area)) return true;
    return false;
}

bool clique_escolhe(const Entrada *in, GrupoClique grupo, int *cursor) {
    int sob = clique_sob_ponteiro(in, grupo);
    if (sob < 0) return false;
    if (in->ponteiro_moveu || in->clique) *cursor = sob;
    return in->clique;
}

void clique_aperta_teclas(Entrada *in) {
    if (!in->clique) return;
    switch ((TeclaClique)clique_sob_ponteiro(in, CLIQUE_TECLA)) {
    case TECLA_ESC:
        in->voltar = true;
        break;
    case TECLA_ENTER:
        in->confirmar = true;
        break;
    case TECLA_H:
        in->ajuda = true;
        break;
    case TECLA_1:
        in->tecla1 = true;
        break;
    case TECLA_2:
        in->tecla2 = true;
        break;
    case TECLA_3:
        in->tecla3 = true;
        break;
    case TECLA_E:
        in->interagir = true;
        break;
    case TECLA_F:
        in->lanterna = true;
        break;
    case TECLA_ESPACO:
        in->respirar = true;
        break;
    case TECLA_B:
        in->comandos = true;
        break;
    default:
        return; // fora da barra: quem trata é a tela
    }
    in->clique = false; // o clique já virou uma tecla: a tela embaixo da barra não o recebe de novo
}
