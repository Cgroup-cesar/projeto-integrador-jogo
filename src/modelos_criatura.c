// modelos_criatura.c — os modelos do Precursor que o jogo sabe carregar, cada um com o seu esqueleto e as
// medidas tiradas do arquivo (unidades do .glb: y de −1 a 1, rosto para +z, +x é a esquerda DELE).
// Escolha com --criatura <nome> (esqueleto_usa); o padrão é o primeiro. Puro: só dados.
#include "esqueleto.h"

const ModeloCriatura MODELOS_CRIATURA[CRIATURAS_TOTAL] = {
    // O modelo de sempre. Em repouso ele já está curvado: quadril para trás, joelhos dobrados ~45°, braços
    // pendendo à frente. É um pouco assimétrico (o braço direito pende mais à frente): a tabela segue o
    // modelo como ele é. Os dedos esquerdos estão fundidos à canela (rig.c corta essa emenda).
    [CRIATURA_PRECURSOR] =
        {
            .nome = "precursor",
            .arquivo = "modelos/precursor.glb",
            .ossos =
                {
                    [OSSO_QUADRIL] = {"quadril", -1, {-0.03f, 0.08f, -0.27f}, {-0.03f, 0.24f, -0.27f}, 0.17f},
                    [OSSO_LOMBAR] =
                        {"lombar", OSSO_QUADRIL, {-0.03f, 0.24f, -0.26f}, {-0.03f, 0.42f, -0.20f}, 0.15f},
                    [OSSO_PEITO] =
                        {"peito", OSSO_LOMBAR, {-0.03f, 0.42f, -0.20f}, {-0.03f, 0.64f, -0.09f}, 0.15f},
                    [OSSO_PESCOCO] =
                        {"pescoco", OSSO_PEITO, {-0.03f, 0.64f, -0.08f}, {-0.02f, 0.77f, 0.00f}, 0.06f},
                    [OSSO_CABECA] =
                        {"cabeca", OSSO_PESCOCO, {-0.02f, 0.77f, 0.00f}, {-0.02f, 0.95f, 0.08f}, 0.11f},
                    [OSSO_MANDIBULA] =
                        {"mandibula", OSSO_CABECA, {-0.015f, 0.80f, 0.03f}, {-0.015f, 0.66f, 0.12f}, 0.0f},
                    [OSSO_OMBRO_E] =
                        {"ombro_e", OSSO_PEITO, {-0.01f, 0.60f, -0.08f}, {0.17f, 0.60f, -0.06f}, 0.07f},
                    [OSSO_BRACO_E] =
                        {"braco_e", OSSO_OMBRO_E, {0.18f, 0.60f, -0.06f}, {0.33f, 0.18f, -0.01f}, 0.045f},
                    [OSSO_ANTEBRACO_E] =
                        {"antebraco_e", OSSO_BRACO_E, {0.33f, 0.18f, -0.01f}, {0.33f, -0.14f, 0.18f}, 0.04f},
                    [OSSO_MAO_E] =
                        {"mao_e", OSSO_ANTEBRACO_E, {0.33f, -0.14f, 0.18f}, {0.33f, -0.30f, 0.26f}, 0.04f},
                    [OSSO_DEDOS_E] =
                        {"dedos_e", OSSO_MAO_E, {0.33f, -0.30f, 0.26f}, {0.335f, -0.64f, 0.30f}, 0.02f},
                    [OSSO_OMBRO_D] =
                        {"ombro_d", OSSO_PEITO, {-0.05f, 0.60f, -0.08f}, {-0.21f, 0.60f, -0.03f}, 0.07f},
                    [OSSO_BRACO_D] =
                        {"braco_d", OSSO_OMBRO_D, {-0.22f, 0.60f, -0.02f}, {-0.31f, 0.22f, 0.06f}, 0.045f},
                    [OSSO_ANTEBRACO_D] =
                        {"antebraco_d", OSSO_BRACO_D, {-0.31f, 0.22f, 0.06f}, {-0.32f, -0.09f, 0.26f}, 0.04f},
                    [OSSO_MAO_D] =
                        {"mao_d", OSSO_ANTEBRACO_D, {-0.32f, -0.09f, 0.26f}, {-0.34f, -0.20f, 0.35f}, 0.04f},
                    [OSSO_DEDOS_D] =
                        {"dedos_d", OSSO_MAO_D, {-0.34f, -0.20f, 0.35f}, {-0.38f, -0.40f, 0.44f}, 0.02f},
                    [OSSO_COXA_E] =
                        {"coxa_e", OSSO_QUADRIL, {0.11f, 0.08f, -0.26f}, {0.29f, -0.33f, 0.07f}, 0.07f},
                    [OSSO_CANELA_E] =
                        {"canela_e", OSSO_COXA_E, {0.29f, -0.33f, 0.07f}, {0.28f, -0.88f, 0.02f}, 0.045f},
                    [OSSO_PE_E] =
                        {"pe_e", OSSO_CANELA_E, {0.28f, -0.88f, 0.02f}, {0.29f, -0.99f, 0.29f}, 0.04f},
                    [OSSO_COXA_D] =
                        {"coxa_d", OSSO_QUADRIL, {-0.16f, 0.08f, -0.26f}, {-0.36f, -0.33f, 0.05f}, 0.07f},
                    [OSSO_CANELA_D] =
                        {"canela_d", OSSO_COXA_D, {-0.36f, -0.33f, 0.05f}, {-0.35f, -0.88f, 0.02f}, 0.045f},
                    [OSSO_PE_D] =
                        {"pe_d", OSSO_CANELA_D, {-0.35f, -0.88f, 0.02f}, {-0.30f, -0.99f, 0.30f}, 0.04f},
                },
            .altura_nominal = 1.993f, // o arquivo vai de y = −0,997 a 0,996
            .base_nominal = -0.997f,
            // o queixo: abaixo da boca e à frente do pescoço; a dobradiça (a cabeça da mandíbula), perto das
            // orelhas
            .queixo = {0.60f, 0.765f, 0.06f, -0.015f, 0.12f},
            .olhos = {{-0.0495f, 0.8485f, 0.2f}, {0.0275f, 0.8485f, 0.2f}},
            .rosto = {0.48f, 0.90f, 0.72f}, // o rosto do susto, em fração da caixa do modelo
            .peito_m = 1.5f,
            .cela_avanco_m = 0.0f,
            .corta_emendas = true,
            .postura = {.passo = 1.0f}, // o arquivo já é curvado: a animação só soma a postura de sempre
        },
    // O alien do papelão do estande, gerado por IA (Hunyuan3D 2.1, sobre a arte conceitual do Meshy feita a
    // partir do desenho): de pé, reto, simétrico e de braços abaixados, sem olhos (rosto liso com a fenda),
    // sem textura. A tabela foi medida por fatias horizontais (o mesmo método dá ~2 cm de erro na tabela do
    // precursor) e simetrizada; o joelho vai 3 cm à frente da linha quadril-tornozelo para o IK saber para
    // onde dobrar. A curvatura ameaçadora sai da postura (a animação soma), não do arquivo.
    [CRIATURA_PAPELAO] =
        {
            .nome = "papelao",
            .arquivo = "modelos/precursor_papelao.glb",
            .ossos =
                {
                    [OSSO_QUADRIL] =
                        {"quadril", -1, {-0.005f, 0.08f, -0.065f}, {-0.005f, 0.24f, -0.085f}, 0.09f},
                    [OSSO_LOMBAR] =
                        {"lombar", OSSO_QUADRIL, {-0.005f, 0.24f, -0.085f}, {-0.005f, 0.41f, -0.08f}, 0.095f},
                    [OSSO_PEITO] =
                        {"peito", OSSO_LOMBAR, {-0.005f, 0.41f, -0.08f}, {-0.005f, 0.625f, -0.105f}, 0.115f},
                    [OSSO_PESCOCO] = {"pescoco",
                                      OSSO_PEITO,
                                      {-0.005f, 0.625f, -0.105f},
                                      {-0.005f, 0.735f, -0.09f},
                                      0.085f},
                    [OSSO_CABECA] = {"cabeca",
                                     OSSO_PESCOCO,
                                     {-0.005f, 0.735f, -0.09f},
                                     {-0.005f, 0.945f, -0.075f},
                                     0.085f},
                    [OSSO_MANDIBULA] =
                        {"mandibula", OSSO_CABECA, {-0.005f, 0.80f, -0.07f}, {-0.005f, 0.73f, 0.025f}, 0.0f},
                    [OSSO_OMBRO_E] =
                        {"ombro_e", OSSO_PEITO, {0.015f, 0.58f, -0.10f}, {0.15f, 0.58f, -0.105f}, 0.09f},
                    [OSSO_BRACO_E] =
                        {"braco_e", OSSO_OMBRO_E, {0.15f, 0.58f, -0.105f}, {0.215f, 0.265f, -0.10f}, 0.06f},
                    [OSSO_ANTEBRACO_E] = {"antebraco_e",
                                          OSSO_BRACO_E,
                                          {0.215f, 0.265f, -0.10f},
                                          {0.255f, 0.00f, -0.04f},
                                          0.045f},
                    [OSSO_MAO_E] =
                        {"mao_e", OSSO_ANTEBRACO_E, {0.255f, 0.00f, -0.04f}, {0.24f, -0.145f, -0.03f}, 0.07f},
                    [OSSO_DEDOS_E] =
                        {"dedos_e", OSSO_MAO_E, {0.24f, -0.145f, -0.03f}, {0.195f, -0.33f, -0.08f}, 0.045f},
                    [OSSO_OMBRO_D] =
                        {"ombro_d", OSSO_PEITO, {-0.025f, 0.58f, -0.10f}, {-0.16f, 0.58f, -0.105f}, 0.09f},
                    [OSSO_BRACO_D] =
                        {"braco_d", OSSO_OMBRO_D, {-0.16f, 0.58f, -0.105f}, {-0.225f, 0.265f, -0.10f}, 0.06f},
                    [OSSO_ANTEBRACO_D] = {"antebraco_d",
                                          OSSO_BRACO_D,
                                          {-0.225f, 0.265f, -0.10f},
                                          {-0.265f, 0.00f, -0.04f},
                                          0.045f},
                    [OSSO_MAO_D] = {"mao_d",
                                    OSSO_ANTEBRACO_D,
                                    {-0.265f, 0.00f, -0.04f},
                                    {-0.25f, -0.145f, -0.03f},
                                    0.07f},
                    [OSSO_DEDOS_D] =
                        {"dedos_d", OSSO_MAO_D, {-0.25f, -0.145f, -0.03f}, {-0.205f, -0.33f, -0.08f}, 0.045f},
                    [OSSO_COXA_E] =
                        {"coxa_e", OSSO_QUADRIL, {0.06f, 0.08f, -0.06f}, {0.095f, -0.32f, -0.05f}, 0.055f},
                    [OSSO_CANELA_E] =
                        {"canela_e", OSSO_COXA_E, {0.095f, -0.32f, -0.05f}, {0.10f, -0.82f, -0.105f}, 0.05f},
                    [OSSO_PE_E] =
                        {"pe_e", OSSO_CANELA_E, {0.10f, -0.82f, -0.105f}, {0.155f, -0.99f, 0.185f}, 0.065f},
                    [OSSO_COXA_D] =
                        {"coxa_d", OSSO_QUADRIL, {-0.065f, 0.08f, -0.06f}, {-0.10f, -0.32f, -0.05f}, 0.055f},
                    [OSSO_CANELA_D] = {"canela_d",
                                       OSSO_COXA_D,
                                       {-0.10f, -0.32f, -0.05f},
                                       {-0.105f, -0.82f, -0.105f},
                                       0.05f},
                    [OSSO_PE_D] =
                        {"pe_d", OSSO_CANELA_D, {-0.105f, -0.82f, -0.105f}, {-0.16f, -0.99f, 0.185f}, 0.065f},
                },
            .altura_nominal = 1.987f, // o arquivo vai de y = −0,998 a 0,989
            .base_nominal = -0.998f,
            .queixo = {0}, // sem boca: a mandíbula não leva nenhum vértice
            // sem olhos: no menu, dois pontos de luz na fenda, um sobre o outro
            .olhos = {{-0.003f, 0.856f, 0.034f}, {-0.003f, 0.826f, 0.037f}},
            .rosto = {0.50f, 0.925f, 0.60f}, // o centro da fenda (o z da caixa vem dos pés)
            // na malha, a fenda é um sulco de 2 mm (some na tela): um traço escuro, metade enterrado na cara
            .fenda = {{-0.003f, 0.870f, 0.030f}, {-0.003f, 0.802f, 0.0385f}},
            .fenda_raio = 0.0045f,
            .peito_m = 1.9f,        // reto, o peito fica mais alto
            .cela_avanco_m = 0.18f, // reto, ele ficaria longe do vidro: chega um pouco mais perto
            .corta_emendas = false, // nada fundido: mãos e dedos soltos do corpo
            // abaixo de y 0,40 a malha é três peças (o corpo com as pernas e os dois braços): a ponta da mão
            // fica a 3 cm da coxa e, sem separar, iria para o osso da perna (esticando a pele)
            .bracos_soltos_y = 0.40f,
            .postura =
                {
                    .lombar = 14.0f,
                    .peito = 12.0f,
                    .pescoco = 18.0f,
                    .cabeca = -14.0f,
                    .braco = -10.0f,
                    .braco_abre = 6.0f,
                    .antebraco = -22.0f,
                    .mao = -12.0f,
                    .dedos = 10.0f,
                    .desce_m = 0.10f, // o quadril desce: o IK dobra os joelhos
                    .passo = 0.8f,    // a perna é ~20% mais curta que a do precursor
                },
        },
};
