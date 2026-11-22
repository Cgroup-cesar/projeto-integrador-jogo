// config.h — constantes ajustáveis do jogo. Mudou um número de balanceamento? É aqui.
// Este arquivo não inclui a raylib: as cores são macros e só viram valor onde a raylib já foi incluída.
#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------- título
// Trocar o nome do jogo no futuro exige mudar só estas duas linhas.
#define TITULO_JOGO       "PROJETO PRECURSOR"
#define TITULO_PROVISORIO 1

// ---------------------------------------------------------------- janela e tempo
#define LARGURA_JANELA 1280
#define ALTURA_JANELA  720
#define ESCALA_RENDER  0.60f // a cena 3D é renderizada a 60% da janela e ampliada (bilinear)
#define FPS_ALVO       60
#define DT_MAXIMO      0.05f // limita o dt para um travamento não teletransportar ninguém

// ---------------------------------------------------------------- filtro VHS (sentido, não visto)
#define VHS_LIGADO_AO_INICIAR 1
#define VHS_RUIDO             0.04f
#define VHS_ABERRACAO_PX      1.5f
#define VHS_SCANLINES         0.06f
#define VHS_VINHETA           0.35f
#define VHS_WOBBLE_PX         2.5f

// ---------------------------------------------------------------- debug e modos
#define TECLAS_DEBUG                                                                                         \
    1 // F1 VHS · F2 pular fase · F3 estado da criatura · F5 reiniciar · F9 resposta · F12 captura
#define FEIRA_INATIVIDADE_S 90.0f // no modo feira (--feira), volta ao menu após esse tempo sem nenhuma tecla
#define FIM_VOLTA_MENU_S    20.0f // a tela de fim volta sozinha ao menu
#define FIM_RUIM_TEXTO_S    8.2f  // final ruim: quando o texto e o "por quê" aparecem (depois do susto)
#define FIM_BOM_TEXTO_S     13.0f // final bom: quando o texto e o "por quê" aparecem (depois das falas)
#define FIM_LEITURA_MAX_S   50.0f // o texto espera a leitura: ENTER ou, no máximo, isso, e vai ao placar
// O rádio falso (o Precursor imitando o Varela, a partir da S2)
#define RADIO_FALSO_TOM    0.78f // tom da voz do rádio (1 = normal; menor = mais grave)
#define RADIO_FALSO_CANTO  0.35f // o canto do Precursor por baixo da fala, em fração do volume da voz dele
#define FALHA_INTERVALO_S  1.4f  // a legenda falha mais ou menos uma vez a cada intervalo...
#define FALHA_DURACAO_S    0.18f // ...por este tempo
#define FALHA_LETRAS       0.35f // fração das letras trocadas por símbolos enquanto falha
#define IDENTIDADE_FEIRA_S 20.0f // no modo feira, "professor ou professora?" e o nome seguem sozinhos
#define SMOKE_LIMITE_FASE_S                                                                                  \
    180.0f // no --smoke, uma fase que demora mais que isso (tempo simulado) é travamento

// ---------------------------------------------------------------- mundo
#define TAMANHO_CELULA 2.0f // 1 célula do mapa = 2 m
#define PE_DIREITO     3.0f // altura das paredes
#define ALTURA_OLHOS   1.6f

// ---------------------------------------------------------------- jogador
#define JOGADOR_RAIO            0.30f
#define JOGADOR_VEL_ANDAR       2.5f
#define JOGADOR_VEL_CORRER      4.2f
#define JOGADOR_FOLEGO_MAX      4.0f // segundos de corrida
#define JOGADOR_FOLEGO_RECUPERA 0.8f // fôlego recuperado por segundo andando/parado
#define SENSIBILIDADE_MOUSE     0.0025f
#define DISTANCIA_INTERACAO     2.4f
#define ANGULO_INTERACAO_GRAUS  35.0f

