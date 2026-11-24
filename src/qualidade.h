// qualidade.h — os presets gráficos (Baixo, Médio, Alto, Ultra) e o liga/desliga de cada efeito. Lidos uma
// vez na linha de comando (--qualidade, --sem); F7 troca o preset durante o jogo (com TECLAS_DEBUG).
// Detalhes e custos medidos: docs/graficos.md.
#ifndef QUALIDADE_H
#define QUALIDADE_H

#include <stdbool.h>

typedef enum {
    QUALIDADE_BAIXA,
    QUALIDADE_MEDIA,
    QUALIDADE_ALTA,
    QUALIDADE_ULTRA,
    QUALIDADE_TOTAL
} NivelQualidade;

typedef struct {
    NivelQualidade nivel;
    float escala;        // resolução da cena 3D (fração da janela)
    float reflexo;       // resolução do reflexo planar do chão (fração da cena; 0 = sem reflexo)
    int sombra_px;       // lado do mapa de sombra da lanterna (0 = sem sombra)
    bool ssao;           // oclusão ambiente em espaço de tela
    int volume_passos;   // passos da névoa volumétrica (0 = névoa simples)
    float volume_escala; // resolução da névoa volumétrica (fração da cena)
    bool bloom;          // brilho das luzes espalhado
    bool mapas;          // mapas de normal e ORM dos materiais (sem eles: relevo chapado)
    bool pos;            // gradação de cor, vinheta, grão e aberração
} Qualidade;

const Qualidade *qualidade(void);
void qualidade_escolhe(NivelQualidade nivel); // aplica o preset (mantém o que --sem desligou)
const char *qualidade_nome(NivelQualidade nivel);

// --qualidade baixa|media|alta|ultra e --sem ssao,volume,bloom,sombra,reflexo,mapas,pos. Devolve true se
// reconheceu argv[*i] (e avança *i).
bool qualidade_le_argumento(int argc, char **argv, int *i);

#endif
