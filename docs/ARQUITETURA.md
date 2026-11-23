# Arquitetura

A tese dos slides guia o código: **o game loop imperativo orquestra (o "como"), a lógica em estilo funcional decide (o "o quê") e o estado em structs guarda tudo.** Cada camada mora em arquivos diferentes, e a fronteira é visível nos `#include`: a camada de decisão não inclui `raylib.h`.

```mermaid
flowchart TB
    subgraph LOOP["Game loop imperativo (o como): usa a raylib"]
        main["main.c: entrada → atualiza → desenha; --smoke; modo feira"]
        jogo["jogo.c: aplica a máquina de fases e despacha por fase"]
        mods["jogador.c · lanterna.c · criatura.c · sanidade.c · rodadas.c · circuito.c · compositor.c · nivel.c · glifos.c"]
        arte["texturas.c · cenario.c: texturas do mapa (geradas uma vez) e ambientação sem colisão"]
        tela["terminal.c · ui.c · posfx.c · vhs.c · audio_sintese.c · dialogo.c"]
    end
    subgraph DECIDE["Lógica funcional (o o quê): sem raylib.h, testável sem janela"]
        logica["logica.c: árvore, avaliação, tabela-verdade, parser, contradição"]
        regras["regras.c: máquina de fases, movimento, visão, ruído, placar, vidro, circuitos"]
        mat["materiais.c: ruído, desenho de cada material, UV de mundo"]
        fx["funcional.h: fx_map · fx_filter_inplace · fx_fold"]
        col["colecoes.c/.h: as 6 coleções e seus map/filter/fold"]
    end
    subgraph ESTADO["Estado em structs"]
        est["estado.h: Estado (Jogador, Criatura, Rodada, Painel, Compositor, coleções...)"]
        cfg["config.h: constantes ajustáveis"]
    end
    main --> jogo --> mods
    jogo --> tela
    mods -->|"pergunta"| regras
    mods -->|"pergunta"| logica
    mods --> arte
    arte -->|"pergunta"| mat
    regras --> logica
    col --> regras
    col --> fx
    mods -->|"lê e escreve"| est
    tela -->|"só lê (const Estado *)"| est
```

## Um quadro

1. **Entrada** (`main.c`): lê teclado e mouse uma vez e preenche `Estado.in`. No `--smoke`, quem preenche é o piloto automático; o resto do jogo não sabe a diferença.
2. **Atualiza** (`jogo_atualiza` e os `xxx_atualiza(Estado *e, float dt)`): cada módulo muda o estado e, quando a fase precisa mudar, só **pede** um evento (`e->evento = EV_RODADA_CONCLUIDA`). No fim do quadro, `jogo_aplica_evento` pergunta à função pura `regras_proxima_fase(fase, evento)` qual é a próxima fase e prepara o mundo. O `dt` vem de `GetFrameTime()`, limitado a 0,05 s, e multiplica todo movimento, temporizador e animação. `sanidade_atualiza` roda logo depois de `criatura_atualiza` (usa a distância deste quadro para a queda e o grão); os bônus de sanidade das tarefas entram em `jogo_aplica_evento`, junto com o placar. A barra (`sanidade_desenha`) é desenhada na interface, depois do shader; o grão vai para o shader do VHS.
3. **Desenha** (`xxx_desenha(const Estado *e)`): o `const` garante que desenhar não muda o estado. A cena 3D vai para um render target a 60% da janela (`posfx.c`), recebe luz e névoa por shader e o filtro VHS (`vhs.c`); a interface (terminal, painéis, HUD, legendas) é desenhada por cima, na resolução da janela.
   Dentro do shader de luz, a ordem é: `nivel_desenha` (piso, teto, paredes em passadas por material, móveis) → `cenario_desenha` (eletrodutos, grelhas, fios e, por último, os decalques com alfa, sem escrever profundidade) → criatura → lanterna. Fora do shader, `nivel_desenha_emissivos` desenha as lâmpadas e os halos (soma de luz, sem profundidade).

## Recursos carregados uma vez

`main.c` chama `carrega_recursos` antes do primeiro quadro e `descarrega_recursos` ao sair. Tudo o que vai para a GPU fica em `Estado.rec` (`Recursos`): render targets e shaders (`posfx.c`), fonte (`ui.c`), **texturas do mapa** (`texturas_carrega`, depois da fonte, porque as placas em estêncil usam a fonte), sons e modelos. Nenhum desses é criado por quadro. `texturas_carrega` gera cada material a partir de `materiais_pixel` (puro) ou lê `assets/texturas/<material>.png`, gera mipmaps e manda para a GPU; `texturas_descarrega` libera todas.

