# Decisões de implementação

Registro das escolhas feitas onde a especificação deixava espaço ou onde o ambiente pediu um ajuste. Cada item diz **o que** foi decidido e **por quê**. A especificação (`docs/ESPECIFICACAO_PROJETO_PRECURSOR.md`) continua sendo a fonte da verdade.

## Build e ambiente

1. **macOS também é suportado.** A especificação pede Windows e Linux; a v0 foi gerada num Mac (Apple Silicon, raylib 6.2 do Homebrew). O `Makefile` detecta `Darwin` com `uname -s` e usa `pkg-config raylib`; com `RAYLIB_PATH` (raylib estática), linka os frameworks `OpenGL`, `Cocoa`, `IOKit` e `CoreVideo`. O CMake não precisa de nada especial.
2. **raylib 6.2 nesta máquina.** O código usa só funções presentes na 5.5, 6.0 e 6.2 (por exemplo `IsShaderValid`, `IsFontValid`, `IsSoundValid`, que existem desde a 5.5).
3. **`raylib.h` sempre antes de `raymath.h`.** A raylib declara `Vector2`/`Vector3` sem proteção, e o `raymath.h` só os declara se a raylib ainda não o fez. Por isso todo módulo impuro inclui `estado.h` (que começa por `raylib.h`) antes de qualquer outro cabeçalho do projeto.
4. **`RAYMATH_STATIC_INLINE`** é definido no `Makefile` e no CMake. Assim as funções do `raymath.h` são `static inline` e os testes dos módulos puros não precisam linkar a raylib.
5. **Avisos da raylib separados dos nossos.** O caminho de include da raylib entra com `-isystem`; o `-Wall -Wextra` vale para o código do projeto.
6. **Saídas do build:** objetos em `obj/`, executáveis em `bin/` (o `make run` e o `make smoke` rodam a partir da raiz, onde fica `assets/`). Os assets também são procurados ao lado do executável.
7. **CMake procura a raylib instalada primeiro** (`find_package(raylib 5.5)`) e só baixa a 6.0 com `FetchContent` se não achar, ou se `-DPRECURSOR_BAIXAR_RAYLIB=ON`.
8. **`--captura <arquivo.png>`** salva um quadro depois de 0,8 s. Serve para gerar a imagem do README sem ninguém jogando.

## Núcleo puro

9. **Pool da lógica dentro de uma struct (`PoolLogico`)**, passado por ponteiro. É estático (vetor de tamanho fixo, sem `malloc`) e não usa variável global, então as funções continuam puras.
10. **Os painéis usam a mesma semântica do motor de lógica.** OU-EXCLUSIVO é `¬(A ↔ B)` e NÃO-E é `¬(A ∧ B)`, ambos calculados por `logica_aplica`. O test_logica confere as duas soluções do Painel 2 e a solução única do Painel 1 avaliando o circuito.
11. **Interativos filtrados na entrada de cada fase.** A coleção é recriada a partir do mapa ao entrar numa fase e o filter (compactação) deixa só os ativos nela (`regras_interativo_ativo`). Assim o filter faz trabalho real sem apagar nada que a fase precise.
12. **Danos das rachaduras somam 1 por erro.** Cada erro cria de 1 a 3 rachaduras e divide 1 vida entre elas; o fold soma os danos e `regras_integridade_vidro` devolve as vidas.
13. **Placar por fase:** `regras_placar_da_fase` copia os eventos para um vetor local, filtra por compactação e reaproveita o fold de `regras_placar`.

## Fatia vertical

