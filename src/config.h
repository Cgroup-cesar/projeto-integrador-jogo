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
// Cada preset é uma linha; a ordem das colunas é a de ParametroVhs em vhs.c. O preset 0 é "desligado".
// Limites para não atrapalhar o jogo: ruído ≤ 0,08, cintilação ≤ 0,05 e curvatura ≤ 0,15 (cantos só).
// Colunas: ruído · aberração cromática (px) · scanlines · vinheta · wobble (px) · sangramento de cor (px) ·
//          tracking (0..1) · saturação (1 = normal) · tom (+ quente, − verde) · curvatura · cintilação ·
//          rolagem (faixa de ruído que sobe pela tela, 0..1) · brilho (1 = normal; ≥ 0,85)
#define VHS_PRESET_PADRAO 1 // com qual preset o jogo abre (F1 alterna entre eles)
#define VHS_PARAMETROS    13
// clang-format off
#define VHS_PRESETS {                                                                                        \
    {0.00f, 0.0f, 0.00f, 0.00f, 0.0f, 0.0f, 0.00f, 1.00f,  0.00f, 0.00f, 0.00f, 0.00f, 1.00f}, /* desligado (sem shader) */ \
    {0.04f, 1.5f, 0.06f, 0.35f, 2.5f, 0.0f, 0.00f, 1.00f,  0.00f, 0.00f, 0.00f, 0.00f, 1.00f}, /* original (v0) */         \
    {0.05f, 1.2f, 0.05f, 0.40f, 3.0f, 1.2f, 0.60f, 0.85f,  0.60f, 0.00f, 0.02f, 0.00f, 0.88f}, /* fita caseira */ \
    {0.08f, 2.5f, 0.07f, 0.45f, 5.0f, 3.5f, 1.00f, 0.65f,  0.25f, 0.00f, 0.05f, 1.00f, 1.00f}, /* fita gasta */           \
    {0.03f, 1.0f, 0.14f, 0.50f, 1.5f, 1.0f, 0.00f, 0.90f,  0.00f, 0.12f, 0.03f, 0.00f, 1.00f}, /* monitor CRT */          \
    {0.06f, 0.8f, 0.10f, 0.45f, 2.0f, 1.5f, 0.50f, 0.15f, -0.60f, 0.06f, 0.02f, 0.50f, 1.00f}, /* câmera de vigilância */ \
}
// clang-format on
#define VHS_UNIFORMES (VHS_PARAMETROS + 3) // + tempo, resolução e insanidade (grão)

// ---------------------------------------------------------------- sanidade (barra no topo e grão na tela)
// Cai com a criatura perto (só nos corredores), volta devagar enquanto se joga e volta muito ao concluir
// tarefas. Não mata nem muda regras. A barra fica fixa no topo (SANIDADE e a porcentagem).
// O grão tem duas fontes, combinadas com teto: a PROXIMIDADE (na hora: um pouco já a SANIDADE_RAIO, cresce
// até SANIDADE_PERTO) e a SANIDADE baixa (memória: fica depois da fuga e some quando ela volta).
#define SANIDADE_MAX             100.0f
#define SANIDADE_RAIO            10.0f // metros: daqui para dentro a criatura abala (e o grão aparece)
#define SANIDADE_PERTO           2.5f  // metros: daqui para dentro, queda e grão no máximo
#define SANIDADE_QUEDA_MIN       1.5f  // pontos/s na borda do raio (já se vê a barra descer)
#define SANIDADE_QUEDA_MAX       12.0f // pontos/s colado nela (MIN→MAX com p², p = proximidade 0..1)
#define SANIDADE_FATOR_ARMARIO   0.5f  // escondido no armário a queda cai pela metade
#define SANIDADE_REGEN           0.8f  // pontos/s enquanto joga: +50 em ~1 min
#define SANIDADE_ESPERA_REGEN    3.0f  // s sem a criatura no raio antes de voltar a subir (corredores)
#define SANIDADE_BONUS_RODADA    35.0f // decifrou e traduziu (EV_RODADA_CONCLUIDA)
#define SANIDADE_BONUS_SAIDA     40.0f // religou a energia e voltou à sala (EV_SAIDA_CONCLUIDA)
#define SANIDADE_BONUS_CONECTIVO 10.0f // cada conectivo decifrado pela primeira vez
#define SANIDADE_PISO_RECOMECO   60.0f // pego ou vidro rompido: recomeça com pelo menos isso (sem espiral)
#define SANIDADE_GRAO_INICIO     85.0f // sanidade abaixo disso já deixa grão (a memória do susto)
#define SANIDADE_GRAO_BORDA      0.35f // 0..1: intensidade do grão ao cruzar SANIDADE_RAIO (já se nota)
#define SANIDADE_GRAO_MAX        0.30f // amplitude do grão na intensidade 1: o teto jogável (ver vhs.c)
#define SANIDADE_GRAO_SOBE       4.0f  // quão rápido o grão cresce (1/s): acompanha a aproximação
#define SANIDADE_GRAO_DESCE      1.2f  // quão rápido ele some (1/s): nunca some de repente
#define SANIDADE_BARRA_SOBE      3.0f  // a barra enche suave com os bônus (1/s)
#define SANIDADE_ECO_VEL         12.0f // pontos/s: o pedaço perdido (mais escuro) alcança a barra
#define SANIDADE_ALERTA          30.0f // abaixo disso a barra fica vermelha e pulsa

