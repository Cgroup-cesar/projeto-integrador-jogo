// dialogo.c — todos os textos do jogo. Para mudar uma fala, mude só aqui.
// Duração das legendas em segundos (o jogador pode ler com calma; o rádio não tem pressa).
#include "dialogo.h"

// ================================================================== falas (legendas)
// Marcadores: {NOME}, {PROF}, {prof} e {masculino|feminino} (veja dialogo_personaliza).
// O General Varela verdadeiro chama sempre pelo título e pelo nome: "{PROF} {NOME}" (Professora Inês).
// Quando o rádio chama SÓ pelo nome ("{NOME}", sem o título), é o Precursor imitando a voz dele: é a pista do
// plot twist (o bilhete do Arquivo avisa). Não use {NOME} sozinho nas falas do Varela verdadeiro.
#define V FALANTE_VARELA
#define J FALANTE_JOGADOR

static const Legenda INTRO_INTERFONE[] = {
    // a carta de boas-vindas, pelo rádio
    {"Olá, {PROF} {NOME}. General Varela. Bem-vind{o|a} ao Programa Precursor.", 5.0f, V},
    {"Seu trabalho: entender o que ele diz. E nada de política, religião... muito menos futebol.", 5.5f, V},
    {"Ele aprende rápido. Rápido demais. Vai tentar te enganar.", 4.0f, V},
    {"Uma regra só: nunca deixe as luzes se apagarem. Estou no rádio. Qualquer coisa, grite.", 5.5f, V},
};
static const Legenda APOS_R1[] = {
    {"'Nós não somos perigo.' ...É o que todos dizem, {PROF} {NOME}.", 4.0f, V},
};
static const Legenda INICIO_S1[] = {
    {"As luzes caíram, {PROF} {NOME}. O gerador fica no fim do corredor.", 4.0f, V},
    {"Vá. E não corra: ele ouve.", 3.0f, V},
};
static const Legenda FIM_S1[] = {
    {"Luzes de volta, {PROF} {NOME}. Ele está na cela, como se nunca tivesse saído. Volte.", 5.0f, V},
};
static const Legenda APOS_R2[] = {
    {"'Você e nós falamos'... {PROF} {NOME}, ele está aprendendo a nossa língua.", 5.0f, V},
};
static const Legenda INICIO_S2[] = {
    {"De novo, {PROF} {NOME}. O painel da contenção fica no Arquivo.", 4.0f, V},
    {"Eu vou descer até a cela. Se eu não voltar...", 3.5f, V},
};
static const Legenda FIM_S2[] = {
    // já é o Precursor imitando o Varela: só o nome, sem o título (a pista)
    {"Contenção estável, {NOME}. Pode voltar.", 3.5f, V},
};
static const Legenda APOS_R3[] = {
    // o Precursor imitando o Varela: só o nome, sem o título (a pista)
    {"{NOME}, ele quer conversar. Transmita: se você falar, a porta abre.", 4.5f, V},
    {"Confie em mim.", 2.5f, V},
};
static const Legenda FIM_BOM_FALAS[] = {
    {"...{PROF} {NOME}? É o Varela. O de verdade. O que você disse a ele?", 5.0f, V},
    {"Nada que pudesse ser verdade.", 3.5f, J},
};
static const Legenda PORTA_TRANCADA[] = {
    {"A porta fica trancada enquanto você trabalha, {PROF} {NOME}. Volte ao console.", 4.5f, V},
};
static const Legenda RODADA_RECOMECA[] = {
    {"Vidro substituído. Recomece a leitura, {PROF} {NOME}. Com mais cuidado.", 4.5f, V},
};
static const Legenda PEGO_FALAS[] = {
    {"...{PROF} {NOME}? Responda! — Ele voltou ao começo do corredor. Tente de novo.", 4.5f, V},
};

#undef V
#undef J

typedef struct {
    const Legenda *falas;
    int n;
} Bloco;

#define BLOCO(vetor) {vetor, (int)(sizeof(vetor) / sizeof(vetor[0]))}

