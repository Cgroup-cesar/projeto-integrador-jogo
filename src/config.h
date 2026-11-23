// config.h — constantes ajustáveis do jogo. Mudou um número de balanceamento? É aqui.
// Este arquivo não inclui a raylib: as cores são macros e só viram valor onde a raylib já foi incluída.
#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------- título
// Trocar o nome do jogo no futuro exige mudar só estas duas linhas.
#define TITULO_JOGO       "PRECURSOR"
#define TITULO_PROVISORIO 0
#define MENU_SUBLIMINARES                                                                                    \
    1 // frases que piscam por ~0,1 s na tela de início (0 desliga; ex.: pedido da banca)
// Chuvisco de TV da tela de início (estilo menu de terror dos anos 2010): a cena continua visível por trás.
#define MENU_ESTATICA_ALFA    0.09f // força base do chuvisco (0 desliga; acima de ~0,3 a cena some)
#define MENU_ESTATICA_PICO    0.07f // reforço nos surtos de interferência
#define MENU_ESTATICA_GRAO    3.0f  // tamanho do grão em px (antes da escala da UI)
#define MENU_ESTATICA_QUADROS 30.0f // quantas vezes por segundo o chuvisco muda
#define MENU_FALHA_TROCA_S    0.35f // quanto dura a falha da opção logo depois de escolhida
#define MENU_BALANCO_PX       3.0f  // amplitude do balanço lento da opção escolhida (px, antes da escala)
#define MENU_FALHA_LEVE       0.4f // intensidade da falha nas opções da pausa e das perguntas (1 = a do menu)
#define TELAS_ESTATICA_ALFA   0.03f // chuvisco da pausa e das perguntas da intro: bem mínimo
#define TELAS_ESTATICA_PICO   0.02f
// A fita do avistamento na intro (fita.c): três fotos de assets/fita/ por trás do texto datilografado.
#define FITA_FOTOS         3
#define FITA_LINHA_QUEDA   2     // na 3ª linha da intro o sinal cai: falha, chuvisco e tela preta
#define FITA_TROCA_LETRA   30.0f // na 2ª linha, a foto do feixe entra nesta letra ("...desceu da nave")
#define FITA_CORTE_LETRAS  5.0f  // quanto dura (em letras) a falha do corte entre duas fotos
#define FITA_ENTRADA_S     0.35f // no começo de cada linha com foto, o sinal "pega" com uma falha curta
#define FITA_QUEDA_S       0.8f  // a queda: o sinal falha cada vez mais por este tempo...
#define FITA_ESTATICA_S    0.35f // ...vira chuvisco puro por este; depois a tela fica preta
#define FITA_PAUSA_S       2.0f  // as linhas com foto ficam mais tempo na tela (as outras, 1,1 s)
// A passagem da intro para a sala: a última frase fica, some, um instante de preto e a sala clareia.
#define INTRO_FIM_PAUSA_S    2.5f // a última frase ("Nunca traduziu nada vivo.") fica na tela
#define INTRO_TEXTO_SOME_S   0.8f // o texto some devagar...
#define INTRO_PRETO_S        0.5f // ...um instante de tela preta e silêncio...
#define INTRO_ABRE_S         2.0f // ...e a sala aparece saindo do preto
#define INTRO_RADIO_ATRASO_S 1.0f // o rádio do Varela só toca com a sala já à vista
// O computador de época sobre a mesa do console (console_modelo.c).
#define CONSOLE_TERMINAL_LARGURA_M 0.75f  // de lado a lado
#define CONSOLE_TAMPO_Y            1.05f  // altura do tampo da mesa (a base do terminal)
#define CONSOLE_TERMINAL_Z         -0.62f // em relação ao ponto do console (negativo: para o lado do vidro)
// As pilhas de caixotes das salas (caixas.c): a colisão usa estas medidas, com ou sem o modelo.
#define CAIXAS_COMPRIMENTO_M  2.5f
#define CAIXAS_ALTURA_M       1.21f
#define CAIXAS_PROFUNDIDADE_M 0.9f
#define CAIXAS_PLACA_LARGURA_M 0.62f // a placa do Exército no caixote grande (cobre o "Fragile" do modelo)
#define CAIXAS_PLACA_ALTURA_M  0.28f
// A mesa do bilhete, no fundo do Arquivo (mesa_bilhete.c).
#define MESA_ALTURA_M      0.76f  // o modelo é escalado por esta altura
#define MESA_FUNDO_M       0.84f  // de frente para trás (eixo X): encostada na parede oeste
#define MESA_COMPRIMENTO_M 1.69f
#define MESA_PAPEL_ESCALA  0.01f  // as folhas de papeis.glb estão em centímetros
#define MESA_LUZ_Y         2.1f   // a lâmpada pendurada sobre a mesa (mais baixa que as do teto)
#define MESA_LUZ_FORCA     3.6f   // o dobro de uma lâmpada de corredor: o ponto mais claro do Arquivo
#define MESA_LUZ_ALCANCE_M 14.0f  // até esta distância ela sempre acende (cobre o Arquivo e a porta)
#define MESA_COR_LUZ       (Color){0xff, 0x9a, 0x3c, 255} // âmbar quente: a única luz assim no Arquivo
// A porta de bunker da sala de observação (porta_bunker.c) e as câmeras de segurança (cameras.c).
#define PORTA_ALTURA_M        2.2f  // o batente cabe embaixo da verga das portas
#define PORTA_ABRE_S          1.8f  // quanto a folha leva para abrir quando a saída começa
#define CAMERA_COMPRIMENTO_M  0.65f // da placa do suporte até a frente da carcaça
#define CAMERA_ALTURA_M       2.5f  // onde a placa encosta na parede
#define CAMERA_INCLINA_GRAUS  22.0f // a lente aponta um pouco para baixo
#define FITA_HORA_INICIO   (21 * 3600 + 47 * 60 + 52) // a câmera às 21:47:52, durante o canto
#define TECLAS_ANDAR_S     10.0f // andando, a barra de teclas some depois disso em cada fase
#define TECLAS_ESCALA_JOGO 0.88f // tamanho da barra de teclas durante a partida (1 = original)

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
// O efeito na tela (cansaço + fita VHS, em vhs.c) tem força linear: 1 − sanidade/100 (100% → 0, 99% → 0,01,
// 20% → 0,8, 0% → 1). A criatura age por meio da sanidade, que já cai a partir de SANIDADE_RAIO.
#define SANIDADE_MAX             100.0f
#define SANIDADE_RAIO            15.0f // metros: daqui para dentro a criatura abala (e o efeito aparece)
#define SANIDADE_PERTO           2.5f  // metros: daqui para dentro, queda no máximo
#define SANIDADE_QUEDA_MIN       1.5f // pontos/s base na borda do raio (MIN→MAX com p², p = proximidade 0..1)
#define SANIDADE_QUEDA_MAX       12.0f // pontos/s base colado nela
#define SANIDADE_ACELERA_LONGE   1.5f  // a base é multiplicada por 1,5× na borda do raio (2,25 pontos/s)...
#define SANIDADE_ACELERA_PERTO   2.0f  // ...até 2× colado nela (24 pontos/s: 100 → 0 em ~4 s)
#define SANIDADE_FATOR_ARMARIO   0.5f  // escondido no armário a queda cai pela metade
#define SANIDADE_REGEN           0.8f  // pontos/s enquanto joga: +50 em ~1 min
#define SANIDADE_ESPERA_REGEN    3.0f  // s sem a criatura no raio antes de voltar a subir (corredores)
#define SANIDADE_BONUS_RODADA    35.0f // decifrou e traduziu (EV_RODADA_CONCLUIDA)
#define SANIDADE_BONUS_SAIDA     40.0f // religou a energia e voltou à sala (EV_SAIDA_CONCLUIDA)
#define SANIDADE_BONUS_CONECTIVO 10.0f // cada conectivo decifrado pela primeira vez
#define SANIDADE_PISO_RECOMECO   60.0f // pego ou vidro rompido: recomeça com pelo menos isso (sem espiral)
// Os tetos abaixo valem com força 1 (sanidade 0) e são o limite jogável.
#define SANIDADE_CURVA            0.85f // só a percepção: o shader usa força^0,85 (50% → 0,55, 20% → 0,83)
#define SANIDADE_DESFOQUE_PX      1.8f  // cansaço: raio do desfoque nas bordas (px do render target)
#define SANIDADE_TUNEL            0.35f // cansaço: quanto a vinheta fecha a mais (visão de túnel)
#define SANIDADE_DESSATURA        0.25f // cansaço: perda de saturação
#define SANIDADE_PULSO_A_S        5.3f  // cansaço: períodos das duas ondas lentas que embaçam e desembaçam
#define SANIDADE_PULSO_B_S        3.1f  //          (somadas para não ficar regular)
#define SANIDADE_RUIDO            0.45f // fita: ruído de luminância, proporcional ao brilho (±45% nas estrias)
#define SANIDADE_RUIDO_PRETO      0.035f // fita: piso aditivo no preto (±1,75% de brilho): mantém as silhuetas
#define SANIDADE_RUIDO_LARGURA_PX 4.0f   // fita: o ruído é esticado na horizontal (1 linha × isso)
#define SANIDADE_CROMA            0.03f  // fita: ruído de cor, só nas sombras
#define SANIDADE_DROPOUT          1.0f   // fita: frequência dos riscos claros (1 ≈ 2 por quadro com força 1)
#define SANIDADE_TRACKING_PX      5.0f   // fita: tremor horizontal das linhas, só abaixo de ~25% de sanidade
#define SANIDADE_BARRA_SOBE       3.0f   // a barra enche suave com os bônus (1/s)
#define SANIDADE_ECO_VEL          12.0f  // pontos/s: o pedaço perdido (mais escuro) alcança a barra
#define SANIDADE_ALERTA           30.0f  // abaixo disso a barra fica vermelha e pulsa

