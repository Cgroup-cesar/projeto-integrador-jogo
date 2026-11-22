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
    FALAS_PORTA_TRANCADA_FALSO, // as três de cima depois que o Varela desceu (o Precursor no rádio)
    FALAS_RODADA_RECOMECA_FALSO,
    FALAS_PEGO_FALSO,
    FALAS_TOTAL
} BlocoFalas;

const Legenda *dialogo_falas(BlocoFalas bloco, int *n);
// A versão do Precursor imitando o Varela, para as falas que podem acontecer depois que ele desceu (pego,
// vidro rompido, porta trancada). Sem versão falsa, devolve o próprio bloco. Quem decide quando é
// regras_radio_falso.
BlocoFalas dialogo_versao_falsa(BlocoFalas bloco);

// Linhas datilografadas da intro (tela preta). Podem ter marcadores: passe por dialogo_personaliza.
#define INTRO_LINHAS 3
const char *dialogo_intro(int linha);

// Textos soltos da interface e das rodadas.
typedef enum {
    TXT_MENU_INICIAR,
    TXT_MENU_SAIR,
    TXT_MENU_AVISO_PROVISORIO,
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
    TXT_HUD_SANIDADE,
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
    TXT_TERMINAL_CABECALHO,
    TXT_TERMINAL_PLACAR,
    TXT_TERMINAL_LUZ_UMA,
    TXT_TERMINAL_LUZ_DUAS,
    TXT_TERMINAL_COLUNA,
    TXT_TERMINAL_COLUNA_UNICA,
    TXT_TERMINAL_ESTADO_LUZ,
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
    TXT_TECLA_LUZ_A,
    TXT_TECLA_AJUDA,
    TXT_TECLA_FECHAR_AJUDA,
    TXT_TERMINAL_OBSERVE_DUAS,
    TXT_TERMINAL_SELO_AJUDA,
    TXT_TERMINAL_SELO_FECHAR,
    TXT_TECLA_LUZ_B,
    TXT_TECLA_ESCOLHER,
    TXT_TECLA_CONFIRMAR,
    TXT_TECLA_SAIR,
    TXT_TECLA_FECHAR,
    TXT_TECLA_LIGAR,
    TXT_TECLA_CHAVES,
    TXT_TECLA_CHAVE_A,
    TXT_TECLA_CHAVE_B,
    TXT_TECLA_CHAVE_C,
    TXT_TECLA_ENCAIXE,
    TXT_TECLA_PORTA,
    TXT_TECLA_MOVER,
    TXT_TECLA_USAR_FICHA,
    TXT_TECLA_ANDAR,
    TXT_TECLA_OLHAR,
    TXT_TECLA_CORRER,
    TXT_TECLA_INTERAGIR,
    TXT_TECLA_LANTERNA,
    TXT_TECLA_PAUSA,
    TXT_TECLA_SAIR_ARMARIO,
    TXT_COLA_TITULO,
    TXT_COLA_NAO,
    TXT_COLA_E,
    TXT_COLA_OU,
    TXT_COLA_SE,
    TXT_COLA_XOU, // as duas portas que só existem no painel 2
    TXT_COLA_NE,
    TXT_COLA_COMPARE,
    TXT_COLA_SUA_FRASE,
    TXT_COLA_SUA_TABELA,
    TXT_TERMINAL_EM_ANALISE,
    TXT_TABELA_LUZ_A,
    TXT_TABELA_LUZ_B,
    TXT_TABELA_RESPOSTA,
    TXT_TABELA_VERDADE,
    TXT_TABELA_FALSO,
    TXT_PAINEL_OBJETIVO1,
    TXT_PAINEL_OBJETIVO2,
    TXT_MENU_RANKING, // ranking e avaliação
    TXT_MENU_GANCHO,
    TXT_MENU_DATA_FITA,
    TXT_FITA_DATA,
    TXT_SUBLIMINAR_1, // as frases que piscam por um instante na tela de início
    TXT_SUBLIMINAR_2,
    TXT_SUBLIMINAR_3,
    TXT_SUBLIMINAR_4,
    TXT_SUBLIMINAR_5,
    TXT_MENU_LIDER,
    TXT_MENU_CTA,
    TXT_MENU_CTA_VAZIO,
    TXT_RANKING_TITULO,
    TXT_RANKING_VAZIO,
    TXT_RANKING_VOCE,
    TXT_RANKING_CTA_FIM,
    TXT_FIM_POSICAO,
    TXT_FIM_FORA,
    TXT_AVALIA_TITULO,
    TXT_AVALIA_P1,
    TXT_AVALIA_P2,
    TXT_AVALIA_P3,
    TXT_TECLA_PERGUNTA,
    TXT_TECLA_ESTRELAS,
    TXT_TECLA_ENVIAR,
    TXT_TECLA_MENU,
    TXT_RESP_TITULO, // segurar a respiração no armário
    TXT_RESP_INTRO,
    TXT_RESP_PASSO1,
    TXT_RESP_PASSO2,
    TXT_RESP_PASSO3,
    TXT_RESP_DEMO,
    TXT_RESP_ACERTOS,
    TXT_RESP_ACERTOS_DICA,
    TXT_RESP_CHANCES,
    TXT_RESP_CHANCES_DICA,
    TXT_RESP_APERTE,
    TXT_RESP_SEGUROU,
    TXT_RESP_AFASTANDO,
    TXT_RESP_ASSIM,
    TXT_RESP_COMECAR,
    TXT_RESP_OUVIU,
    TXT_RESP_FOI,
    TXT_TECLA_RESPIRAR,
    TXT_PAINEL_BARULHO, // o preço do chute e a pergunta da vez
    TXT_PAINEL_MOVIMENTOS,
    TXT_PAINEL_BONUS,
    TXT_PAINEL_DICA_E,
    TXT_PAINEL_DICA_OU,
    TXT_PAINEL_DICA_NAO,
    TXT_PAINEL2_DICA,
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