14. **Armários dos corredores encostados na parede.** A legenda diz que o `L` "ocupa a célula", mas vários `L` ficam no meio do loop de corredores (por exemplo, `(12,5)` e `(8,8)`). Se ocupassem a célula inteira, o loop ficaria bloqueado. O armário fica encostado na primeira parede vizinha, com 0,75 m de profundidade; o corredor continua passável e o A* trata a célula como piso.
15. **Entrada num struct (`Entrada`).** O `main.c` lê teclado e mouse uma vez por quadro; os módulos só leem `e->in`. Isso deixa o loop no formato entrada → atualiza → desenha e permite que o piloto do `--smoke` jogue com as mesmas teclas do jogador.
16. **Pedidos de troca de fase em `e->evento`.** O módulo de cada fase só pede (por exemplo, `EV_RODADA_CONCLUIDA`); no fim do `jogo_atualiza`, `jogo_aplica_evento` consulta `regras_proxima_fase` (pura) e prepara a fase nova.
17. **O `--smoke` joga de verdade.** O piloto usa as teclas do terminal, do painel e do compositor; só nas saídas ele se teletransporta até o painel e de volta à sala. A cada troca de fase confere a sequência contra `regras_roteiro_partida`, o mesmo roteiro do test_fluxo, e na primeira saída anda até o vidro para conferir a colisão. Ele roda com passo fixo de 0,05 s e sem limite de FPS (velocidade acelerada).
18. **Luzes por quadro.** O shader aceita 4 luzes. Nas fases de sala: a lâmpada da sala, as luzes A e B da cela e o brilho do console. Nas saídas: as 3 lâmpadas mais próximas do jogador (vermelhas e fracas até a energia voltar) e uma luz bem fraca no próprio jogador.
19. **A porta da sala trancada vira uma parede** (`PAREDE_PORTA`) nas fases de sala e some nas saídas.
20. **Ruídos só nas saídas.** Passos, corrida, portas e painéis só entram na coleção de ruídos quando a criatura pode estar nos corredores.
21. **`--fase <NOME>`** (debug) começa direto numa fase, por exemplo `bin/precursor --fase S2`.
22. **Sem monitor, sem travar.** Se a janela não abrir, o jogo avisa e sai com código 1.

## Rodadas

23. **Léxico no terminal.** Para a tradução ter solução, o analisador mostra embaixo de cada glifo o que ele significa (NÓS, PERIGO, VOCÊ, FALAR, PORTA, ABRIR). O conectivo só aparece com nome depois de decifrado; antes, "?".
24. **Ordem das opções de tradução embaralhada.** A especificação lista a certa primeiro; no jogo ela fica na 2ª posição na R1, na 3ª na R2 e na 1ª na R3, para não ser sempre a primeira.
25. **Só combinações novas fazem ele responder**, como pede a especificação. Ao entrar no terminal, a combinação atual (luzes apagadas) já conta como nova.
26. **Placar "rodada sem erro"** considera todos os erros da rodada, inclusive os de tentativas que terminaram com o vidro rompido.
27. **`--captura-fase <NOME>` com `--captura-em <s>`** fotografa um momento de uma fase (o tempo conta a partir da entrada na fase); com `--smoke`, dá para registrar qualquer etapa sem ninguém jogar.

## Saídas

28. **Painel 2 sem chaves de entrada.** A especificação diz que o painel mostra "as 4 linhas"; por isso as entradas A e B não são chaves: a tabela mostra as 4 linhas de uma vez e os LEDs das portas percorrem as linhas (uma a cada 0,9 s), sempre avaliando o circuito.
29. **Painel 2 começa com (OU, E)**, que não resolve, para o jogador ter que mexer.
30. **Voltar à sala é andando.** Depois do painel, a energia volta e a criatura some; a saída só termina quando o jogador entra de novo na sala de observação (o Varela pede: "Volte para a sala").
31. **Apagões nas saídas:** enquanto a energia não volta, há uma chance de ~4% por segundo de um apagão de 0,35 a 0,65 s (as lâmpadas caem a 8%).

## O Precursor

32. **Pontos de patrulha:** os cantos do loop de corredores `(8,5)`, `(21,5)`, `(21,11)`, `(8,11)`, mais o gerador `(19,2)` e o Arquivo `(3,11)`, em ciclo.
33. **INVESTIGA termina em PROCURA.** Ao chegar no ruído sem achar ninguém, ele procura em volta por 6 s (a mesma regra da PROCURA depois de perder o jogador de vista) e volta a patrulhar.
34. **Perseguição direta só com linha de visão livre.** Quando ele vê o jogador, anda em linha reta até ele (a visão já garantiu que não há parede no meio); sem ver, segue o caminho do A*.
35. **Móveis bloqueiam a visão** (o raycast usa a coleção de paredes, que inclui armários, console e painéis): dá para se esconder atrás de um armário, não só dentro.
36. **Checkpoint = início da saída atual.** Ser pego recomeça a saída com a porta abrindo e a criatura de novo no duto (15 s); o diário lido continua valendo.
37. **Smoke determinístico.** Com `--smoke`, a semente do sorteio é fixa (1985); o jogo normal continua aleatório.

