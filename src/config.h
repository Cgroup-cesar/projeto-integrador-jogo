// config.h — constantes ajustáveis do jogo. Mudou um número de balanceamento? É aqui.
// Este arquivo não inclui a raylib: as cores são macros e só viram valor onde a raylib já foi incluída.
#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------- título
// Trocar o nome do jogo no futuro exige mudar só estas duas linhas.
#define TITULO_JOGO "PROJETO PRECURSOR"
#define TITULO_PROVISORIO 1

// ---------------------------------------------------------------- janela e tempo
#define LARGURA_JANELA 1280
#define ALTURA_JANELA 720
#define ESCALA_RENDER 0.60f // a cena 3D é renderizada a 60% da janela e ampliada (bilinear)
#define FPS_ALVO 60
#define DT_MAXIMO 0.05f // limita o dt para um travamento não teletransportar ninguém

// ---------------------------------------------------------------- filtro VHS (sentido, não visto)
#define VHS_LIGADO_AO_INICIAR 1
#define VHS_RUIDO 0.04f
#define VHS_ABERRACAO_PX 1.5f
#define VHS_SCANLINES 0.06f
#define VHS_VINHETA 0.35f
#define VHS_WOBBLE_PX 2.5f

// ---------------------------------------------------------------- debug e modos
#define TECLAS_DEBUG 1          // F1 VHS · F2 pular fase · F3 estado da criatura · F5 reiniciar · F9 resposta · F12 captura
#define FEIRA_INATIVIDADE_S 90.0f // no modo feira (--feira), volta ao menu após esse tempo sem nenhuma tecla
#define FIM_VOLTA_MENU_S 20.0f    // a tela de fim volta sozinha ao menu
#define SMOKE_LIMITE_FASE_S 180.0f // no --smoke, uma fase que demora mais que isso (tempo simulado) é travamento

// ---------------------------------------------------------------- mundo
#define TAMANHO_CELULA 2.0f // 1 célula do mapa = 2 m
#define PE_DIREITO 3.0f     // altura das paredes
#define ALTURA_OLHOS 1.6f

// ---------------------------------------------------------------- jogador
#define JOGADOR_RAIO 0.30f
#define JOGADOR_VEL_ANDAR 2.5f
#define JOGADOR_VEL_CORRER 4.2f
#define JOGADOR_FOLEGO_MAX 4.0f       // segundos de corrida
#define JOGADOR_FOLEGO_RECUPERA 0.8f  // fôlego recuperado por segundo andando/parado
#define SENSIBILIDADE_MOUSE 0.0025f
#define DISTANCIA_INTERACAO 2.4f
#define ANGULO_INTERACAO_GRAUS 35.0f

// ---------------------------------------------------------------- ruídos
#define RUIDO_RAIO_ANDAR 3.0f
#define RUIDO_RAIO_CORRER 10.0f
#define RUIDO_RAIO_PAINEL 6.0f
#define RUIDO_RAIO_PORTA 5.0f
#define RUIDO_DECAIMENTO 0.9f      // intensidade perdida por segundo
#define RUIDO_LIMIAR_PERCEBIDO 0.15f
#define INTERVALO_PASSO_ANDAR 0.55f
#define INTERVALO_PASSO_CORRER 0.33f

// ---------------------------------------------------------------- o Precursor
#define CRIATURA_VEL_PATRULHA 1.6f
#define CRIATURA_VEL_PERSEGUE 3.6f
#define CRIATURA_VEL_INVESTIGA 2.2f
#define CRIATURA_VISAO_GRAUS 70.0f // cone total
#define CRIATURA_VISAO_DIST 14.0f
#define CRIATURA_PROCURA_S 6.0f
#define CRIATURA_SURGE_S 15.0f     // tempo até ele sair do duto no início de cada saída
#define CRIATURA_RAIO_CAPTURA 0.5f // duas esferas de 0,5 m: pego quando a distância < 1 m
#define CRIATURA_DIST_ARMARIO 2.0f // entrar no armário a menos disso enquanto ele vê = pego
#define JUMPSCARE_S 1.2f
#define JUMPSCARE_SILENCIO_S 0.2f

// ---------------------------------------------------------------- sala, vidro e placar
#define VIDAS_VIDRO 3
#define RESPOSTA_PRECURSOR_S 0.9f // tempo que ele leva para "responder" a uma combinação de luzes
#define PONTOS_RODADA_SEM_ERRO 100
#define PONTOS_ERRO (-25)
#define PONTOS_SAIDA_SEM_SER_PEGO 50
#define PONTOS_FINAL_BOM 200
#define TEMPO_ALVO_PARTIDA_S 480.0f // bônus de tempo: meio ponto por segundo abaixo disso
#define MAX_FICHAS_FRASE 7

// ---------------------------------------------------------------- capacidades das coleções (sem malloc por quadro)
#define MAX_RUIDOS 32
#define MAX_PARTICULAS 256
#define MAX_RACHADURAS 24
#define MAX_EVENTOS 64
#define MAX_INTERATIVOS 24
#define MAX_PAREDES 256
#define MAX_LAMPADAS 24
#define MAX_LEGENDAS 8
#define MAX_NOS_LOGICA 64
#define MAX_CAMINHO 128

// ---------------------------------------------------------------- luz e névoa
#define LUZ_AMBIENTE 0.035f
#define NEVOA_DENSIDADE 0.085f

// ---------------------------------------------------------------- paleta (só use onde a raylib já foi incluída)
#define COR_SOMBRA (Color){0x0c, 0x0f, 0x0c, 255}
#define COR_CONCRETO (Color){0x2b, 0x3a, 0x33, 255}
#define COR_OLIVA (Color){0x4a, 0x4f, 0x3a, 255}
#define COR_LAMPADA (Color){0xd9, 0xa4, 0x41, 255}
#define COR_FOSFORO (Color){0x7d, 0xff, 0x9a, 255}
#define COR_VIDRO (Color){0x5a, 0xa0, 0xc8, 255}
#define COR_ALARME (Color){0xff, 0x5a, 0x4f, 255}
#define COR_PELE (Color){0xc9, 0xd4, 0xdc, 255}
#define COR_AMBAR (Color){0xd9, 0xa4, 0x41, 255}

#endif