static const Bloco BLOCOS[FALAS_TOTAL] = {
    BLOCO(INTRO_INTERFONE), BLOCO(APOS_R1),        BLOCO(INICIO_S1),       BLOCO(FIM_S1),
    BLOCO(APOS_R2),         BLOCO(INICIO_S2),      BLOCO(FIM_S2),          BLOCO(APOS_R3),
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
    "CHÃ DE ALEGRIA, PERNAMBUCO — AGOSTO DE 1985.",
    "Há dois meses, algo desceu da nave que paira sobre a cidade.",
    "Você é {o|a} {PROF} {NOME}, de linguística da UFPE. Nunca traduziu nada vivo.",
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
    [TXT_ID_PERGUNTA_GENERO] = "Professor ou professora?",
    [TXT_ID_PROFESSOR] = "PROFESSOR",
    [TXT_ID_PROFESSORA] = "PROFESSORA",
    [TXT_ID_DICA_GENERO] = "← → escolhem · ENTER confirma",
    [TXT_ID_PERGUNTA_NOME] = "Qual é o seu nome, {prof}?",
    [TXT_ID_DICA_NOME] = "Até 16 letras · ENTER confirma · sem nome, fica \"%s\"",
    [TXT_NOME_PADRAO] = "Moura",
    [TXT_PROF_MAIUSCULA_M] = "Professor",
    [TXT_PROF_MAIUSCULA_F] = "Professora",
    [TXT_PROF_MINUSCULA_M] = "professor",
    [TXT_PROF_MINUSCULA_F] = "professora",
    [TXT_HUD_VIDRO] = "VIDRO",
    [TXT_HUD_PLACAR] = "PLACAR",
    [TXT_HUD_FOLEGO] = "FÔLEGO",
    [TXT_MIRA_CONSOLE] = "E — usar o console",
    [TXT_MIRA_PAINEL] = "E — abrir o painel de circuito",
    [TXT_MIRA_DIARIO] = "E — ler o bilhete",
    [TXT_MIRA_ARMARIO] = "E — esconder-se no armário",
    [TXT_MIRA_PORTA] = "E — porta da sala de observação",
    [TXT_ARMARIO_SAIR] = "E — sair do armário",
    [TXT_OBJETIVO_SALA] = "Vá ao console (E) e descubra o que ele diz.",
    [TXT_OBJETIVO_S1] = "Religue o gerador: painel 1, no fim do corredor (ao norte).",
    [TXT_OBJETIVO_S2] = "Religue a contenção: painel 2, no Arquivo, ao sul da sala.",
    [TXT_OBJETIVO_VOLTAR] = "Volte para a sala de observação.",
    [TXT_OBJETIVO_RESPOSTA] = "Responda pelo console.",
    [TXT_TERMINAL_TITULO] = "ANALISADOR SEMÂNTICO SNI-85",
    [TXT_TERMINAL_DICA_R1_1] = "Tabela-verdade: para cada combinação das luzes, ele diz VERDADEIRO ou FALSO.",
    [TXT_TERMINAL_DICA_R1_2] = "Teste todas as combinações; o padrão das respostas revela o conectivo.",
    [TXT_TERMINAL_OBSERVE] = "Mude as luzes e ouça a resposta. A tabela se preenche sozinha.",
    [TXT_TERMINAL_ESCOLHA_CONECTIVO] = "O que significa o glifo âmbar?  (setas + ENTER)",
    [TXT_TERMINAL_TRADUZA] = "Ele está falando. Qual é a tradução?  (setas + ENTER)",
    [TXT_TERMINAL_ERRO] = "INCORRETO. O VIDRO CEDE.",
    [TXT_TERMINAL_CERTO] = "CORRETO.",
    [TXT_TERMINAL_RESPONDENDO] = "...ELE RESPONDE...",
    [TXT_TERMINAL_SAIR] = "Esc — sair do terminal",
    [TXT_TERMINAL_ANOTACAO] = "ANOTAÇÃO RECUPERADA: \"uma frase que nunca possa ser verdade\"",
    [TXT_TERMINAL_RESPONDA] = "RESPONDA.",
    [TXT_TERMINAL_SINTAXE_INVALIDA] = "SINTAXE INVÁLIDA",
    [TXT_TERMINAL_TRANSMITINDO] = "TRANSMITINDO...",
    [TXT_TERMINAL_FRASE] = "FRASE:",
    [TXT_TERMINAL_FICHAS] = "Monte a resposta com as fichas (setas + ENTER).",
    [TXT_TERMINAL_LEGENDA_PQ] = "P = VOCÊ FALAR · Q = PORTA ABRIR",
    [TXT_TERMINAL_FRASE_CHEIA] = "No máximo 7 fichas por frase.",
    [TXT_TERMINAL_INCOMPLETA] = "(frase incompleta)",
    [TXT_TERMINAL_FRASE_OBSERVADA] = "FRASE OBSERVADA",
    [TXT_TERMINAL_LUZ_ACESA] = "ACESA",
    [TXT_TERMINAL_LUZ_APAGADA] = "APAGADA",
    [TXT_TERMINAL_TABELA] = "TABELA-VERDADE",
    [TXT_TERMINAL_QUAL_FRASE] = "Frase %d de %d",
    [TXT_TERMINAL_CABECALHO] = "RODADA %d/3 · VIDRO:",
    [TXT_TERMINAL_PLACAR] = "· PLACAR: %03d",
    [TXT_TERMINAL_LUZ_UMA] = "LUZ A: %s      [1] alterna",
    [TXT_TERMINAL_LUZ_DUAS] = "LUZ A: %s   LUZ B: %s      [1] [2] alternam",
    [TXT_TERMINAL_COLUNA] = "frase %d",
    [TXT_TERMINAL_COLUNA_UNICA] = "frase",
    [TXT_PAINEL_CHAVE] = "[%d] %s  %s",
    [TXT_FALANTE_VARELA] = "GEN. VARELA (rádio)",
    [TXT_FALANTE_JOGADOR] = "{PROF} {NOME}",
    [TXT_PAINEL1_TITULO] = "PAINEL 1 — GERADOR AUXILIAR",
    [TXT_PAINEL2_TITULO] = "PAINEL 2 — CONTENÇÃO (ARQUIVO)",
    [TXT_PAINEL_INSTRUCOES1] = "[1] [2] [3] ligam e desligam as chaves A, B, C · Esc fecha",
    [TXT_PAINEL_INSTRUCOES2] = "← → escolhem o encaixe · ↑ ↓ trocam a porta · Esc fecha",
    [TXT_PAINEL_RESOLVIDO] = "SAÍDA ACESA — ENERGIA RESTABELECIDA",
    [TXT_PAINEL_ALVO] = "ALVO",
    [TXT_PAINEL_SAIDA] = "SAÍDA",
    [TXT_PAINEL_CHAVE_LIGADA] = "LIGADA",
    [TXT_PAINEL_CHAVE_DESLIGADA] = "DESLIGADA",
    [TXT_DIARIO] = // o bilhete do Varela verdadeiro, no Arquivo: a pista do final bom
    "Ele aprende as nossas vozes. Eu nunca te chamo só pelo nome: se o rádio te chamar assim, não sou eu. "
    "Ele só age sobre o que PODE ser verdade. Dê a ele uma frase que nunca possa ser. "
    "Algo como: você fala e você não fala (P ∧ ¬P). — V.",
    [TXT_DIARIO_FECHAR] = "E ou Esc — fechar",
    [TXT_FIM_RUIM_ACEITA] = "TRANSMISSÃO ACEITA.",
    [TXT_FIM_RUIM_OBRIGADO] = "OBRIGADO, {NOME}.", // só o nome, sem o título: como o rádio falso
    [TXT_FIM_RUIM_TELA] = "FINAL 2 DE 3 — A PORTA. Você abriu a porta para ele. Você nunca saiu do bloco. "
                          "Em agosto de 1985, todas as TVs de Pernambuco exibiram o mesmo símbolo.",
    [TXT_FIM_BOM_TELA] =
        "FINAL 1 DE 3 — O SILÊNCIO. Você calou o Precursor e saiu viv{o|a} de Chã de Alegria. "
        "Às 22:00, a nave foi embora. Oficialmente, nada desceu dela.",
    // A explicação de cada final, logo abaixo do texto: %s é a frase transmitida, em notação lógica (P, Q).
    [TXT_FIM_BOM_PORQUE] =
        "POR QUE VOCÊ VENCEU: a sua frase (%s) é uma contradição. Ela é falsa em todas as linhas "
        "da tabela-verdade. O Precursor só age sobre o que PODE ser verdade: sem nenhuma brecha, "
        "ele travou.",
    [TXT_FIM_RUIM_PORQUE] = "POR QUE VOCÊ PERDEU: a sua frase (%s) pode ser verdadeira em pelo menos uma "
                            "linha da tabela-verdade, e essa "
                            "brecha bastou para ele agir. O rádio que te chamava só pelo nome não era o "
                            "Varela. Da próxima vez, leia o "
                            "bilhete no Arquivo e transmita uma frase que nunca possa ser verdade.",
    [TXT_FIM_VER_PLACAR] = "ENTER — ver o placar  (%d)",
    [TXT_FIM_VITORIA] = "VITÓRIA",
    [TXT_FIM_DERROTA] = "DERROTA",
    [TXT_FIM_PLACAR] = "PLACAR TOTAL",
    [TXT_FIM_TEMPO] = "TEMPO",
    [TXT_FIM_ERROS] = "ERROS",
    [TXT_FIM_ENTER] = "Pressione ENTER",
    [TXT_FIM_RODAPE] = "Projeto Integrador PIF · nome provisório",
    [TXT_FIM_JOGADOR] = "{PROF} {NOME}",
    [TXT_TERMINAL_OPERADOR] = "{OPERADOR|OPERADORA}: {PROF} {NOME}", // no alto do analisador
    [TXT_ID_AVISO_NOME] = "Seu nome fica só nesta partida: nada é gravado e ele é apagado ao voltar ao menu.",
    [TXT_PLACA_POSTO] = "BLOCO 85", // placas pintadas em estêncil nas paredes (cenario.c)
    [TXT_PLACA_RESTRITA] = "ÁREA RESTRITA",
    [TXT_PLACA_ARQUIVO] = "ARQUIVO",
    // Filtro VHS (F1 alterna): "%d/%d · nome" aparece no canto por alguns segundos.
    [TXT_VHS_ROTULO] = "FILTRO VHS %d/%d · %s",
    [TXT_VHS_DESLIGADO] = "DESLIGADO",
    [TXT_VHS_ORIGINAL] = "ORIGINAL",
    [TXT_VHS_FITA_CASEIRA] = "FITA CASEIRA (1985)",
    [TXT_VHS_FITA_GASTA] = "FITA GASTA",
    [TXT_VHS_MONITOR_CRT] = "MONITOR CRT",
    [TXT_VHS_VIGILANCIA] = "CÂMERA DE VIGILÂNCIA",
};