## Final

38. **Prévia em notação lógica.** Enquanto monta a frase, o jogador vê a mesma frase em P, Q, ¬, ∧, ∨, → (legenda P = VOCÊ FALAR, Q = PORTA ABRIR). Isso liga a resposta final ao conteúdo de Lógica para Computação sem entregar a solução.
39. **Bônus de tempo nos dois finais.** O bônus (meio ponto por segundo abaixo de 8 min) entra no placar do final, vitória ou derrota; o +200 só no final bom.
40. **ENTER acelera as cinemáticas** (pula 3 s por toque), para a fila da feira andar.
41. **"OBRIGADO, PROFESSOR."** é desenhado na posição da tela correspondente ao vidro, sobre a cabeça dele (`GetWorldToScreen`), em vez de glifos 3D: fica legível com ou sem a fonte externa.

## Áudio e atmosfera

42. **Nomes dos sons substituíveis:** `zumbido_lampada`, `voz_precursor`, `verdadeiro`, `falso`, `bip_interfone`, `maquina_escrever`, `interruptor`, `passo`, `passo_criatura`, `vidro_estalo`, `estrondo`, `alarme`, `respiracao`, `ambiente_sala`, `ambiente_saida`, `papel`, `energia`, `distorcao` (todos `.ogg`, em `assets/audio/`).
43. **Smoke calado.** O `--smoke` zera o volume; `--mudo` faz o mesmo no jogo normal.
44. **Paredes com rodapé e faixa de tinta.** Em vez de textura (nenhum asset é obrigatório), duas faixas finas (rodapé escuro e tinta oliva a 1,1 m) dão escala e leitura no escuro.

## Polimento e auditoria

45. **Um commit por marco na v0.** A especificação manda "um commit por marco"; RNF6 pede commits pequenos. Na v0, gerada de uma vez, valeu a primeira regra (10 commits, um por marco, cada um verificado). Daqui em diante vale o fluxo do `CLAUDE.md`: branch por card, commits pequenos `tipo: descrição`, PR revisado.
46. **FILTER-eventos numa cópia.** `regras_placar_da_fase` compacta uma cópia local dos eventos (não o vetor do `Estado`), para continuar pura: o placar por fase não pode apagar os eventos da partida.
47. **Erros somados por fase.** Cada erro vale −25, mas os erros consecutivos da mesma fase viram um evento só no placar. O fold dá o mesmo total e o vetor nunca enche numa partida longa.
48. **Folds recalculados a cada quadro.** `e->vidas` e `e->placar` são caches; recalcular os dois folds por quadro (dezenas de itens) é barato e evita placar desatualizado.
49. **Terminal × rodada.** `terminal_atualiza` cuida da navegação (setas e Esc); `rodadas_atualiza` decide o que o ENTER faz. A resposta final tem a navegação própria no compositor.
50. **Sorteios por segundo.** Tudo que é aleatório no tempo (fagulhas, poeira, apagões) usa `col_sorteia_por_segundo(taxa, dt)`, para a frequência não mudar com o FPS.

## README

51. **README gerado a partir das âncoras.** Os números de linha e os trechos de código do README saem de um `grep` das âncoras `[REQ:…]` e são copiados do arquivo, nunca digitados. Se o código mudar, o `make readme-check` acusa o link ou o trecho desatualizado.
52. **Âncoras também nos testes.** Cada função de teste que prova uma função pura leva a mesma âncora `[REQ:PURA-<função>]`, para a tabela "Funções puras" apontar a linha exata do teste.
53. **Só o macOS foi testado.** A v0 foi compilada e jogada no macOS (raylib 6.2). O `Makefile` e o CMake têm os caminhos de Windows e Linux pedidos pela especificação, mas o README diz com franqueza que eles ainda precisam ser testados pela equipe.

## Precursor: modelo, luz e lâmpada com defeito