// ---------------------------------------------------------------- debug e modos
#define TECLAS_DEBUG                                                                                         \
    1 // F1 preset do VHS · F2 pular fase · F3 estado da criatura · F5 reiniciar · F9 resposta · F12 captura
#define FEIRA_INATIVIDADE_S 90.0f // no modo feira (--feira), volta ao menu após esse tempo sem nenhuma tecla
#define FIM_VOLTA_MENU_S    20.0f // a tela de fim volta sozinha ao menu (conta desde a última tecla)
#define FIM_RANKING_S       15.0f // a tela do ranking, depois do desempenho, volta sozinha ao menu
#define RANKING_MAX         100   // o arquivo guarda as 100 melhores partidas; a tela mostra RANKING_TELA
#define RANKING_TELA        8
#define RANKING_ARQUIVO     "ranking.txt"    // na pasta do jogo (fora do git: .gitignore)
#define AVALIACOES_ARQUIVO  "avaliacoes.csv" // as estrelas de quem jogou, sem nome, para o relatório de IHC
#define AVALIACAO_PERGUNTAS 3
#define AVALIACAO_MAX       5     // estrelas por pergunta
#define FIM_PLACAR_FOLGA_S  0.4f  // o placar ignora o ENTER nesse começo (o do texto não pode fechá-lo)
#define FIM_RUIM_TEXTO_S    8.2f  // final ruim: quando o texto e o "por quê" aparecem (depois do susto)
#define FIM_BOM_TEXTO_S     13.0f // final bom: quando o texto e o "por quê" aparecem (depois das falas)
#define FIM_LEITURA_MAX_S   50.0f // o texto espera a leitura: ENTER ou, no máximo, isso, e vai ao placar
// O rádio falso (o Precursor imitando o Varela, a partir da S2)
#define RADIO_FALSO_TOM    0.78f // tom da voz do rádio (1 = normal; menor = mais grave)
#define RADIO_FALSO_CANTO  0.35f // o canto do Precursor por baixo da fala, em fração do volume da voz dele
#define FALHA_INTERVALO_S  1.4f  // a legenda falha mais ou menos uma vez a cada intervalo...
#define FALHA_DURACAO_S    0.18f // ...por este tempo
#define FALHA_LETRAS       0.35f // fração das letras trocadas por símbolos enquanto falha
#define CLARAO_RESPOSTA_S  1.2f  // o VERDADE/FALSO grande no terminal depois de cada resposta dele
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
#define CRIATURA_LANTERNA_ALCANCE_M 16.0f // até onde a lanterna, apontada para ele, começa a falhar
#define CRIATURA_OLHO_PX            1.0f  // no menu: raio, em pixels da cena, dos dois pontinhos dos olhos
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
#define PONTOS_PAINEL_POUCOS_MOV                                                                             \
    50                           // painel resolvido com até MOVIMENTOS_BONUS_CHEIO trocas (metade até _MEIO)
