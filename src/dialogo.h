// dialogo.h — todos os textos do jogo, numa tabela só, para o roteirista editar sem tocar na lógica.
#ifndef DIALOGO_H
#define DIALOGO_H

#include "estado.h"

// Blocos de falas (legendas com bip de rádio), na ordem em que aparecem na partida.
typedef enum {
    FALAS_INTRO_INTERFONE,
    FALAS_APOS_R1,
    FALAS_INICIO_S1,
    FALAS_FIM_S1,
    FALAS_APOS_R2,
    FALAS_INICIO_S2,
    FALAS_FIM_S2,
    FALAS_APOS_R3,
    FALAS_FIM_BOM,
    FALAS_PORTA_TRANCADA,
    FALAS_RODADA_RECOMECA,
    FALAS_PEGO,
    FALAS_TOTAL
} BlocoFalas;

const Legenda *dialogo_falas(BlocoFalas bloco, int *n);

// Linhas datilografadas da intro (tela preta). Podem ter marcadores: passe por dialogo_personaliza.
#define INTRO_LINHAS 3
const char *dialogo_intro(int linha);

// Textos soltos da interface e das rodadas.
typedef enum {
    TXT_MENU_INICIAR,
    TXT_MENU_SAIR,
    TXT_MENU_AVISO_PROVISORIO,
    TXT_MENU_CONTROLES,
    TXT_MENU_DICA,
    TXT_PAUSA,
    TXT_PAUSA_CONTINUAR,
    TXT_PAUSA_MENU,
    TXT_INTRO_PULAR,
    TXT_ID_PERGUNTA_GENERO,
    TXT_ID_PROFESSOR,
    TXT_ID_PROFESSORA,
    TXT_ID_DICA_GENERO,
    TXT_ID_PERGUNTA_NOME,
    TXT_ID_DICA_NOME,
    TXT_NOME_PADRAO,
    TXT_PROF_MAIUSCULA_M,
    TXT_PROF_MAIUSCULA_F,
    TXT_PROF_MINUSCULA_M,
    TXT_PROF_MINUSCULA_F,
    TXT_HUD_VIDRO,
    TXT_HUD_PLACAR,
    TXT_HUD_FOLEGO,
    TXT_MIRA_CONSOLE,
    TXT_MIRA_PAINEL,
    TXT_MIRA_DIARIO,
    TXT_MIRA_ARMARIO,
    TXT_MIRA_PORTA,
    TXT_ARMARIO_SAIR,
    TXT_OBJETIVO_SALA,
    TXT_OBJETIVO_S1,
    TXT_OBJETIVO_S2,
    TXT_OBJETIVO_VOLTAR,
    TXT_OBJETIVO_RESPOSTA,
    TXT_TERMINAL_TITULO,
    TXT_TERMINAL_DICA_R1_1,
    TXT_TERMINAL_DICA_R1_2,
    TXT_TERMINAL_OBSERVE,
    TXT_TERMINAL_ESCOLHA_CONECTIVO,
    TXT_TERMINAL_TRADUZA,
    TXT_TERMINAL_ERRO,
    TXT_TERMINAL_CERTO,
    TXT_TERMINAL_RESPONDENDO,
    TXT_TERMINAL_SAIR,
    TXT_TERMINAL_ANOTACAO,
    TXT_TERMINAL_RESPONDA,
    TXT_TERMINAL_SINTAXE_INVALIDA,
    TXT_TERMINAL_TRANSMITINDO,
    TXT_TERMINAL_FRASE,
    TXT_TERMINAL_FICHAS,
    TXT_TERMINAL_LEGENDA_PQ,
    TXT_TERMINAL_FRASE_CHEIA,
    TXT_TERMINAL_INCOMPLETA,
    TXT_TERMINAL_FRASE_OBSERVADA,
    TXT_TERMINAL_LUZ_ACESA,
    TXT_TERMINAL_LUZ_APAGADA,
    TXT_TERMINAL_TABELA,
    TXT_TERMINAL_QUAL_FRASE,
    TXT_TERMINAL_CABECALHO,
    TXT_TERMINAL_PLACAR,
    TXT_TERMINAL_LUZ_UMA,
    TXT_TERMINAL_LUZ_DUAS,
    TXT_TERMINAL_COLUNA,
    TXT_TERMINAL_COLUNA_UNICA,
    TXT_PAINEL_CHAVE,
    TXT_FALANTE_VARELA,
    TXT_FALANTE_JOGADOR,
    TXT_PAINEL1_TITULO,
    TXT_PAINEL2_TITULO,
    TXT_PAINEL_INSTRUCOES1,
    TXT_PAINEL_INSTRUCOES2,
    TXT_PAINEL_RESOLVIDO,
    TXT_PAINEL_ALVO,
    TXT_PAINEL_SAIDA,
    TXT_PAINEL_CHAVE_LIGADA,
    TXT_PAINEL_CHAVE_DESLIGADA,
    TXT_DIARIO,
    TXT_DIARIO_FECHAR,
    TXT_FIM_RUIM_ACEITA,
    TXT_FIM_RUIM_OBRIGADO,
    TXT_FIM_RUIM_TELA,
    TXT_FIM_BOM_TELA,
    TXT_FIM_BOM_PORQUE,
    TXT_FIM_RUIM_PORQUE,
    TXT_FIM_VER_PLACAR,
    TXT_FIM_VITORIA,
    TXT_FIM_DERROTA,
    TXT_FIM_PLACAR,
    TXT_FIM_TEMPO,
    TXT_FIM_ERROS,
    TXT_FIM_ENTER,
    TXT_FIM_RODAPE,
    TXT_FIM_JOGADOR,
    TXT_TERMINAL_OPERADOR,
    TXT_ID_AVISO_NOME,
    TXT_PLACA_POSTO,
    TXT_PLACA_RESTRITA,
    TXT_PLACA_ARQUIVO,
    TXT_VHS_ROTULO,
    TXT_VHS_DESLIGADO,
    TXT_VHS_ORIGINAL,
    TXT_VHS_FITA_CASEIRA,
    TXT_VHS_FITA_GASTA,
    TXT_VHS_MONITOR_CRT,
    TXT_VHS_VIGILANCIA,
    TXT_TOTAL
} TextoId;

const char *dialogo_texto(TextoId id);

// Troca os marcadores de um texto pelo que o jogador escolheu no início: {NOME}, {PROF} ("Professor" ou
// "Professora"), {prof} (minúsculo) e {masculino|feminino} (ex.: "viv{o|a}"). A regra está em
// regras_monta_texto (pura); aqui só ficam as palavras.
void dialogo_personaliza(const Identidade *id, const char *modelo, char *saida, size_t tamanho);

// Nome de cada fase na tela de fim (placar por fase).
const char *dialogo_nome_fase(int fase);

// Conteúdo das rodadas: nomes dos conectivos e as 3 opções de tradução (a ordem de exibição é a da tabela).
#define CONECTIVOS_ESCOLHA 4
const char *dialogo_conectivo(int indice); // NÃO, E, OU, SE…ENTÃO
const char *dialogo_opcao_traducao(int rodada, int opcao);

// Léxico do analisador: o que cada glifo de palavra significa (conectivos: só depois de decifrados).
const char *dialogo_significado_glifo(int glifo);

// Nome de cada porta lógica dos painéis (ordem de TipoPorta em regras.h).
const char *dialogo_porta(int tipo);

// Fichas da resposta final.
const char *dialogo_ficha(int indice);

#endif