54. **Modelo de terceiros com licença CC BY.** O Precursor usa o "Gaunt Horror Creature", de Purple.Point (Sketchfab, CC BY 4.0), creditado no README. Foi escolhida a versão GLB com texturas de 1k: a cena é renderizada a 60% da janela, com névoa e VHS, então texturas maiores não fariam diferença na tela.
55. **Texturas convertidas para PNG.** A raylib instalada pelo Homebrew vem sem suporte a JPEG, e a cor base do modelo era JPEG (o modelo aparecia cinza e liso). As imagens embutidas no `.glb` foram convertidas para PNG; a geometria não mudou.
56. **Postura procedural no lugar da animação.** O modelo não tem rig nem animação. `regras_postura_criatura` dá a corcunda (3° parado a 18° perseguindo), o balanço da passada (±5°, uma passada a cada 1,6 m andados) e a respiração (só parado), aplicados no desenho com o pivô nos pés. Vale também para o corpo procedural.
57. **Holofotes da cela na frente do Precursor.** As luzes A e B ficavam atrás dele (z = 4, ele em z = 5) e só iluminavam as costas. Agora ficam entre ele e o vidro, rente ao teto, como cones de 30° apontados para o peito (fora do cone sobram 15%). Ele também ganhou uma contraluz fria só no corpo (0,55 na cela, 0,22 nos corredores) para a silhueta aparecer no escuro sem clarear o resto do jogo.
58. **Lâmpada com defeito.** Nas rodadas, com a cela toda apagada, o holofote A dá uma piscada dupla a cada ~3 s (2,1 a 3,9 s, 15% de chance de pular), revelando o Precursor por um instante. Acendendo A ou B pelo console, a luz fica firme. O sorteio é um hash do ciclo, não `rand()`, para a função ser pura e testável.
59. **Lanterna sem bateria, desde o início.** O jogador já começa com ela, e o F liga e desliga. A primeira versão tinha bateria (gastava acesa, recarregava apagada e nunca apagava de vez), mas a equipe preferiu deixar infinita para a partida curta da feira. O Precursor não reage à luz. Ela apaga sozinha ao entrar no armário. Modelo: "Lowpoly Flashlight", de EnrycoDosSantos (CC BY 4.0).
60. **Shader com 5 luzes e queda por luz.** As 4 luzes do cenário continuam iguais, e a lanterna é a 5ª, em cone. Cada luz ganhou uma "queda" (quanto enfraquece com a distância): 1 nas lâmpadas e 0,4 na lanterna, para o facho chegar ao fundo do corredor sem estourar de perto. O número de luzes do shader sai de `LUZES_SHADER`, no `config.h`, e entra no texto GLSL por macro.
61. **Nada de enviar o lote dentro de `rlPushMatrix`.** Ao enviar a geometria imediata (paredes, `DrawCube`), a raylib manda como `matModel` a transformação atual. A contraluz do Precursor era ligada dentro do bloco dele, e as paredes passavam a ser iluminadas na posição dele (na S1, a sala ficava preta assim que ele saía do duto; na cela, mais escura do que devia). Agora ela liga antes do `rlPushMatrix` e desliga depois do `rlPopMatrix`.
62. **Susto com o rosto do modelo e duração pelo som.** O susto saiu de `criatura.c` para um módulo próprio, `susto.c`. Com o `precursor.glb`, o rosto do modelo (a 90% da altura, medido no arquivo) pula de 2,6 m a 0,2 m da câmera em 0,16 s e fica tremendo: posição e giro novos 40 vezes por segundo, com uma luz branca de baixo que falha e um clarão na chegada. O fundo é o cenário onde o jogador estava, 85% mais escuro, para o foco ficar na criatura (o corpo dela some do cenário enquanto o rosto está na tela). A boca abre e fecha cerca de 4,5 vezes por segundo, num ritmo irregular: como o modelo não tem esqueleto, os ~950 vértices do queixo giram em volta de uma dobradiça perto das orelhas (de 8° para cima até 28° para baixo), com pesos que suavizam a emenda com o resto do rosto. Os vértices são separados uma vez no carregamento; a cada quadro só essa faixa da malha vai para a GPU, e fora do susto o modelo volta ao original. O rosto é desenhado num render target só dele, com a própria profundidade, e colado por cima da cena: assim ele nunca atravessa uma parede. O movimento sai de `regras_quadro_susto`, uma função pura e testada. O som é o `assets/audio/jumpscare.mp3`, e o rosto fica 2,04 s na tela, que é onde termina o trecho alto do arquivo (medido pelo volume, de 20 em 20 ms). O eco continua tocando na tela seguinte. Sem o arquivo, entra o estrondo sintetizado com os 1,2 s de antes; sem o modelo, o rosto procedural em 2D pula e treme do mesmo jeito. A legenda não aparece por cima do rosto. Os sons de `assets/audio/` agora podem ser `.ogg`, `.wav` ou `.mp3`, e o `.mp3` entrou no Git LFS. O susto tem som próprio (`SOM_JUMPSCARE`); o apagão continua com o estrondo sintetizado. Para o susto vir logo que ele chega perto, a captura passou de 1 m para 1,6 m (esferas de 0,8 m: o modelo tem 2,6 m e anda curvado, então a 1 m ele já estava "dentro" do jogador), e, perseguindo a menos de 3 m, ele dá o bote: sem as pausas do andar e 35% mais rápido. A especificação ainda fala em 1 m e em no máximo 1,2 s de susto; a equipe pediu os dois ajustes.

