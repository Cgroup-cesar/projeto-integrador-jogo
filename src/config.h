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
// Os ícones dos botões do controle na barra de teclas (botoes.c): um por botão citado nela.
#define BOTOES_TOTAL 13
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
#define PORTA_ABRE_S          1.8f  // quanto a folha leva para abrir (E na porta, nas saídas)
#define PORTA_FECHA_S         1.2f  // e para fechar (o som bate junto com a folha no batente)
#define PORTA_SOLIDA_ATE      0.35f // abaixo desta abertura a folha já fecha o vão (colisão)
#define PORTA_SOM_ALCANCE_M   16.0f // até onde se ouve a porta abrindo e fechando
#define PORTA_CRIATURA_ABRE_M 2.2f  // a criatura chegando a esta distância da porta fechada, ela abre...
#define PORTA_CRIATURA_ABRE_S 0.45f // ...com um tranco: a folha abre nesse tempo
#define CAMERA_COMPRIMENTO_M  0.65f // da placa do suporte até a frente da carcaça
#define CAMERA_ALTURA_M       2.5f  // onde a placa encosta na parede
#define CAMERA_INCLINA_GRAUS  22.0f // a lente aponta um pouco para baixo
#define MAX_CAMERAS           8     // vagas para o estado de cada câmera (cameras.c tem 7)
#define CAMERA_BRACO_M        0.16f // a câmera gira na ponta de um braço curto, fora da parede
#define CAMERA_VISAO_M        16.0f // até onde uma câmera enxerga o jogador
#define CAMERA_GIRO_MAX_GRAUS 70.0f // o quanto ela vira para cada lado, a partir de para onde aponta
#define CAMERA_VELOCIDADE     45.0f // graus por segundo no máximo: o motor é lento, dá para ouvir ele virando
#define CAMERA_FIRMEZA        3.0f  // o motor desacelera perto do alvo (passo = diferença × isto × dt)
#define CAMERA_REACAO_S       0.35f // ao avistar o jogador, demora isto para começar a virar
#define CAMERA_ATRASO_S       0.55f // a mira persegue onde o jogador esteve (constante de tempo, em s)
#define CAMERA_ESPERA_S       2.5f  // depois de perder o jogador, fica parada olhando antes de voltar
#define CAMERA_SOM_ALCANCE_M  6.0f  // até onde se ouve o motor: só chegando perto da câmera
#define CAMERA_SOM_VOLUME     0.3f  // o motor é discreto: mesmo colado nela, 30% do volume
// A volta à sala depois de religar a energia (retorno.c): uma cena sem corte, de 4,6 s.
#define RETORNO_VIRA_S         0.9f   // vira para a porta, de dentro da sala...
#define RETORNO_FECHA_S        1.5f   // ...fecha (a rodada seguinte começa quando a folha bate)...
#define RETORNO_ENTRA_S        2.2f   // ...e entra na sala, virando para a cela
#define RETORNO_ESPERA_S       1.0f   // a cena só começa este tempo depois de o painel resolver
#define RETORNO_DIANTE_PORTA_M 1.35f  // de onde ele fecha a porta (o plano da folha fica no meio da célula D)
#define RETORNO_ENTRA_M        2.4f   // quanto ele anda sala adentro depois de fechar
#define RETORNO_PUXA_PITCH     0.1f   // o olhar desce um pouco enquanto ele puxa a folha
#define RETORNO_FAIXA_S        0.45f  // as faixas pretas de cinema entram e saem nesse tempo
#define RETORNO_FAIXA_ALTURA   0.085f // cada faixa, em fração da altura da tela
// O quadro de investigação na parede sul da sala de observação (quadro.c), atrás de quem está no console.
#define QUADRO_LARGURA_M 1.95f
#define QUADRO_X         8.6f   // centro, no meio da parede sul
#define QUADRO_PAREDE_Z  13.995f // a parede sul da sala (linha 7 do mapa: z = 14)
#define QUADRO_BASE_Y    0.95f  // onde fica a borda de baixo
// O chaveiro com a chave do Arquivo (chave.c): na mesma parede, à direita do quadro para quem olha a parede,
// bem atrás de quem começa a S2 de frente para o console.
#define CHAVEIRO_X           6.3f
#define CHAVEIRO_Y           1.68f // logo acima do azulejo (BARRADO_ALTURA)
#define CHAVEIRO_LARGURA_M   0.34f
#define CHAVEIRO_ALTURA_M    0.24f
#define CHAVEIRO_FUNDO_M     0.025f
#define COR_CHAVE            (Color){0xd4, 0xa8, 0x4a, 255} // latão
#define CHAVE_AVISO_S        9.0f                           // o aviso da chave no começo da S2
#define CHAVE_AVISO_CURTO_S  3.5f                           // porta trancada, chave pega
#define PORTA_ARQUIVO_ABRE_S 1.2f                           // a folha desliza para dentro da parede
#define FITA_HORA_INICIO   (21 * 3600 + 47 * 60 + 52) // a câmera às 21:47:52, durante o canto
#define TECLAS_ESCALA_JOGO 0.62f // tamanho do B — Comandos e da lista na partida (1 = a barra do menu)
#define MIRA_RAIO_PX       1.6f  // a mira: um ponto branco pequeno (px na tela de 1280x720)
#define HUD_TEXTO_PX       16.0f // VIDRO e PLACAR, no canto superior direito
#define HUD_OBJETIVO_PX    15.0f // a missão, no canto superior esquerdo
#define DICA_LANTERNA_HZ   1.8f  // até a 1ª vez que acende, o F do canto pulsa em amarelo nesse ritmo
#define DICA_LANTERNA_COR  (Color){0xff, 0xd8, 0x4a, 255}
#define DICA_LANTERNA_GLOW 0.16f // força do brilho em volta do F no pico do pulso (camadas somadas)

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
#define VHS_UNIFORMES (VHS_PARAMETROS + 5) // + tempo, resolução, insanidade (grão), surto e piscada

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
#define SANIDADE_REGEN           2.0f  // pontos/s, contínuo e sempre igual (+50 em 25 s): ele se afastou
#define SANIDADE_RAIO_VOLTA      10.0f // metros: além disso, com ele sem se aproximar, a sanidade já volta
#define SANIDADE_ESPERA_REGEN    0.5f  // s afastando antes de subir (as pausas do passo dele são mais curtas)
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
// Sanidade zerada (o surto): a visão falha em apagões curtos e alternados e uma vinheta vermelha fecha as
// bordas (vhs.c). Some devagar quando a sanidade volta a subir.
#define SURTO_LIMIAR       0.5f  // sanidade (a da barra) até aqui conta como zerada
#define SURTO_ENTRA_S      0.5f  // quanto o surto leva para chegar ao máximo
#define SURTO_SAI_S        1.2f  // e para sumir depois que a sanidade volta
#define SURTO_FALHA_HZ     11.0f // sorteios por segundo: cada um decide se a visão falha agora
#define SURTO_FALHA_CHANCE 0.40f // fração dos sorteios que falham (nunca dois seguidos)
#define SURTO_ESCURO_MIN   0.10f // brilho no meio de uma falha (0 = preto total)...
#define SURTO_ESCURO_MAX   0.45f // ...até este
#define SURTO_VINHETA      0.9f  // força do vermelho nas bordas (continua acesa nas falhas)
#define SURTO_TINGE        0.45f // quanto a imagem inteira puxa para o vermelho
#define SURTO_NEVOA        1.0f  // névoa vermelha somada à imagem (tira o preto puro, como sangue no olho)
#define SURTO_TREMOR_PX    10.0f // quanto as linhas escorregam de lado numa falha

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
// Controle (Xbox ou qualquer um que a raylib reconheça; controle.c). O olhar é em radianos por segundo, com
// o analógico direito todo empurrado; o vertical gira mais devagar, como na maioria dos jogos de console.
#define CONTROLE_GIRO_RAD_S     2.8f
#define CONTROLE_GIRO_VERTICAL  0.65f // fração do giro horizontal
#define CONTROLE_ZONA_MORTA     0.18f // analógico: abaixo disso conta como solto (deriva do controle)
#define CONTROLE_CURVA          2.0f  // resposta do analógico do olhar (2 = ao quadrado: mira fina no meio)
#define CONTROLE_MENU_LIMIAR    0.6f  // analógico esquerdo empurrado além disso vira uma seta nos menus
#define CONTROLE_GATILHO_LIMIAR 0.5f  // LT apertado além disso (eixo de -1 a 1) corre, como o LB
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
// ---------------------------------------------------------------- animação do Precursor (rig: esqueleto.c)
// Ele é RETRAÍDO: doente, desconfiado, contido. Na dúvida, menos é mais. Ângulos em graus, distâncias em m.
// Postura (sempre)
#define ANIM_CURVA_GRAUS   7.0f  // a coluna se curva para a frente (soma-se à curvatura do modelo)
#define ANIM_OMBROS_GRAUS  9.0f  // os ombros se fecham para dentro
#define ANIM_CABECA_FRENTE 10.0f // o pescoço projeta a cabeça à frente (a cabeça compensa e olha adiante)
// Passada (o ritmo segue a distância andada: o pé plantado não desliza)
#define ANIM_PASSADA_M       1.45f  // passada completa (dois passos) andando devagar
#define ANIM_PASSADA_CORRE_M 2.2f   // passada completa na perseguição
#define ANIM_PASSO_VARIA     0.12f  // cada passo sai até 12% mais curto ou mais longo (nunca dois iguais)
#define ANIM_PE_ALTURA_M     0.10f  // quanto o pé sobe no meio do passo
#define ANIM_PE_ARRASTA      0.55f  // o pé direito sobe só isso da altura (arrasta um pouco: algo errado)
#define ANIM_PASSO_LENTO_S   0.8f   // um passo nunca demora mais que isso (termina mesmo se ele parar)
#define ANIM_SOBE_M          0.035f // o quadril sobe e desce a cada passo
#define ANIM_BALANCO_M       0.035f // o quadril vai para o lado do pé de apoio
#define ANIM_QUADRIL_GRAUS   7.0f   // o quadril gira com a perna que avança; o tronco gira ao contrário
#define ANIM_BRACO_GRAUS     6.0f   // o balanço dos braços (pesados: pouco)
#define ANIM_BRACO_MOLA      6.5f   // rad/s: quanto menor, mais os braços atrasam em relação ao tronco
#define ANIM_INCLINA_CORRE   12.0f  // a mais de inclinação na perseguição
// Parado (idle) e cabeça
#define ANIM_RESPIRA_S     4.6f   // um ciclo de respiração (varia ±15%)
#define ANIM_RESPIRA_GRAUS 1.6f   // o peito e os ombros sobem e descem
#define ANIM_PESO_M        0.035f // o peso passa de uma perna para a outra
#define ANIM_PESO_S        7.0f   // ...devagar e sem ritmo fixo
#define ANIM_OLHAR_GRAUS_S 55.0f  // a cabeça vira no máximo isso por segundo (devagar)
#define ANIM_ESTABILIZA    0.7f   // 0..1: quanto a cabeça segura o olhar enquanto o corpo balança
#define ANIM_ESPIADA_MIN_S 5.0f   // de tempos em tempos ele vira a cabeça devagar para um lado
#define ANIM_ESPIADA_MAX_S 12.0f
#define ANIM_ESPIADA_GRAUS 35.0f
// Tiques: raros e pequenos (um dedo, a cabeça inclinando, um ombro se ajustando)
#define ANIM_TIQUE_MIN_S 6.0f // intervalo entre tiques: sorteado entre MIN e MAX
#define ANIM_TIQUE_MAX_S 15.0f
#define ANIM_TIQUE_GRAUS 8.0f // amplitude típica (o dedo vai ao dobro)
// Transições (crossfade): quanto maior, mais rápido um estado entra no lugar do outro (1/s)
#define ANIM_TRANSICAO 3.0f
// Saída do bueiro: começa ANIM_BUEIRO_PRE_S antes de ele ficar ativo (sem mudar o tempo do jogo) e termina
// junto com os 1,5 s de `surgindo`.
#define ANIM_BUEIRO_PRE_S 5.0f
#define ANIM_BUEIRO_SAI_S 1.5f // o mesmo tempo de atualiza_surgimento (criatura.c)
// Saída pela tubulação da cela, no fim da R1 e da R2 (só apresentação: não muda o tempo das fases). Começa
// na falha de luz e continua nos primeiros segundos da saída seguinte, bem antes de ele surgir no duto.
#define ANIM_CANO_INICIO_S 2.2f  // depois de concluir a rodada: o mesmo instante da falha de luz (rodadas.c)
#define ANIM_CANO_VIRA_S   0.6f  // vira para a tubulação (e, lá, de volta para o vidro)
#define ANIM_CANO_ANDA_S   2.8f  // anda até a boca
#define ANIM_CANO_DESCE_S  5.0f  // desce: a saída do bueiro ao contrário, um pouco mais rápida...
#define ANIM_CANO_VISTO    0.7f  // ...e passa esta fração do tempo no que se vê do vidro (até os ombros)
#define CANO_CELA_DX       -2.2f // a boca, em relação ao lugar dele na cela (m): à esquerda de quem olha
#define CANO_CELA_DZ       -2.3f // ...e no fundo (o console tapa a direita; perto do vidro, o parapeito)
// A boca da cela é igual ao duto do corredor (nivel.c). Os marcos da cena, em segundos de cano_t:
#define ANIM_CANO_DESCE_EM (2.0f * ANIM_CANO_VIRA_S + ANIM_CANO_ANDA_S) // chegou à boca e virou para o vidro
#define ANIM_CANO_FIM      (ANIM_CANO_DESCE_EM + ANIM_CANO_DESCE_S)
// Sons da tubulação (tubulacao_som.c). A tampa sai de assets/audio/som-tampa-tubulacao.mp3 (28 s com vários
// golpes): dois trechos cortados ao carregar. Os passos (passos-tubulacao.wav) emendam sem corte.
#define TAMPA_SAI_DE_S    7.10f // sair do duto: a tampa arrasta e cai (o baque em +0,85 s: as mãos na borda)
#define TAMPA_SAI_ATE_S   9.25f
#define TAMPA_ENTRA_DE_S  17.65f // entrar na tubulação: o baque forte (+0,3 s) e a tampa se acomodando
#define TAMPA_ENTRA_ATE_S 19.75f
#define TAMPA_ALCANCE_M   26.0f
#define TAMPA_VOLUME      1.0f
#define TAMPA_PISO        0.4f // volume mínimo (0..1): longe ou atrás da parede, ainda se ouve
// Os passos por dentro dos canos: do fim da entrada na cela ao começo da saída no corredor.
#define PASSOS_TUBO_ALCANCE_M 30.0f // a travessia vai da cela ao corredor sul (~30 m)
#define PASSOS_TUBO_VOLUME    1.0f
#define PASSOS_TUBO_PISO      0.5f // volume mínimo: por cima da voz do rádio, sempre audível
#define PASSOS_TUBO_FUNDO_M   1.0f // os canos passam por baixo do chão
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
#define TEXTURA_LADO_PX         1024  // lado das texturas quadradas (piso, teto, oliva, metal, porta)
#define TEXTURA_CONCRETO_PX     2048  // o concreto é 2048 × 1024: cobre 6 m × 3 m sem repetir à vista
#define TEXTURA_SEMENTE         1985u // mesma semente → mesma imagem em toda execução
#define TEXTURA_SUJEIRA         0.8f  // 0 = limpo; 1 = manchas, escorridos e descascado no máximo
#define TEXTURA_METROS_CONCRETO 6.0f  // metros por repetição na horizontal (na vertical: PE_DIREITO)
#define TEXTURA_METROS_PISO     2.0f  // 4 × 4 placas de 0,5 m
#define TEXTURA_METROS_TETO     2.0f  // 3 × 3 placas de forro
#define TEXTURA_METROS_METAL    1.0f
#define EXPOSICAO               1.25f // multiplica a cena HDR antes do tone mapping (ACES)
// Névoa volumétrica, bloom e gradação de cor (efeitos.c; os presets de qualidade ligam e desligam)
#define NEVOA_VOLUME_DENSIDADE  0.05f    // por metro; a poeira faz variar em volta disso
#define NEVOA_VOLUME_AMBIENTE   0.004f   // a luz que a névoa espalha sozinha: o fundo do corredor clareia
#define BLOOM_CORTE             1.0f     // só o que passa disso (em HDR) brilha em volta
#define BLOOM_FORCA             0.18f
#define GRADACAO_SOMBRA         {0.78f, 1.02f, 1.0f}  // as sombras puxam para o azul-petróleo
#define GRADACAO_LUZ            {0.92f, 1.06f, 0.95f} // as luzes, para o verde-claro
#define GRADACAO_SATURACAO      0.78f
#define GRADACAO_CONTRASTE      1.06f
#define GRADACAO_ELEVACAO       {0.012f, 0.028f, 0.03f} // os escuros puxam para o azul-petróleo...
#define GRADACAO_PISO_PRETO     0.08f // ...com força total só a partir deste brilho: o preto fica preto
#define GRADACAO_VINHETA        0.85f
#define GRADACAO_GRAO           0.03f
#define GRADACAO_ABERRACAO      0.012f
#define AGUA_NO_CHAO            1.0f  // 0 = piso seco; 1 = manchas de água e o rejunte cheio
#define TEXTURA_ARQUIVO_MAX_PX  1024  // PNG maior que isso (ou fora de potência de 2) é redimensionado
#define BARRADO_ALTURA          1.5f  // azulejo do chão até aqui (acima: reboco pintado, descascando); múltiplo
                                      // de 15 cm a partir do teto: a fiada de cima não sai cortada