// ---------------------------------------------------------------- ruídos
#define RUIDO_RAIO_ANDAR       3.0f
#define RUIDO_RAIO_CORRER      10.0f
#define RUIDO_RAIO_PAINEL      6.0f
#define RUIDO_RAIO_PORTA       5.0f
#define RUIDO_DECAIMENTO       0.9f // intensidade perdida por segundo
#define RUIDO_LIMIAR_PERCEBIDO 0.15f
#define INTERVALO_PASSO_ANDAR  0.55f
#define INTERVALO_PASSO_CORRER 0.33f

// ---------------------------------------------------------------- o Precursor
#define CRIATURA_VEL_PATRULHA  1.6f
#define CRIATURA_VEL_PERSEGUE  3.6f
#define CRIATURA_VEL_INVESTIGA 2.2f
#define CRIATURA_VISAO_GRAUS   70.0f // cone total
#define CRIATURA_VISAO_DIST    14.0f
#define CRIATURA_PROCURA_S     6.0f
#define CRIATURA_SURGE_S       15.0f // tempo até ele sair do duto no início de cada saída
#define CRIATURA_RAIO_CAPTURA  0.8f  // duas esferas de 0,8 m: pego quando a distância < 1,6 m
#define CRIATURA_BOTE_M        3.0f  // perseguindo e mais perto que isso: avança sem as pausas, mais rápido
#define CRIATURA_BOTE_VEL      1.35f // multiplica a velocidade de perseguição no bote
#define CRIATURA_DIST_ARMARIO  2.0f  // entrar no armário a menos disso enquanto ele vê = pego
#define CRIATURA_ALTURA_M      2.6f  // o precursor.glb é escalado para essa altura, com os pés no chão
// Postura procedural (o modelo .glb não tem animação): ver regras_postura_criatura
#define CRIATURA_PASSADA_M        1.6f   // metros por passada completa (pernas longas)
#define CRIATURA_INCLINA_PARADO   3.0f   // graus para a frente, parado
#define CRIATURA_INCLINA_CORRENDO 18.0f  // graus para a frente, perseguindo
#define CRIATURA_BALANCO_GRAUS    5.0f   // de um lado para o outro a cada passo
#define CRIATURA_SOBE_M           0.05f  // sobe e desce a cada passo
#define CRIATURA_RESPIRA          0.015f // o tronco se expande 1,5% parado
// Susto (jumpscare): o rosto do modelo pula até a câmera e fica tremendo (ver regras_quadro_susto).
// Com assets/audio/jumpscare.mp3, o rosto fica até o fim do trecho alto do som (depois só sobra o eco).
#define JUMPSCARE_S            1.2f  // duração com o som sintetizado (sem assets/audio/jumpscare.*)
#define JUMPSCARE_AUDIO_S      2.04f // duração com o jumpscare.mp3 (medida no arquivo)
#define JUMPSCARE_SILENCIO_S   0.2f
#define JUMPSCARE_PULO_S       0.16f // o pulo: de longe até colado na câmera
#define JUMPSCARE_LONGE_M      2.6f  // distância da câmera ao rosto no começo do pulo
#define JUMPSCARE_PERTO_M      0.2f  // e no fim: o rosto enche a tela
#define JUMPSCARE_TREMOR_HZ    40.0f // posições novas do tremor por segundo
#define JUMPSCARE_TREMOR_M     0.02f // deslocamento máximo do tremor
#define JUMPSCARE_TREMOR_GRAUS 5.0f  // giro máximo do tremor
#define JUMPSCARE_CLARAO_S     0.12f // clarão branco quando ele chega
#define JUMPSCARE_ESCURECE     0.85f // o cenário ao fundo fica 85% mais escuro: o foco é a criatura
#define JUMPSCARE_LUZ          1.1f  // força da luz branca de baixo no rosto (pisca)
#define JUMPSCARE_BOCA_HZ      4.5f  // quantas vezes por segundo a boca abre e fecha
#define JUMPSCARE_BOCA_FECHA   8.0f  // graus que a mandíbula sobe para a boca fechar
#define JUMPSCARE_BOCA_ABRE    28.0f // e escancarada
// Onde fica o rosto no precursor.glb, em fração da caixa do modelo (0 = mínimo, 1 = máximo).
#define JUMPSCARE_ROSTO_X 0.48f
#define JUMPSCARE_ROSTO_Y 0.90f
#define JUMPSCARE_ROSTO_Z 0.72f