63–68. *Retiradas na junção com a história nova: tratavam da `FASE_NOME` e do(a) operador(a). O que continua valendo (o nome só em memória) está nas decisões 80 a 83.*

## Texturas e ambientação do mapa

69. **Texturas procedurais, com override opcional.** Paredes, piso, teto, móveis e portas eram `DrawCubeV` de cor sólida. Agora cada material (concreto, oliva, piso, teto, metal, porta) é gerado em código quando o jogo abre, a partir de funções puras (`materiais_pixel`, em `materiais.c`, sem `raylib.h` e testadas sem janela). Se existir `assets/texturas/<material>.png`, ele substitui a versão gerada; sem arquivo, o jogo usa a procedural e só registra no log, seguindo a regra de que nenhum asset é obrigatório. A geração leva cerca de 0,13 s, uma vez só, com semente fixa (`TEXTURA_SEMENTE`): a mesma imagem em toda execução.
70. **Criadas uma vez, liberadas ao sair.** As texturas moram em `Recursos` e são criadas em `carrega_recursos` (depois da fonte, porque as placas usam a fonte) e liberadas em `descarrega_recursos`. Nada é gerado, carregado ou enviado à GPU por quadro. Todas são potência de 2, com mipmaps e filtro trilinear; um PNG fora disso é redimensionado ao carregar, porque repetir textura e gerar mipmaps exige potência de 2 no OpenGL 2.1/ES.
71. **UV em metros, não por face.** `materiais_uv` calcula a UV pela posição no mundo: a mesma textura cobre paredes de qualquer tamanho sem esticar, e paredes vizinhas emendam. No concreto e na oliva, o v é preso ao teto (`v = 0` em cima), para os escorridos começarem no alto e a umidade ficar embaixo. O concreto é 512 × 256 e cobre 6 m × 3 m, para a repetição não aparecer nos corredores longos; cada parede ainda desloca a UV (hash do índice). A porta usa UV local (as almofadas acompanham a folha).
72. **Passadas por material.** Trocar de textura encerra o lote da `rlgl`. Por isso as paredes são desenhadas em passadas (todo o concreto, depois todo o barrado oliva, depois rodapé e friso), e não parede por parede. O FPS sem limite caiu cerca de 8% (de 730–800 para 640–715 na máquina de desenvolvimento), ainda mais de 10 vezes a meta de 60.
73. **O shader de luz não mudou.** Ele já multiplicava `texture(texture0, fragTexCoord)`; bastou desenhar com `rlSetTexture` e coordenadas de textura. As versões GLSL 330 e 100 continuam iguais.
74. **Barrado oliva em vez da faixa.** A faixa oliva de 12 cm virou um barrado de tinta do chão até 1,2 m (`BARRADO_ALTURA`), com friso e rodapé de metal escuro: o visual institucional de quartel e hospital dos anos 80. O descascado foi ajustado depois das capturas (a primeira versão, clara e grande, parecia camuflagem).
75. **Ambientação sem colisão (`cenario.c`).** Placas em estêncil ("POSTO 7", "ÁREA RESTRITA", "ARQUIVO", com os textos em `dialogo.c` e a fonte do jogo, traço engrossado e tinta gasta), manchas de umidade, marcas de arrasto saindo do duto, um anel de eletrodutos no teto dos corredores, grelhas de ventilação e fios soltos nos painéis. Nada disso entra em `ColParedes`: a planta, as colisões, a IA e o `--smoke` não mudaram. Os decalques não escrevem profundidade, para a criatura e as partículas passarem por cima. O desgaste das placas foi reduzido depois das capturas (a primeira versão lia "PCSTO").
76. **Halo das lâmpadas.** As lâmpadas acesas ganharam um brilho radial em soma de luz (`HALO_LAMPADA_ALFA` 0,35, 1,3 m), sem escrever profundidade e apagando com a névoa, para não furar o escuro de longe nem estourar o VHS. A luz de jogo (holofotes A/B, apagões) não mudou.