#define RODAPE_ALTURA           0.15f // a fiada de baixo, mais escura (uma peça de 15 cm)
#define DECALQUE_PX             256   // manchas: 256 × 256; placas: 512 × 128
#define HALO_LAMPADA_M          1.3f  // tamanho do brilho em volta das lâmpadas
#define HALO_LAMPADA_ALFA       0.35f // força do brilho (some com a névoa)

// ---------------------------------------------------------------- luz e névoa
#define LUZ_AMBIENTE       0.045f // era 0,06: o escuro pesa mais, mas a lanterna e os holofotes não mudaram
#define NEVOA_DENSIDADE    0.095f // era 0,085: o fundo do corredor some um pouco antes
#define LUZ_BIT_MESA       13    // o bit da lâmpada da mesa no mapa de alcance (as luminárias: 0..12)
#define LUMINARIAS_ACESAS  7     // quantas luminárias (as mais próximas) entram no shader de luz
#define LUMINARIAS_FALHANDO 0.3f // fração das luminárias que falham (flicker)
#define LUMINARIA_FORCA     1.6f  // a luz de cada luminária com a energia de volta
#define LUMINARIA_EMISSAO   7.0f  // o brilho do painel (HDR: estoura no bloom e no reflexo do chão)
#define LUMINARIA_LUZ_ABAIXO_M 0.12f // a luz fica logo abaixo do painel do teto
// A lâmpada pendurada da sala de observação (lampada_sala.c), no centro da célula S, em cima de quem começa.
#define LAMPADA_SALA_X          7.0f
#define LAMPADA_SALA_Z          11.0f
#define LAMPADA_SALA_ALTURA_M   0.7f  // do gancho no teto ao fundo da gaiola (o modelo é escalado por ela)
#define LAMPADA_SALA_BULBO      0.68f // altura do bulbo, em fração da lâmpada medida a partir do teto
#define LAMPADA_SALA_BULBO_RAIO 0.04f
#define LAMPADA_SALA_COR_VIDRO  (Color){0xd8, 0xe8, 0xe0, 40} // o vidro da gaiola: um tom fraco
#define LUZ_CORREDOR_GANHO 0.8f   // força das lâmpadas dos corredores (a luz fraca no jogador fica igual)
// Sem energia (nas saídas, até o painel ser resolvido): só as luzes de emergência, fracas, e quase nada de
// ambiente. Sem a lanterna, dá para ver o vulto do corredor, mas não o que tem nele.
#define SEM_ENERGIA_AMBIENTE   0.35f // multiplica LUZ_AMBIENTE
#define SEM_ENERGIA_JOGADOR    0.06f // sem energia as luminárias apagam: só a lanterna e este resto de luz
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
#define LUZES_SHADER 12 // até 9 luminárias + a luz fraca no jogador + a lanterna (+1 de folga)

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
#define PASSO_CRIATURA_ALCANCE_M 22.0f // até onde o passo dele se ouve
#define SOM_PERTO_M              4.0f  // som no espaço: daqui para perto, volume cheio
#define SOM_QUEDA_DB             28.0f // quanto cai em dB do perto até o alcance
#define SOM_CURVA                1.6f  // > 1: a queda começa suave e acelera longe (perto continua alto)
#define SOM_ATRAS_DB             3.0f  // bem atrás soa um pouco mais baixo: ajuda a saber se é na frente
#define SOM_PAREDE_DB            8.0f  // com parede no meio, abafado
#define SOM_PAN_MAX              0.85f // o mais para um lado que vai (nunca 100% num ouvido)
#define GOTA_INTERVALO_S         1.3f  // goteira: uma gota a cada ~1,3 s (o instante é sorteado no intervalo)
#define GOTA_QUEDA_S             0.6f  // da boca do cano até a poça
#define GOTA_INCHA_S             0.3f  // a gota cresce na boca antes de cair
#define GOTA_ONDA_S              0.9f  // quanto dura a onda na poça
#define GOTA_ONDA_RAIO_M         0.3f
#define GOTA_ALCANCE_M           14.0f // até onde se ouve o pingo
#define CHAO_MOLHADO_BRILHO      2.5f  // força do reflexo das luzes no chão molhado e na poça
#define PASSO_CRIATURA_TOM       0.8f  // abaixo de 1 soa pesado; mais baixo, o grave some na caixa de som

// ---------------------------------------------------------------- paleta (só use onde a raylib já foi
// incluída)
#define COR_SOMBRA    (Color){0x0c, 0x0f, 0x0c, 255}
#define COR_CONCRETO  (Color){0x2b, 0x3a, 0x33, 255}
#define COR_OLIVA     (Color){0x4a, 0x4f, 0x3a, 255}
#define COR_LAMPADA   (Color){0xd9, 0xa4, 0x41, 255}
#define COR_LUMINARIA (Color){0xb4, 0xe2, 0xd2, 255} // a luz fria das luminárias com a energia de volta
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