// ---------------------------------------------------------------- debug e modos
#define TECLAS_DEBUG                                                                                         \
    1 // F1 preset do VHS · F2 pular fase · F3 estado da criatura · F5 reiniciar · F9 resposta · F12 captura
#define FEIRA_INATIVIDADE_S 90.0f // no modo feira (--feira), volta ao menu após esse tempo sem nenhuma tecla
#define FIM_VOLTA_MENU_S    20.0f // a tela de fim volta sozinha ao menu
#define SMOKE_LIMITE_FASE_S                                                                                  \
    180.0f // no --smoke, uma fase que demora mais que isso (tempo simulado) é travamento

// ---------------------------------------------------------------- nome do(a) jogador(a) (só em memória)
// O limite é em letras (codepoints); o buffer é em bytes: em UTF-8 cada letra ocupa até 4 bytes, mais o '\0'.
#define NOME_MAX_CHARS         16
#define NOME_BYTES_POR_LETRA   4
#define NOME_CAPACIDADE        (NOME_MAX_CHARS * NOME_BYTES_POR_LETRA + 1)
#define NOME_CURSOR_PISCA_S    0.5f // meio período do cursor piscante
#define ENTRADA_MAX_CARACTERES 16   // caracteres digitados guardados por quadro (fila do GetCharPressed)

// ---------------------------------------------------------------- mundo
#define TAMANHO_CELULA 2.0f // 1 célula do mapa = 2 m
#define PE_DIREITO     3.0f // altura das paredes
#define ALTURA_OLHOS   1.6f

// ---------------------------------------------------------------- jogador
#define JOGADOR_RAIO            0.30f
#define JOGADOR_VEL_ANDAR       2.5f
#define JOGADOR_VEL_CORRER      4.6f // +10% (era 4,2)
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
#define MAX_RUIDOS      32
#define MAX_PARTICULAS  256
#define MAX_RACHADURAS  24
#define MAX_EVENTOS     96
#define MAX_INTERATIVOS 24
#define MAX_PAREDES     256
#define MAX_LAMPADAS    24
#define MAX_LEGENDAS    8
#define MAX_NOS_LOGICA  64
#define MAX_CAMINHO     128

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
#define LUZ_AMBIENTE       0.045f // era 0,06: o escuro pesa mais, mas a lanterna e os holofotes não mudaram
#define NEVOA_DENSIDADE    0.095f // era 0,085: o fundo do corredor some um pouco antes
#define LUZ_CORREDOR_GANHO 0.8f   // força das lâmpadas dos corredores (a luz fraca no jogador fica igual)
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
#define LANTERNA_QUEDA 0.4f // perda de força com a distância (lâmpadas = 1): menor = alcança mais longe
#define LANTERNA_COMPRIMENTO_M 0.24f // o modelo na mão é escalado para esse comprimento

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
#define ARMARIO_YAW_MAX   0.45f  // limite do olhar para os lados dentro do armário
#define ARMARIO_PITCH_MAX 0.20f  // limite do olhar para cima/baixo dentro do armário