## Sonorização

77. **Sons gravados por streaming, sintetizados como reserva.** Os sons longos de `assets/audio` (trilha do menu, vozes, coração, passos) tocam em laço por `Music` em `trilhas.c`, com fade de 0,4 s (1,5 s na trilha). O volume de cada um sai da função pura `regras_volumes_trilhas`. Sem o arquivo, volta o som sintetizado equivalente. FLAC foi convertido para WAV (a raylib padrão não lê FLAC).
78. **Cada som no seu momento.** A voz do Precursor só toca enquanto ele "diz" uma frase nas rodadas (os quadros dos símbolos no vidro) e ao receber a transmissão final, nunca ao sair do duto. A voz do rádio toca por baixo das legendas do Varela, cada fala começando noutro trecho; com ela, o bip sintetizado do interfone sai. Na tela inicial só toca a trilha: o chiado e o zumbido entram no laboratório.
79. **Coração pela distância.** Com o Precursor solto, nada além de 16 m, batida normal (mais alta quanto mais perto) até 6 m e batida rápida abaixo disso. Passos: a trilha lenta andando, a rápida correndo; o ruído que a criatura ouve não mudou.

## História nova: quem joga (professor ou professora, nome)

80. **As perguntas são etapas da intro, não uma fase.** "Professor ou professora?" e o nome vêm logo depois de INICIAR, antes do texto datilografado, porque a 3ª linha da intro já usa a palavra ("Você é professora de linguística..."). Em vez de uma fase nova na máquina de fases, a `Intro` ganhou um `EtapaIntro` (`INTRO_GENERO`, `INTRO_NOME`, `INTRO_DATILOGRAFA`, `INTRO_RADIO`): é a mesma tela preta e o mesmo evento `EV_INTRO_TERMINOU` no fim. O módulo é `identidade.c`. Esta versão substituiu a `FASE_NOME`/`tela_nome.c` que veio da `develop`, porque na história nova quem joga **é** o professor ou a professora, não um operador ao lado dele.
81. **Marcadores no roteiro.** As falas de `dialogo.c` usam `{NOME}`, `{PROF}`, `{prof}` e `{masculino|feminino}` (ex.: `viv{o|a}`). Quem troca é `regras_monta_texto`, pura e testada, que não conhece nenhuma palavra: recebe uma tabela de marcadores montada por `dialogo_personaliza`. Assim todo texto continua só em `dialogo.c`. Nunca corta uma letra acentuada pela metade.
82. **Título e nome sempre; o nome sozinho é a pista.** Para o nome de quem joga aparecer ao longo da partida, o Varela verdadeiro e a narração usam sempre o título com o nome (`{PROF} {NOME}`: "Professora Inês"), da intro ("Você é a Professora Inês...") ao final. A partir do fim da S2, quem fala no rádio é o Precursor imitando o Varela, e ele chama **só pelo nome**, sem o título ("Inês, ele quer conversar"): íntimo demais. O bilhete do Arquivo dá a pista ("Eu nunca te chamo só pelo nome"). Por isso o nome também voltou ao alto do terminal ("OPERADORA: Professora Inês"). Substitui a primeira versão, em que o Varela nunca dizia o nome, porque o nome perguntado quase não aparecia.
83. **Nome: até 16 letras, só em memória.** Valem letras (com os acentos do Latin-1, que a fonte carrega), espaço entre palavras, hífen e apóstrofo; números não, porque é o nome de um professor de 1985. `regras_nome_edita` faz a digitação e o Backspace (que apaga a letra inteira, mesmo com 2 bytes). ENTER vazio usa "Moura". A primeira letra de cada palavra vira maiúscula ao digitar ("inês" vira "Inês"), porque o nome aparece depois do título. A regra de privacidade veio da versão da `develop` (antiga decisão 63): o buffer é zerado inteiro ao voltar ao menu (fim, pausa, inatividade) e ao começar uma partida nova, continua nos recomeços (vidro rompido, pego, F5) e, nas fases de jogo, vazio vira o padrão (F2, `--fase`). O `--smoke` confere o buffer zerado no menu e o nome padrão na R1. No modo feira, cada pergunta segue sozinha depois de 20 s.
84. **`ImageDraw` na raylib 6.** A raylib 6.0 trocou `ImageDraw` por `ImageDrawImagePro` (com origem e rotação); a 5.5 só tem `ImageDraw`. `texturas.c` escolhe pela versão (`RAYLIB_VERSION_MAJOR`), para o jogo compilar nas duas.
85. **Cada final explica por que aconteceu.** Logo abaixo do texto do final aparece "POR QUE VOCÊ VENCEU" ou "POR QUE VOCÊ PERDEU", com a frase transmitida em notação lógica (a mesma da prévia, `formula_das_fichas`) e o motivo pela tabela-verdade: contradição (falsa em todas as linhas, sem brecha) ou satisfazível (verdadeira em pelo menos uma linha, a brecha). Quem perde recebe a dica para tentar de novo (o rádio que chamava só pelo nome, o bilhete, a frase que nunca pode ser verdade). Quando o texto aparece, a cena para e espera a leitura: o ENTER leva ao placar e, sem ninguém, ele vai sozinho em `FIM_LEITURA_MAX_S` (50 s), com a contagem no rodapé. No placar, ENTER ou 20 s voltam ao menu, como antes. Antes de o texto aparecer, o ENTER continua acelerando a cena 3 s por toque, para a fila da feira andar.

