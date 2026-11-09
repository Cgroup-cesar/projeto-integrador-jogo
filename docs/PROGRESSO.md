# Progresso da v0

Arquivo de retomada: se a sessão cair, leia este arquivo e o `git log` antes de continuar.

| Marco | Situação | Commit |
| :---- | :---- | :---- |
| M0 — Fundação | ✅ pronto | `feat: fundação do projeto (M0)` |
| M1 — Núcleo puro | ✅ pronto | `feat: núcleo puro de lógica e regras com testes (M1)` |
| M2 — Fatia vertical | ✅ pronto | `feat: fatia vertical jogável de ponta a ponta (M2)` |
| M3 — Rodadas completas | ✅ pronto | `feat: rodadas completas com tabela-verdade ao vivo (M3)` |
| M4 — Saídas completas | ⏳ próximo | |
| M5 — Precursor | | |
| M6 — Final | | |
| M7 — Áudio e atmosfera | | |
| M8 — Polimento e auditoria | | |
| M9 — README | | |

## M0 — Fundação

**Pronto:** `Makefile` (Windows, Linux e macOS) e `CMakeLists.txt` com os mesmos alvos; `build.sh`/`build.bat`; `.gitignore`, `.gitattributes` (binários no LFS) e `.clang-format`; `src/config.h` com todas as constantes; `src/estado.h`; game loop em `src/main.c` (60 FPS, `dt` limitado a 0,05 s, entrada → atualiza → desenha); janela 1280×720 redimensionável com o título provisório; render target a 60% da janela com escala bilinear; filtro VHS em GLSL 330/100 (F1 liga/desliga); fonte opcional em `assets/fontes/` com fallback para a padrão e `translitera()`; `tools/baixa_fonte.sh`/`.bat`; esqueleto dos testes com a macro `CHECK`.

**Verificado:** `make` sem avisos; `make test` 100% (esqueleto); janela abre e mostra o título provisório (captura conferida).

**Falta:** tudo a partir do M1.

**Observação do ambiente:** neste Mac, a raylib só abre janela com a tela acordada (com a tela dormindo, o GLFW não acha o monitor). Para testes automáticos, acorde a tela antes (`caffeinate -u -t 5`).

## M1 — Núcleo puro

**Pronto:** `src/logica.c` (árvore num pool estático; `logica_avalia`, `logica_tabela_verdade`, tautologia/contradição/satisfazível, `logica_para_texto` com ¬ ∧ ∨ → ↔ e parênteses mínimos, `logica_parse_fichas` com precedência ¬ > ∧ > ∨ > → > ↔, → à direita e mensagens de erro amigáveis); `src/regras.c` (máquina de fases `regras_proxima_fase`, roteiro da partida usado pelo test_fluxo e pelo smoke, `regras_avalia_transmissao`, movimento, visão da criatura, ruído percebido, estados da criatura, placar e placar por fase, bônus de tempo, integridade do vidro, interativos por fase, circuitos dos painéis avaliados porta a porta com a semântica de `logica_aplica`); `src/funcional.h` (`fx_map`, `fx_filter_inplace`, `fx_fold`); `src/colecoes.c` (as 6 coleções, com vetor cheio tratado por `TraceLog`).

**Verificado:** `make` sem avisos; `make test`: test_logica 35/35, test_regras 54/54, test_fluxo 36/36; CTest 3/3.

**Decisões:** ver itens 9 a 12 de `docs/DECISOES.md`.

## M2 — Fatia vertical

**Pronto:** todas as fases existem e são navegáveis: menu → intro (texto datilografado + falas do interfone) → R1 → S1 → R2 → S2 → R3 → resposta → os dois finais → menu. `src/nivel.c` monta o Posto 7 a partir do mapa ASCII (corridas de `#` viram uma caixa só; vidro, mureta, console, painéis, mesa do diário, armários encostados na parede, vergas das portas, duto) e escolhe 4 luzes por quadro; `src/posfx.c` ganhou o shader de luz e névoa (GLSL 330/100); `src/jogador.c` tem câmera em 1ª pessoa, WASD com colisão eixo por eixo (fold de `CheckCollisionBoxes`), corrida com 4 s de fôlego, passos que geram ruído nas saídas, mira (fold de `GetRayCollisionBox`) e armário; `src/jogo.c` aplica `regras_proxima_fase`, prepara cada fase, pontua e cuida de pausa, debug (F2, F3, F5, F9, F12) e modo feira; `src/ui.c` tem menu, HUD (vidro, placar, fôlego, objetivo, mira), legendas, pausa, diário e tela de fim com placar por fase; `src/dialogo.c` concentra todos os textos; `src/glifos.c` tem os 13 glifos e a projeção no vidro.

**Versões mínimas (completadas nos próximos marcos):** rodadas e terminal (ENTER conclui), painéis (ENTER resolve), resposta final ([1] contradição, [2] satisfazível), Precursor parado na cela, finais sem cinemática, áudio só com a interface.

**Verificado:** `make` sem avisos; `make test` 100%; `make smoke` sai com 0 jogando as duas partidas (final ruim e final bom), conferindo cada troca de fase contra `regras_roteiro_partida` e testando a colisão com o vidro; capturas do menu, da R1 e da S1 conferidas.

## M3 — Rodadas completas

**Pronto:** `src/rodadas.c` com os dados das 3 rodadas (frases de teste, conectivos, frase a traduzir e a opção certa) e o fluxo decifrar → escolher o conectivo → traduzir. As luzes A/B (teclas 1/2) controlam as luzes da cela; a cada combinação nova o Precursor "responde" e a linha da tabela se preenche com o valor de `logica_avalia` sobre a frase convertida em fichas (nunca uma tabela fixa). Na R2 ele responde às duas frases, uma depois da outra. Com a tabela completa o jogador escolhe NÃO/E/OU/SE…ENTÃO (o conectivo decifrado passa de âmbar a verde-fósforo, também nas rodadas seguintes) e depois a tradução, com o léxico embaixo de cada glifo. Cada erro: −25, de 1 a 3 rachaduras (dano somando 1 vida), estalo e luzes falhando; no 3º erro o vidro rompe, vem o susto (0,2 s de silêncio + rosto com a fenda) e a rodada recomeça com o vidro restaurado. Depois de cada rodada, a cena da especificação: fala do Varela; R1 com o passo em direção ao vidro e o apagão; R2 com apagão maior; R3 com alarme, a mão no vidro e RESPONDA. `src/terminal.c` desenha o SNI-85 (cabeçalho com rodada, vidro e placar; frases; luzes; tabela ao vivo; opções com destaque; dica de 2 linhas na R1; anotação do diário no canto). `src/glifos.c` projeta no vidro, tremendo, as frases que ele está dizendo, com fagulhas (partículas). O Precursor ganhou o corpo procedural (pernas, tronco, braços e dedos longos, cabeça elipsoide com a fenda que abre ao falar, trancos), quase invisível com as luzes da cela apagadas.

**Verificado:** `make` sem avisos; `make test` 100%; `make smoke` passa e agora atravessa o caminho do erro (na 1ª partida o vidro rompe na R3 e ela recomeça; na 2ª a R2 tem um erro). Capturas conferidas: tabela se preenchendo na R2, glifos no vidro, R3 com RESPONDA, rachaduras e o susto.

**Bug corrigido no caminho:** as vidas eram recalculadas antes de a rodada limpar as rachaduras, e o recomeço mostrava o vidro zerado.
