// dialogo.c — todos os textos do jogo. Para mudar uma fala, mude só aqui.
// Duração das legendas em segundos (o jogador pode ler com calma; o interfone não tem pressa).
#include "dialogo.h"

// ================================================================== falas (legendas)
#define V FALANTE_VARELA
#define E FALANTE_ELIAS

static const Legenda INTRO_INTERFONE[] = {
    {"Boa noite, professor. Coronel Varela. Não encoste no vidro.", 4.0f, V},
    {"Nós o chamamos de Precursor. Está conosco desde 77. Não fala como nós. Não pensa como nós.", 5.5f, V},
    {"A Dra. Sampaio concluiu que a língua dele não tem palavras. Só estrutura. Verdadeiro. Falso.", 5.5f, V},
    {"Os interruptores à sua frente controlam as luzes da cela. Ele responde ao que vê. Descubra o que ele diz.", 6.0f, V},
};
static const Legenda APOS_R1[] = {
    {"'Nós não somos perigo.' ...Anotado.", 3.5f, V},
};
static const Legenda INICIO_S1[] = {
    {"Queda de energia. O gerador auxiliar fica no fim do corredor. Sem energia, a contenção...", 5.0f, V},
    {"Vá. Agora. E não corra se não precisar. Ele ouve.", 4.0f, V},
};
static const Legenda FIM_S1[] = {
    {"Energia restabelecida. O campo puxou ele de volta. Volte para a sala.", 4.5f, V},
};
static const Legenda APOS_R2[] = {
    {"'Você e nós falamos.' Ele está aprendendo rápido demais, professor.", 4.5f, V},
};
static const Legenda INICIO_S2[] = {
    {"De novo. Agora o painel do Arquivo. É a sala da Dra. Sampaio... não mexa nas coisas dela.", 5.5f, V},
};
static const Legenda FIM_S2[] = {
    {"Contenção estável. Por enquanto.", 3.5f, V},
};
static const Legenda APOS_R3[] = {
    {"Professor... desligue o transmissor. Isso é uma ordem!", 4.5f, V},
};
static const Legenda FIM_BOM_FALAS[] = {
    {"...Ele parou. Professor, o que o senhor disse a ele?", 4.0f, V},
    {"Nada que pudesse ser verdade.", 3.5f, E},
};
static const Legenda PORTA_TRANCADA[] = {
    {"A porta fica trancada enquanto o senhor trabalha, professor. Volte ao console.", 4.0f, V},
};
static const Legenda RODADA_RECOMECA[] = {
    {"Vidro substituído. Recomece a leitura, professor. Com mais cuidado.", 4.0f, V},
};
static const Legenda PEGO_FALAS[] = {
    {"...Professor? Professor! Responda! — Ele voltou ao começo do corredor. Tente de novo.", 4.5f, V},
};

#undef V
#undef E

typedef struct {
    const Legenda *falas;
    int n;
} Bloco;

#define BLOCO(vetor) {vetor, (int)(sizeof(vetor) / sizeof(vetor[0]))}

static const Bloco BLOCOS[FALAS_TOTAL] = {
    BLOCO(INTRO_INTERFONE), BLOCO(APOS_R1),        BLOCO(INICIO_S1),      BLOCO(FIM_S1),
    BLOCO(APOS_R2),         BLOCO(INICIO_S2),      BLOCO(FIM_S2),         BLOCO(APOS_R3),
    BLOCO(FIM_BOM_FALAS),   BLOCO(PORTA_TRANCADA), BLOCO(RODADA_RECOMECA), BLOCO(PEGO_FALAS),
};

const Legenda *dialogo_falas(BlocoFalas bloco, int *n) {
    if (bloco < 0 || bloco >= FALAS_TOTAL) {
        *n = 0;
        return NULL;
    }
    *n = BLOCOS[bloco].n;
    return BLOCOS[bloco].falas;
}

// ================================================================== intro datilografada
static const char *const INTRO[INTRO_LINHAS] = {
    "SERRA DO RONCADOR, MATO GROSSO — 14 DE MARÇO DE 1985.",
    "Amanhã o país terá seu primeiro presidente civil em 21 anos.",
    "Esta noite, o Serviço Nacional de Informações precisa de um tradutor.",
    "Prof. Elias Moura. Lógica Matemática, USP. Convocado às 03h12. Sem direito a recusa.",
};

const char *dialogo_intro(int linha) {
    return (linha >= 0 && linha < INTRO_LINHAS) ? INTRO[linha] : "";
}

