// modelos_criatura.c — o modelo do Precursor (o alien do papelão), com o esqueleto e as medidas tiradas do
// arquivo (unidades do .glb: y de −1 a 1, rosto para +z, +x é a esquerda DELE). Puro: só dados.
#include "esqueleto.h"

// O alien do papelão do estande, gerado por IA (Hunyuan3D 2.1, sobre a arte conceitual do Meshy feita a
// partir do desenho): de pé, reto, simétrico e de braços abaixados, de rosto liso (só as marcas dos olhos),
// sem textura. A tabela foi medida por fatias horizontais e simetrizada; o joelho vai 3 cm à frente da linha
// quadril-tornozelo para o IK saber para onde dobrar. A curvatura ameaçadora sai da postura (a animação
// soma), não do arquivo.
const ModeloCriatura MODELO_CRIATURA =
    {
        .arquivo = "modelos/precursor_papelao.glb",
        .ossos =
            {
                [OSSO_QUADRIL] = {"quadril", -1, {-0.005f, 0.08f, -0.065f}, {-0.005f, 0.24f, -0.085f}, 0.09f},
                [OSSO_LOMBAR] =
                    {"lombar", OSSO_QUADRIL, {-0.005f, 0.24f, -0.085f}, {-0.005f, 0.41f, -0.08f}, 0.095f},
                [OSSO_PEITO] =
                    {"peito", OSSO_LOMBAR, {-0.005f, 0.41f, -0.08f}, {-0.005f, 0.625f, -0.105f}, 0.115f},
                [OSSO_PESCOCO] =
                    {"pescoco", OSSO_PEITO, {-0.005f, 0.625f, -0.105f}, {-0.005f, 0.735f, -0.09f}, 0.085f},
                [OSSO_CABECA] =
                    {"cabeca", OSSO_PESCOCO, {-0.005f, 0.735f, -0.09f}, {-0.005f, 0.945f, -0.075f}, 0.085f},
                [OSSO_MANDIBULA] =
                    {"mandibula", OSSO_CABECA, {-0.005f, 0.80f, -0.07f}, {-0.005f, 0.73f, 0.025f}, 0.0f},
                [OSSO_OMBRO_E] =
                    {"ombro_e", OSSO_PEITO, {0.015f, 0.58f, -0.10f}, {0.15f, 0.58f, -0.105f}, 0.09f},
                [OSSO_BRACO_E] =
                    {"braco_e", OSSO_OMBRO_E, {0.15f, 0.58f, -0.105f}, {0.215f, 0.265f, -0.10f}, 0.06f},
                [OSSO_ANTEBRACO_E] =
                    {"antebraco_e", OSSO_BRACO_E, {0.215f, 0.265f, -0.10f}, {0.255f, 0.00f, -0.04f}, 0.045f},
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
                [OSSO_MAO_D] =
                    {"mao_d", OSSO_ANTEBRACO_D, {-0.265f, 0.00f, -0.04f}, {-0.25f, -0.145f, -0.03f}, 0.07f},
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
                [OSSO_CANELA_D] =
                    {"canela_d", OSSO_COXA_D, {-0.10f, -0.32f, -0.05f}, {-0.105f, -0.82f, -0.105f}, 0.05f},
                [OSSO_PE_D] =
                    {"pe_d", OSSO_CANELA_D, {-0.105f, -0.82f, -0.105f}, {-0.16f, -0.99f, 0.185f}, 0.065f},
            },
        .altura_nominal = 1.987f, // o arquivo vai de y = −0,998 a 0,989
        .base_nominal = -0.998f,
        // rosto liso (a frente da cara foi alisada no arquivo), só com os dois olhos: meios-discos
        // escuros,
        // a reta em cima e a curva embaixo, rentes à pele (a cara está em z ≈ 0,027 na altura dos olhos);
        // no menu, os olhos acendem
        .olhos = {{0.028f, 0.856f, 0.029f}, {-0.036f, 0.856f, 0.029f}},
        .rosto = {0.50f, 0.925f, 0.60f}, // o centro do rosto, entre os olhos (o z da caixa vem dos pés)
        .marcas = {{{0.028f, 0.861f, 0.0283f}, {1.0f, -0.12f, -0.18f}, {0.18f, 0.27f, 1.0f}, 0.012f},
                   {{-0.036f, 0.861f, 0.0283f}, {-1.0f, -0.12f, -0.18f}, {-0.18f, 0.27f, 1.0f}, 0.012f}},
        .n_marcas = 2,
        // a frente da cara é mais saliente em y ≈ 0,80 (z 0,042), entre os olhos (0,856) e o queixo (0,74);
        // escancarada, a boca toma quase toda a parte de baixo do rosto
        .boca = {{-0.004f, 0.796f, 0.043f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0.035f, 0.038f},
        .peito_m = 1.9f,        // reto, o peito fica mais alto
        .cela_avanco_m = 0.18f, // reto, ele ficaria longe do vidro: chega um pouco mais perto
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
                .passo = 0.8f,    // a perna é curta: passada 20% menor que a padrão
            },
};