## Puro × impuro

| Puro (sem efeitos, testado em `tests/`) | Impuro (efeitos: tela, som, entrada) |
| :---- | :---- |
| `logica.c`: `logica_avalia`, `logica_tabela_verdade`, `logica_eh_contradicao`, `logica_parse_fichas` | `rodadas.c` converte os glifos em fichas e pergunta à lógica o valor de cada linha |
| `regras.c`: `regras_proxima_fase`, `regras_nova_posicao`, `regras_criatura_ve_jogador`, `regras_ruido_percebido`, `regras_proximo_estado_criatura`, `regras_placar`, `regras_integridade_vidro`, `regras_avaliar_circuito` | `jogador.c`, `criatura.c`, `circuito.c`, `compositor.c`, `jogo.c` aplicam as respostas ao estado |
| `funcional.h` e os laços de `colecoes.c` (o `TraceLog` do vetor cheio é o único efeito) | `nivel.c`, `glifos.c`, `terminal.c`, `ui.c`, `posfx.c`, `audio_sintese.c` desenham e tocam |
| `materiais.c`: `materiais_pixel`, `materiais_fbm`, `materiais_mancha`, `materiais_uv` | `texturas.c` transforma em imagem e textura (uma vez); `cenario.c` e `nivel.c` desenham |

Por exemplo, a criatura: `criatura.c` faz o raycast contra as paredes (`GetRayCollisionBox`) e o fold dos ruídos, monta uma `PercepcaoCriatura` e entrega a `regras_proximo_estado_criatura`, que decide PATRULHA, INVESTIGA, PERSEGUE ou PROCURA. O caminho sai de um A* na grade do mapa.

## Máquina de fases

```mermaid
stateDiagram-v2
    [*] --> MENU
    MENU --> INTRO: INICIAR
    note right of INTRO: etapas: professor ou professora, nome, texto datilografado, carta pelo rádio
    INTRO --> R1: INTRO_TERMINOU
    R1 --> S1: RODADA_CONCLUIDA
    S1 --> R2: SAIDA_CONCLUIDA
    R2 --> S2: RODADA_CONCLUIDA
    S2 --> R3: SAIDA_CONCLUIDA
    R3 --> RESPOSTA_FINAL: RODADA_CONCLUIDA
    RESPOSTA_FINAL --> FIM_BOM: TRANSMITIU_CONTRADICAO
    RESPOSTA_FINAL --> FIM_RUIM: TRANSMITIU_SATISFAZIVEL
    FIM_BOM --> MENU: VOLTAR_MENU (ENTER ou 20 s)
    FIM_RUIM --> MENU: VOLTAR_MENU (ENTER ou 20 s)
    note right of MENU: VOLTAR_MENU vale de qualquer fase (pausa, inatividade no modo feira) e apaga o nome
    R1 --> R1: VIDRO_ROMPEU (3º erro)
    R2 --> R2: VIDRO_ROMPEU
    R3 --> R3: VIDRO_ROMPEU
    S1 --> S1: PEGO
    S2 --> S2: PEGO
```

## Módulos