#define MOVIMENTOS_BONUS_CHEIO 4 // os dois painéis se resolvem com 2 trocas
#define MOVIMENTOS_BONUS_MEIO  8
#define TEMPO_ALVO_PARTIDA_S   480.0f // bônus de tempo: meio ponto por segundo abaixo disso
#define MAX_FICHAS_FRASE       7

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
#define LUZ_AMBIENTE       0.045f // era 0,06: o escuro pesa mais, mas a lanterna e os holofotes não mudaram
#define NEVOA_DENSIDADE    0.095f // era 0,085: o fundo do corredor some um pouco antes
#define LUZ_CORREDOR_GANHO 0.8f   // força das lâmpadas dos corredores (a luz fraca no jogador fica igual)
// Holofotes da cela (luzes A e B): ficam à frente e acima do Precursor, apontados para o peito dele.
#define HOLOFOTE_ABERTURA_GRAUS 30.0f // meia-abertura do cone; fora dele sobram 15% (a cela não fica preta)
#define HOLOFOTE_GANHO          1.6f  // compensa a luz que deixou de se espalhar pela cela
// Contraluz fria só no corpo do Precursor: recorta a silhueta mesmo no escuro (não clareia o resto).
#define CONTRALUZ_CELA     0.0f  // na cela, nada de contorno: ele fica no escuro (névoa e luzes o revelam)
#define CONTRALUZ_CORREDOR 0.22f // mais fraca: nos corredores ele ainda precisa se esconder no escuro
// Lâmpada com defeito na cela apagada: piscada dupla, em média a cada 3 s, sem ritmo de relógio.
#define PISCADA_INTERVALO_S 3.0f  // um ciclo; a piscada cai num ponto sorteado dentro dele
#define PISCADA_VARIACAO_S  0.9f  // intervalo entre piscadas: de 2,1 a 3,9 s
#define PISCADA_PULA        0.15f // chance de um ciclo passar sem piscar
#define PISCADA_FORCA       0.7f  // fração do holofote A aceso no auge da piscada
// O shader aceita até 5 luzes: as 4 do cenário (nivel_luzes) e a lanterna (o texto do shader segue este
// valor).
// Névoa só dentro da cela (a caixa vem do mapa): densa, mas deixa ver o Precursor no centro; no fundo,
// ela o engole. As luzes (flash, holofotes, lanterna) clareiam a própria névoa.
#define NEVOA_CELA_DENSIDADE        0.55f // por metro de cela atravessado pelo olhar
#define NEVOA_CELA_ESPALHA          0.3f  // quanto das luzes da cela (flash, holofotes) a névoa devolve
#define NEVOA_CELA_ESPALHA_LANTERNA 0.05f // e da lanterna: pouco, senão a névoa vira parede branca
#define NEVOA_CELA_COR_R 0.045f // a cor da névoa sem luz: só um pouco acima do preto, para a silhueta
#define NEVOA_CELA_COR_G 0.050f // escura dele aparecer contra ela
#define NEVOA_CELA_COR_B 0.055f
#define LUZES_SHADER 5