## Filtro VHS e atmosfera

86. **Filtro VHS com presets.** O VHS saiu de `posfx.c` para `vhs.c` e virou uma tabela (`VHS_PRESETS` em `config.h`, uma linha por preset, colunas na ordem de `ParametroVhs`): desligado, original (o da v0), fita caseira (1985), fita gasta, monitor CRT e câmera de vigilância. F1 passa para o próximo e mostra o nome por 2,5 s; `VHS_PRESET_PADRAO` diz com qual o jogo abre. Para escolher sem jogar: `--galeria-vhs pasta` salva o mesmo quadro em cada preset; `--vitrine-vhs` troca sozinho.
87. **O VHS não pode atrapalhar o jogo.** O shader ganhou sangramento de cor (borra só a cor, o contorno fica nítido), faixa de tracking (no rodapé, 3% de baixo, fora da área de jogo), saturação, tom, curvatura e cintilação, com tetos anotados em `config.h` (ruído ≤ 0,08, cintilação ≤ 0,05, curvatura ≤ 0,15). O ruído quase não clareia o preto e as scanlines devolvem o brilho médio que tiram: o escuro do jogo continua o mesmo com ou sem filtro. A interface é desenhada depois do filtro e nunca é afetada.
88. **Interferência perto da criatura.** Nos corredores, uma sombra escura e semitransparente passa pela tela, com um tremor leve dentro dela, quando a criatura está perto. Ela aparece ao cruzar 10 m (nível 0,25: cerca de 14% mais escuro no meio da faixa) e cresce em linha reta até 0,6 a 2,5 m (cerca de 33%, faixa mais larga), suavizado. Uma primeira versão com linhas claras piscando foi descartada: destoava do filtro e chamava atenção demais.
89. **Captura em tela com escala.** No Wayland com escala, `LoadImageFromScreen` devolve uma imagem maior que a janela com o jogo no canto de baixo à esquerda; a captura recorta esse canto.
90. **Ambiente mais escuro, sem ficar injogável.** A luz ambiente caiu de 0,06 para 0,045, a névoa subiu de 0,085 para 0,095 e as lâmpadas dos corredores ficaram com 80% da força (`LUZ_CORREDOR_GANHO`). A lanterna, os holofotes A e B da cela (o enigma depende de ver a resposta do Precursor) e a luz fraca que acompanha o jogador não mudaram: com a lanterna, o corredor continua legível.
91. **Corrida 10% mais rápida.** `JOGADOR_VEL_CORRER` passou de 4,2 para 4,6 m/s. Na perseguição (3,6 m/s) o jogador continua mais rápido; no bote (4,86 m/s) a criatura ainda o alcança. A especificação ainda diz 4,2 m/s.