// ---------------------------------------------------------------- sala, vidro e placar
#define VIDAS_VIDRO               3
#define RESPOSTA_PRECURSOR_S      0.9f // tempo que ele leva para "responder" a uma combinação de luzes
#define PONTOS_RODADA_SEM_ERRO    100
#define PONTOS_ERRO               (-25)
#define PONTOS_SAIDA_SEM_SER_PEGO 50
#define PONTOS_FINAL_BOM          200
#define TEMPO_ALVO_PARTIDA_S      480.0f // bônus de tempo: meio ponto por segundo abaixo disso
#define MAX_FICHAS_FRASE          7

// ---------------------------------------------------------------- capacidades das coleções (sem malloc por
// quadro)
#define MAX_RUIDOS        32
#define MAX_PARTICULAS    256
#define MAX_RACHADURAS    24
#define MAX_EVENTOS       96
#define MAX_INTERATIVOS   24
#define MAX_PAREDES       256
#define MAX_LAMPADAS      24
#define MAX_LEGENDAS      8
#define MAX_LETRAS_QUADRO 8                         // letras digitadas guardadas por quadro (campo do nome)
#define NOME_MAX_LETRAS   16                        // o nome do jogador tem até 16 letras...
#define NOME_MAX_BYTES    (NOME_MAX_LETRAS * 2 + 1) // ...de até 2 bytes cada em UTF-8 (Latin-1), mais o '\0'
#define MAX_NOS_LOGICA    64
#define MAX_CAMINHO       128

// ---------------------------------------------------------------- texturas do mapa (procedurais; override
// opcional em assets/texturas/<material>.png). Geradas uma vez ao abrir o jogo, com semente fixa.
#define TEXTURA_LADO_PX         256   // lado das texturas quadradas (piso, teto, oliva, metal, porta)
#define TEXTURA_CONCRETO_PX     512   // o concreto é 512 × 256: cobre 6 m × 3 m sem repetir à vista
#define TEXTURA_SEMENTE         1985u // mesma semente → mesma imagem em toda execução
#define TEXTURA_SUJEIRA         0.8f  // 0 = limpo; 1 = manchas, escorridos e descascado no máximo
#define TEXTURA_METROS_CONCRETO 6.0f  // metros por repetição na horizontal (na vertical: PE_DIREITO)
#define TEXTURA_METROS_PISO     2.0f  // 4 × 4 placas de 0,5 m
#define TEXTURA_METROS_TETO     2.0f  // 3 × 3 placas de forro
#define TEXTURA_METROS_METAL    1.0f
#define TEXTURA_ARQUIVO_MAX_PX  1024  // PNG maior que isso (ou fora de potência de 2) é redimensionado
#define BARRADO_ALTURA          1.2f  // tinta oliva do chão até aqui (acima: concreto aparente)
#define DECALQUE_PX             256   // manchas: 256 × 256; placas: 512 × 128
#define HALO_LAMPADA_M          1.3f  // tamanho do brilho em volta das lâmpadas
#define HALO_LAMPADA_ALFA       0.35f // força do brilho (some com a névoa)