// ---------------------------------------------------------------- lanterna (tecla F, sem bateria)
#define LANTERNA_ABERTURA_GRAUS 22.0f // meia-abertura do cone
#define LANTERNA_INTENSIDADE    3.1f
#define LANTERNA_QUEDA          0.4f // perda de força com a distância (lâmpadas = 1): menor = alcança mais longe
#define LANTERNA_FALHA_HZ       20.0f // apontada para o Precursor, ela falha: um sorteio a cada 50 ms
#define LANTERNA_FALHA_CHANCE   0.4f  // chance de a luz cair em cada sorteio
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
// Telas de tarefa (analisador, painéis, manual): o monitor de fósforo (crt.c).
#define COR_CRT_FUNDO    (Color){0x07, 0x11, 0x0f, 250} // quase opaco: a sala 3D não atrapalha a leitura
#define COR_CRT_LINHA    (Color){0x1c, 0x4a, 0x36, 255} // borda das caixas
#define COR_CRT_GRADE    (Color){0x0f, 0x2a, 0x1e, 255} // linhas entre as linhas da tabela
#define COR_CRT_TEXTO    (Color){0xb9, 0xf3, 0xcf, 255}
#define COR_CRT_APAGADO  (Color){0x4f, 0x8a, 0x69, 255} // legendas, opções não escolhidas
#define COR_AMBAR_ESCURO (Color){0x5a, 0x43, 0x18, 255} // o anel em volta da lâmpada acesa
#define CRT_VARREDURA_S  7.0f        // quanto a faixa clara leva para descer a tela do analisador
#define TERMINAL_TREMIDA_ERRO_S 1.5f // aviso_t acima disto: a opção errada treme (o aviso dura 1,8 s)

