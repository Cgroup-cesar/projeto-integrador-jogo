// qualidade.c — os presets gráficos e o liga/desliga de cada efeito (ver qualidade.h).
#include "qualidade.h"
#include <string.h>

// Os números de cada preset: o Alto é a meta (60 FPS em 1920 × 1080 numa GPU integrada recente; ver
// docs/graficos.md), o Ultra pede mais da GPU, o Médio e o Baixo cortam por custo.
static const Qualidade PRESETS[QUALIDADE_TOTAL] = {
    {QUALIDADE_BAIXA, 0.6f, 0.0f, 0, false, 0, 0.5f, false, false, true},
    {QUALIDADE_MEDIA, 0.75f, 0.35f, 512, true, 10, 0.35f, true, true, true},
    {QUALIDADE_ALTA, 1.0f, 0.5f, 1024, true, 16, 0.5f, true, true, true},
    {QUALIDADE_ULTRA, 1.0f, 0.75f, 2048, true, 32, 0.5f, true, true, true},
};

static const char *const NOMES[QUALIDADE_TOTAL] = {"baixa", "media", "alta", "ultra"};

static Qualidade atual = {QUALIDADE_ALTA, 1.0f, 0.5f, 1024, true, 16, 0.5f, true, true, true};

// O que --sem desligou continua desligado em qualquer preset.
static struct {
    bool ssao, volume, bloom, sombra, reflexo, mapas, pos;
} sem;

const Qualidade *qualidade(void) {
    return &atual;
}

void qualidade_escolhe(NivelQualidade nivel) {
    if (nivel < 0 || nivel >= QUALIDADE_TOTAL) return;
    atual = PRESETS[nivel];
    if (sem.ssao) atual.ssao = false;
    if (sem.volume) atual.volume_passos = 0;
    if (sem.bloom) atual.bloom = false;
    if (sem.sombra) atual.sombra_px = 0;
    if (sem.reflexo) atual.reflexo = 0.0f;
    if (sem.mapas) atual.mapas = false;
    if (sem.pos) atual.pos = false;
}

const char *qualidade_nome(NivelQualidade nivel) {
    return nivel >= 0 && nivel < QUALIDADE_TOTAL ? NOMES[nivel] : "?";
}

// "ssao,volume" → liga cada flag de `sem` que aparecer na lista.
static void le_lista_sem(const char *lista) {
    sem.ssao = sem.ssao || strstr(lista, "ssao") != NULL;
    sem.volume = sem.volume || strstr(lista, "volume") != NULL;
    sem.bloom = sem.bloom || strstr(lista, "bloom") != NULL;
    sem.sombra = sem.sombra || strstr(lista, "sombra") != NULL;
    sem.reflexo = sem.reflexo || strstr(lista, "reflexo") != NULL;
    sem.mapas = sem.mapas || strstr(lista, "mapas") != NULL;
    sem.pos = sem.pos || strstr(lista, "pos") != NULL;
}

bool qualidade_le_argumento(int argc, char **argv, int *i) {
    if (strcmp(argv[*i], "--qualidade") == 0 && *i + 1 < argc) {
        const char *nome = argv[++*i];
        for (int q = 0; q < QUALIDADE_TOTAL; q++) // padrão FILTER: o preset com esse nome
            if (strcmp(nome, NOMES[q]) == 0) qualidade_escolhe((NivelQualidade)q);
        return true;
    }
    if (strcmp(argv[*i], "--sem") == 0 && *i + 1 < argc) {
        le_lista_sem(argv[++*i]);
        qualidade_escolhe(atual.nivel);
        return true;
    }
    return false;
}