// ================================================================== textos soltos
static const char *const TEXTOS[TXT_TOTAL] = {
    [TXT_MENU_INICIAR] = "INICIAR",
    [TXT_MENU_SAIR] = "SAIR",
    [TXT_MENU_AVISO_PROVISORIO] = "[NOME PROVISÓRIO — título final a definir]",
    [TXT_MENU_CONTROLES] = "WASD andar · mouse olhar · Shift correr · E interagir · Esc pausar",
    [TXT_MENU_DICA] = "Setas e ENTER para escolher",
    [TXT_PAUSA] = "PAUSA",
    [TXT_PAUSA_CONTINUAR] = "CONTINUAR",
    [TXT_PAUSA_MENU] = "VOLTAR AO MENU",
    [TXT_INTRO_PULAR] = "ENTER pula",
    [TXT_HUD_VIDRO] = "VIDRO",
    [TXT_HUD_PLACAR] = "PLACAR",
    [TXT_HUD_FOLEGO] = "FÔLEGO",
    [TXT_MIRA_CONSOLE] = "E — usar o console",
    [TXT_MIRA_PAINEL] = "E — abrir o painel de circuito",
    [TXT_MIRA_DIARIO] = "E — ler o diário",
    [TXT_MIRA_ARMARIO] = "E — esconder-se no armário",
    [TXT_MIRA_PORTA] = "E — porta da sala de observação",
    [TXT_ARMARIO_SAIR] = "E — sair do armário",
    [TXT_OBJETIVO_SALA] = "Vá ao console (E) e descubra o que ele diz.",
    [TXT_OBJETIVO_S1] = "Religue o gerador: painel 1, no fim do corredor (ao norte).",
    [TXT_OBJETIVO_S2] = "Religue a contenção: painel 2, no Arquivo (sudoeste).",
    [TXT_OBJETIVO_VOLTAR] = "Volte para a sala de observação.",
    [TXT_OBJETIVO_RESPOSTA] = "Responda pelo console.",
    [TXT_TERMINAL_TITULO] = "ANALISADOR SEMÂNTICO SNI-85",
    [TXT_TERMINAL_DICA_R1_1] = "Tabela-verdade: para cada combinação das luzes, ele diz VERDADEIRO ou FALSO.",
    [TXT_TERMINAL_DICA_R1_2] = "Teste todas as combinações; o padrão das respostas revela o conectivo.",
    [TXT_TERMINAL_LUZES] = "[1] LUZ A   [2] LUZ B",
    [TXT_TERMINAL_OBSERVE] = "Mude as luzes e ouça a resposta. A tabela se preenche sozinha.",
    [TXT_TERMINAL_ESCOLHA_CONECTIVO] = "O que significa o glifo âmbar?  (setas + ENTER)",
    [TXT_TERMINAL_TRADUZA] = "Ele está falando. Qual é a tradução?  (setas + ENTER)",
    [TXT_TERMINAL_ERRO] = "INCORRETO. O VIDRO CEDE.",
    [TXT_TERMINAL_CERTO] = "CORRETO.",
    [TXT_TERMINAL_RESPONDENDO] = "...ELE RESPONDE...",
    [TXT_TERMINAL_SAIR] = "Esc — sair do terminal",
    [TXT_TERMINAL_ANOTACAO] = "ANOTAÇÃO RECUPERADA: \"uma frase que nunca possa ser verdadeira\"",
    [TXT_TERMINAL_RESPONDA] = "RESPONDA.",
    [TXT_TERMINAL_SINTAXE_INVALIDA] = "SINTAXE INVÁLIDA",
    [TXT_TERMINAL_TRANSMITINDO] = "TRANSMITINDO...",
    [TXT_TERMINAL_FRASE] = "FRASE:",
    [TXT_TERMINAL_FRASE_OBSERVADA] = "FRASE OBSERVADA",
    [TXT_TERMINAL_LUZ_ACESA] = "ACESA",
    [TXT_TERMINAL_LUZ_APAGADA] = "APAGADA",
    [TXT_TERMINAL_TABELA] = "TABELA-VERDADE",
    [TXT_TERMINAL_QUAL_FRASE] = "Frase %d de %d",
    [TXT_VERDADEIRO] = "VERDADEIRO",
    [TXT_FALSO] = "FALSO",
    [TXT_PAINEL1_TITULO] = "PAINEL 1 — GERADOR AUXILIAR",
    [TXT_PAINEL2_TITULO] = "PAINEL 2 — CONTENÇÃO (ARQUIVO)",
    [TXT_PAINEL_INSTRUCOES1] = "[1] [2] [3] chaves A, B, C · Esc fecha",
    [TXT_PAINEL_INSTRUCOES2] = "Setas escolhem o encaixe e a porta · Esc fecha",
    [TXT_PAINEL_RESOLVIDO] = "SAÍDA ACESA — ENERGIA RESTABELECIDA",
    [TXT_PAINEL_ALVO] = "ALVO",
    [TXT_DIARIO] = "02/03/85 — Cada frase que traduzimos, ele repete melhor. Não é diálogo: é aula, e nós somos os "
                   "professores. Ele só consegue agir sobre aquilo que PODE ser verdade. Se um dia for preciso calá-lo, "
                   "dê a ele uma frase que nunca possa ser verdadeira. P E NÃO P. O Varela não pode saber disso. — I.S.",
    [TXT_DIARIO_FECHAR] = "E ou Esc — fechar",
    [TXT_FIM_RUIM_ACEITA] = "TRANSMISSÃO ACEITA.",
    [TXT_FIM_RUIM_OBRIGADO] = "OBRIGADO, PROFESSOR.",
    [TXT_FIM_RUIM_TELA] = "FIM — 1/2. A contenção do Posto 7 nunca foi restabelecida.",
    [TXT_FIM_BOM_ELIAS] = "Nada que pudesse ser verdade.",
    [TXT_FIM_BOM_TELA] = "FIM — 2/2. O Posto 7 foi lacrado em 15 de março de 1985. Oficialmente, ele nunca existiu.",
    [TXT_FIM_VITORIA] = "VITÓRIA",
    [TXT_FIM_DERROTA] = "DERROTA",
    [TXT_FIM_PLACAR] = "PLACAR TOTAL",
    [TXT_FIM_TEMPO] = "TEMPO",
    [TXT_FIM_ERROS] = "ERROS",
    [TXT_FIM_ENTER] = "Pressione ENTER",
    [TXT_FIM_RODAPE] = "Projeto Integrador PIF · nome provisório",
};