const char *dialogo_texto(TextoId id) {
    if (id < 0 || id >= TXT_TOTAL || TEXTOS[id] == NULL) return "";
    return TEXTOS[id];
}

void dialogo_personaliza(const Identidade *id, const char *modelo, char *saida, size_t tamanho) {
    const Marcador marcadores[] = {
        {"NOME", id->nome},
        {"PROF", dialogo_texto(id->feminino ? TXT_PROF_MAIUSCULA_F : TXT_PROF_MAIUSCULA_M)},
        {"prof", dialogo_texto(id->feminino ? TXT_PROF_MINUSCULA_F : TXT_PROF_MINUSCULA_M)},
    };
    regras_monta_texto(modelo, marcadores, (int)(sizeof(marcadores) / sizeof(marcadores[0])), id->feminino,
                       saida, tamanho);
}

const char *dialogo_nome_fase(int fase) {
    static const char *const nomes[FASE_TOTAL] = {
        "",        "Intro",    "Rodada 1",       "Saída 1", "Rodada 2",
        "Saída 2", "Rodada 3", "Resposta final", "Final",   "Final",
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
    {"\"Se você falar, então a porta abre.\"", "\"A porta abre e você fala.\"",
     "\"Se a porta abrir, você fala.\""},
};

const char *dialogo_opcao_traducao(int rodada, int opcao) {
    if (rodada < 1 || rodada > 3 || opcao < 0 || opcao > 2) return "";
    return TRADUCOES[rodada - 1][opcao];
}

// ================================================================== léxico (ordem dos glifos em glifos.h)
static const char *const SIGNIFICADOS[] = {
    "NÃO", "E",    "OU",     "SE…ENTÃO", "SE E SÓ SE", "LUZ A", "LUZ B",
    "NÓS", "VOCÊ", "PERIGO", "FALAR",    "PORTA",      "ABRIR",
};

const char *dialogo_significado_glifo(int glifo) {
    int n = (int)(sizeof(SIGNIFICADOS) / sizeof(SIGNIFICADOS[0]));
    return (glifo >= 0 && glifo < n) ? SIGNIFICADOS[glifo] : "?";
}

// ================================================================== portas lógicas dos painéis
static const char *const PORTAS[] = {"E", "OU", "OU-EXCLUSIVO", "NÃO-E", "NÃO"};

const char *dialogo_porta(int tipo) {
    return (tipo >= 0 && tipo < 5) ? PORTAS[tipo] : "?";
}

// ================================================================== fichas da resposta final
static const char *const FICHAS[] = {
    "[VOCÊ FALAR]", "[PORTA ABRIR]", "[NÃO]", "[E]", "[OU]", "[SE…ENTÃO]", "[APAGAR]", "[TRANSMITIR]",
};

const char *dialogo_ficha(int indice) {
    return (indice >= 0 && indice < 8) ? FICHAS[indice] : "";
}