| Arquivo | Responsabilidade |
| :---- | :---- |
| `main.c` | janela, game loop, leitura da entrada, capturas, `--smoke` (piloto automático) |
| `jogo.c` | aplica `regras_proxima_fase`, prepara cada fase, pontua, pausa, debug, modo feira |
| `config.h` · `estado.h` | constantes · todo o estado do jogo |
| `logica.c` · `regras.c` · `regras_texto.c` · `regras_ranking.c` · `funcional.h` · `colecoes.c` | a camada pura (`regras_texto.c`: marcadores do roteiro, nome digitado e legenda com defeito do rádio falso; `regras_ranking.c`: o lugar da partida no ranking e a linha do arquivo) |
| `nivel.c` | mapa ASCII → paredes, móveis, interativos, luzes; desenho do bloco de contenção com as texturas |
| `materiais.c` | puro: ruído contínuo nas bordas, o desenho de cada material (`materiais_pixel`), manchas, desgaste do estêncil e `materiais_uv` (UV em metros) |
| `texturas.c` | gera ou carrega as texturas uma vez, libera ao sair; `texturas_caixa` (6 faces, UV de mundo ou local) e `texturas_quadro` (decalque) |
| `cenario.c` | ambientação sem colisão: placas em estêncil, umidade, arrasto do duto, eletrodutos, grelhas, fios e halo das lâmpadas |
| `jogador.c` | câmera, movimento com colisão, fôlego, passos, armário, mira |
| `respiracao.c` | o armário: ao entrar, o Precursor para na porta e a pessoa segura a respiração (ESPAÇO sobre a faixa branca; 3 chances); tutorial na 1ª vez da partida, com o mundo congelado |
| `lanterna.c` | a lanterna: F liga e desliga, a 5ª luz do shader (cone saindo dos olhos) e o modelo na mão |
| `criatura.c` | o Precursor: corpo procedural, IA, A*, captura |
| `susto.c` | o jumpscare: silêncio, grito e o rosto do modelo pulando até a câmera e tremendo (movimento em `regras_quadro_susto`); dura o trecho alto do `jumpscare.mp3` |
| `rodadas.c` · `terminal.c` · `glifos.c` | as 3 rodadas (o ENTER decide), o analisador SNI-85 (setas, H e Esc), a língua dos glifos |
| `manual.c` | o manual dos conectivos que o terminal e os painéis compartilham: lâmpada, ✓/✗, mini-tabela e cartão |
| `crt.c` | a cara de monitor de fósforo das telas de tarefa (analisador, painéis, manual): moldura com quinas, scanlines, varredura descendo, caixas com legenda e opções em faixa |
| `teclas.c` | a barra de teclas do rodapé e o minimapa do teclado: só as teclas que valem no contexto |
| `circuito.c` | os painéis 1 e 2: corrente nos fios, o ponto que trava (`regras_circuito_bloqueio`), barulho e movimentos |
| `compositor.c` | a resposta final por fichas e os dois finais |
| `identidade.c` | as perguntas do começo da intro: "Professor ou professora?" e o nome (a edição é `regras_nome_edita`) |
| `dialogo.c` | todos os textos; `dialogo_personaliza` troca `{NOME}`, `{PROF}` e `{x\|y}` (regra em `regras_monta_texto`) |
| `audio_sintese.c` | os sons curtos gerados em código (ou lidos de `assets/audio/`) |
| `trilhas.c` | os sons longos gravados: trilha do menu, vozes, coração e passos (o volume vem de `regras_volumes_trilhas`) |
| `posfx.c` | render target, luz e névoa |
| `vhs.c` | filtro VHS: shader, presets (`VHS_PRESETS`), troca com F1 e o efeito de sanidade (cansaço + fita) |
| `sanidade.c` | sanidade: queda pela distância da criatura, recuperação, bônus e a barra no topo (regras em `regras_sanidade.c`) |
| `ui.c` | fonte com fallback, HUD, legendas, diário, pausa |
| `menu.c` | a tela de início como gravação de fita (▶ PLAY, ● REC, AGO 1985): título que pifa, gancho, frases subliminares, opções em faixas, convite do 1º lugar e a tabela do ranking por cima |
| `fita.c` | a fita do avistamento por trás da intro datilografada: 3 fotos de `assets/fita/` com zoom lento, tremida, franjas de cor, OSD da câmera (▶ PLAY, hora, 05.06.1985) e o sinal caindo antes da 3ª linha (`regras_fita_tomada`) |
| `tela_fim.c` | depois do final: desempenho (resultado, placar, lugar no ranking), avaliação em estrelas e a tabela do ranking |
| `ranking.c` | só arquivo: lê e grava `ranking.txt` (temporário + troca) e anexa `avaliacoes.csv`; nada no `--smoke` |

Módulos sem nada para desenhar (`audio_sintese.c`) ou puros (`logica.c`, `regras.c`, `regras_texto.c`, `regras_ranking.c`, `colecoes.c`, `dialogo.c`, `materiais.c`) não têm `xxx_desenha`. `texturas.c` e `cenario.c` só desenham a partir do estado e não têm nada para atualizar por quadro, então não têm `xxx_atualiza`.

## Como estender

- **Nova fala ou texto:** só `src/dialogo.c`.
- **Novo som:** um caso em `sintetiza()` (`src/audio_sintese.c`) e um nome em `NOMES`; um `.ogg` com o mesmo nome em `assets/audio/` substitui.
- **Mudar o mapa:** edite o texto ASCII em `src/nivel.c`. Paredes, colisão, interativos e o A* saem dele.
- **Nova regra de jogo:** escreva a função pura em `regras.c`, teste em `tests/test_regras.c` e chame do módulo impuro.
- **Nova fase:** um valor em `Fase`, as transições em `regras_proxima_fase` (com teste em `test_fluxo.c`), a preparação em `entra_fase` (`jogo.c`) e o roteiro em `regras_roteiro_partida` para o smoke continuar jogando a partida inteira.