const char *dialogo_texto(TextoId id) {
    if (id < 0 || id >= TXT_TOTAL || TEXTOS[id] == NULL) return "";
    return TEXTOS[id];
}

const char *dialogo_nome_fase(int fase) {
    static const char *const nomes[FASE_TOTAL] = {
        "", "Intro", "Rodada 1", "Saída 1", "Rodada 2", "Saída 2", "Rodada 3", "Resposta final", "Final", "Final",
    };
    return (fase >= 0 && fase < FASE_TOTAL) ? nomes[fase] : "";
}

// ================================================================== rodadas
static const char *const CONECTIVOS[CONECTIVOS_ESCOLHA] = {"NÃO", "E", "OU", "SE…ENTÃO"};

const char *dialogo_conectivo(int indice) {
    return (indice >= 0 && indice < CONECTIVOS_ESCOLHA) ? CONECTIVOS[indice] : "";
}

// Ordem de exibição; a resposta certa de cada rodada está em rodadas.c.
static const char *const TRADUCOES[3][3] = {
    {"\"Nós somos perigo.\"", "\"Nós não somos perigo.\"", "\"Não há nós sem perigo.\""},
    {"\"Você ou nós falamos.\"", "\"Se você fala, nós falamos.\"", "\"Você e nós falamos.\""},
    {"\"Se você falar, então a porta abre.\"", "\"A porta abre e você fala.\"", "\"Se a porta abrir, você fala.\""},
};

const char *dialogo_opcao_traducao(int rodada, int opcao) {
    if (rodada < 1 || rodada > 3 || opcao < 0 || opcao > 2) return "";
    return TRADUCOES[rodada - 1][opcao];
}

// ================================================================== léxico (ordem dos glifos em glifos.h)
static const char *const SIGNIFICADOS[] = {
    "NÃO", "E", "OU", "SE…ENTÃO", "SE E SÓ SE", "LUZ A", "LUZ B", "NÓS", "VOCÊ", "PERIGO", "FALAR", "PORTA", "ABRIR",
};

const char *dialogo_significado_glifo(int glifo) {
    int n = (int)(sizeof(SIGNIFICADOS) / sizeof(SIGNIFICADOS[0]));
    return (glifo >= 0 && glifo < n) ? SIGNIFICADOS[glifo] : "?";
}

// ================================================================== fichas da resposta final
static const char *const FICHAS[] = {
    "[VOCÊ FALAR]", "[PORTA ABRIR]", "[NÃO]", "[E]", "[OU]", "[SE…ENTÃO]", "[APAGAR]", "[TRANSMITIR]",
};

const char *dialogo_ficha(int indice) {
    return (indice >= 0 && indice < 8) ? FICHAS[indice] : "";
}
