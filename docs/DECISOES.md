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