// Folego no armario: expulso aos 12 s; saindo com 8 s ou mais escondido, ofega.
// Segurar a respiração no armário: a linha vermelha vai e volta e a pessoa aperta ESPAÇO sobre a faixa
// branca.
#define RESP_PERIODO_S    1.8f  // tempo da linha atravessar a barra (na 1ª tentativa; cada erro acelera)
#define RESP_ACELERA      0.78f // a cada erro, o período é multiplicado por isso (mais rápido)
#define RESP_ZONA         0.16f // largura da faixa branca (fração da barra)
#define RESP_PRAZO_S      3.6f  // quanto o Precursor fica parado na porta esperando um som
#define RESP_CHANCES      3     // no 3º erro ele abre o armário (jumpscare)
#define RESP_ACERTOS      3     // acertos para ele desistir e ir embora
#define RESP_LONGE_M      7.0f  // "ele foi embora" só quando ele está pelo menos tão longe
#define RESP_AFASTA_MAX_S 6.0f  // se ainda não se afastou (preso num canto), é levado ao ponto mais longe
#define RESP_FOI_S        1.8f  // quanto o "Ele foi embora..." fica na tela (o E já funciona)
#define RESP_CHEGA_M      3.0f  // vindo atrás de quem se escondeu, a 3 m do armário ele para na porta
#define RESP_DIST_PORTA                                                                                      \
    2.6f // até onde ele para, a partir do armário: com olhos a 1,55 m e 70° de visão, a
         // partir de ~2,3 m o corpo inteiro (2,6 m) cabe na tela

// Zerou o folego correndo: so volta a correr com esta fracao da barra.
#define FOLEGO_RETOMA_FRACAO 0.75f

#endif

// ---------------------------------------------------------------- Armário