// ---------------------------------------------------------------- luz e névoa
#define LUZ_AMBIENTE    0.06f
#define NEVOA_DENSIDADE 0.085f
// Holofotes da cela (luzes A e B): ficam à frente e acima do Precursor, apontados para o peito dele.
#define HOLOFOTE_ABERTURA_GRAUS 30.0f // meia-abertura do cone; fora dele sobram 15% (a cela não fica preta)
#define HOLOFOTE_GANHO          1.6f  // compensa a luz que deixou de se espalhar pela cela
// Contraluz fria só no corpo do Precursor: recorta a silhueta mesmo no escuro (não clareia o resto).
#define CONTRALUZ_CELA     0.55f
#define CONTRALUZ_CORREDOR 0.22f // mais fraca: nos corredores ele ainda precisa se esconder no escuro
// Lâmpada com defeito na cela apagada: piscada dupla, em média a cada 3 s, sem ritmo de relógio.
#define PISCADA_INTERVALO_S 3.0f  // um ciclo; a piscada cai num ponto sorteado dentro dele
#define PISCADA_VARIACAO_S  0.9f  // intervalo entre piscadas: de 2,1 a 3,9 s
#define PISCADA_PULA        0.15f // chance de um ciclo passar sem piscar
#define PISCADA_FORCA       0.7f  // fração do holofote A aceso no auge da piscada
// O shader aceita até 5 luzes: as 4 do cenário (nivel_luzes) e a lanterna (o texto do shader segue este
// valor).
#define LUZES_SHADER 5

// ---------------------------------------------------------------- lanterna (tecla F, sem bateria)
#define LANTERNA_ABERTURA_GRAUS 22.0f // meia-abertura do cone
#define LANTERNA_INTENSIDADE    3.1f
#define LANTERNA_QUEDA          0.4f // perda de força com a distância (lâmpadas = 1): menor = alcança mais longe
#define LANTERNA_COMPRIMENTO_M  0.24f // o modelo na mão é escalado para esse comprimento

// ---------------------------------------------------------------- trilhas (sons longos de assets/audio)
#define CORACAO_LONGE_M    16.0f // o Precursor solto além disso: sem coração
#define CORACAO_PERTO_M    6.0f  // aquém disso: coração rápido
#define TRILHA_FADE_S      0.4f  // tempo de um volume ir de 0 a 1 (troca suave, sem corte seco)
#define TRILHA_MENU_FADE_S 1.5f  // a trilha do menu some devagar quando a cena abre no laboratório
#define VOL_TRILHA_MENU    1.0f
#define VOL_VOZ_PRECURSOR  0.9f
#define VOL_VOZ_RADIO      0.85f // a voz do Varela por baixo da legenda
#define VOL_CORACAO        0.85f
#define VOL_PASSO_ANDAR    0.55f
#define VOL_PASSO_CORRER   0.8f

// ---------------------------------------------------------------- paleta (só use onde a raylib já foi
// incluída)
#define COR_SOMBRA    (Color){0x0c, 0x0f, 0x0c, 255}
#define COR_CONCRETO  (Color){0x2b, 0x3a, 0x33, 255}
#define COR_OLIVA     (Color){0x4a, 0x4f, 0x3a, 255}
#define COR_LAMPADA   (Color){0xd9, 0xa4, 0x41, 255}
#define COR_FOSFORO   (Color){0x7d, 0xff, 0x9a, 255}
#define COR_VIDRO     (Color){0x5a, 0xa0, 0xc8, 255}
#define COR_ALARME    (Color){0xff, 0x5a, 0x4f, 255}
#define COR_PELE      (Color){0xc9, 0xd4, 0xdc, 255}
#define COR_CONTRALUZ (Color){0x7a, 0xb0, 0xe0, 255}
#define COR_AMBAR     (Color){0xd9, 0xa4, 0x41, 255}
#define COR_LANTERNA  (Color){0xf2, 0xe8, 0xd0, 255}

// Folego no armario: expulso aos 12 s; saindo com 8 s ou mais escondido, ofega.
#define ARMARIO_TEMPO_MAX   12.0f
#define ARMARIO_TEMPO_OFEGO 8.0f

// Zerou o folego correndo: so volta a correr com esta fracao da barra.
#define FOLEGO_RETOMA_FRACAO 0.75f

#endif

// ---------------------------------------------------------------- Armário
#define ARMARIO_YAW_MAX   0.45f // limite do olhar para os lados dentro do armário
#define ARMARIO_PITCH_MAX 0.20f // limite do olhar para cima/baixo dentro do armário