# Progresso da v0

Arquivo de retomada: se a sessão cair, leia este arquivo e o `git log` antes de continuar.

| Marco | Situação | Commit |
| :---- | :---- | :---- |
| M0 — Fundação | ✅ pronto | `feat: fundação do projeto (M0)` |
| M1 — Núcleo puro | ✅ pronto | `feat: núcleo puro de lógica e regras com testes (M1)` |
| M2 — Fatia vertical | ✅ pronto | `feat: fatia vertical jogável de ponta a ponta (M2)` |
| M3 — Rodadas completas | ✅ pronto | `feat: rodadas completas com tabela-verdade ao vivo (M3)` |
| M4 — Saídas completas | ✅ pronto | `feat: saídas completas com painéis de circuito e diário (M4)` |
| M5 — Precursor | ✅ pronto | `feat: o Precursor nos corredores com IA, A* e captura (M5)` |
| M6 — Final | ✅ pronto | `feat: resposta final por fichas e os dois finais (M6)` |
| M7 — Áudio e atmosfera | ✅ pronto | `feat: áudio procedural e atmosfera (M7)` |
| M8 — Polimento e auditoria | ✅ pronto | `feat: polimento, auditoria e CLAUDE.md (M8)` |
| M9 — README | ✅ pronto | `docs: README com mapa de requisitos` |

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

## M4 — Saídas completas

**Pronto:** `src/circuito.c` com os dois painéis. Painel 1 (gerador): chaves A, B, C (teclas 1/2/3 ou setas + ENTER), diagrama com as portas NÃO, E, E, um LED com a saída ao vivo em cada porta e a lâmpada de saída; resolve quando a lâmpada acende, avaliando `regras_avaliar_circuito`. Painel 2 (Arquivo): os dois encaixes escolhem E / OU / OU-EXCLUSIVO / NÃO-E (← → encaixe, ↑ ↓ porta), os LEDs percorrem as 4 linhas e a tabela mostra saída × alvo; resolve quando `regras_circuito_confere_tabela` bate com o alvo (as duas soluções valem). Mexer no painel faz barulho (raio 6 m). LEDs e lâmpada também no gabinete 3D. Resolvido: a energia volta (lâmpadas quentes), fala do Varela, a criatura é puxada pela contenção e o jogador precisa voltar à sala (objetivo no HUD); entrar na sala conclui a saída. A porta da sala desliza para dentro da parede no início de cada saída (com barulho); enquanto a energia não volta há apagões curtos aleatórios. O diário (luz de destaque) abre a folha datilografada e acende a anotação no canto do terminal.

**Verificado:** `make` sem avisos; `make test` 100%; `make smoke` passa (o piloto resolve os painéis testando combinações pelo circuito e lê o diário na S2). Capturas conferidas: painel 1 resolvido, painel 2 com a tabela, diário.

**Ajustes de interface:** a legenda do interfone vai para a esquerda quando o terminal está aberto; painéis e folha do diário ficam acima da legenda; setas (← ↑ → ↓) transliteradas sem a fonte externa.

## M5 — Precursor

**Pronto:** em `src/criatura.c`, a IA dos corredores. Ele sobe pelo duto `V` 15 s depois do início de cada saída; a cada quadro percebe (visão: raycast contra as paredes com `GetRayCollisionBox` + `regras_criatura_ve_jogador`, cone de 70° e 14 m, invisível no armário; audição: fold `col_ruidos_mais_forte` sobre a coleção de ruídos) e `regras_proximo_estado_criatura` decide PATRULHA (cantos do loop, gerador, Arquivo; 1,6 m/s) → INVESTIGA (vai ao ruído mais forte) → PERSEGUE (3,6 m/s; direto quando a linha de visão está livre) → PROCURA (6 s em volta da última posição vista). O caminho sai de um A* na grade do mapa, com vizinhança de 4 (andando de centro a centro de células vizinhas ele nunca corta a quina de uma parede). Movimento com pausas e trancos, passos graves. Captura com `CheckCollisionSpheres` (duas esferas de 0,5 m → distância < 1 m): susto (0,2 s de silêncio + rosto com a fenda, ≤ 1,2 s) e a saída recomeça do início (checkpoint), com a fala do Varela; entrar no armário a menos de 2 m enquanto ele vê também é captura. Com o painel resolvido ele some numa distorção. F3 mostra o estado e o caminho do A*. Modelo opcional `assets/modelos/precursor.glb` (escalado para 2,6 m, com o shader de luz); sem ele, o placeholder procedural, sem erro para o jogador. O rosto do susto ganhou sombreado e a fenda com borda viva.

**Verificado:** `make` sem avisos; `make test` 100%; `make smoke` passa e agora, na 1ª partida, fica parado no corredor da S2 fazendo barulho: a criatura precisa passar por INVESTIGA e PERSEGUE e pegar o jogador (a saída recomeça); na 2ª partida entra e sai de um armário. O smoke usa semente fixa (`SetRandomSeed(1985)`) e é determinístico (rodado 3 vezes seguidas). Capturas conferidas: a criatura surgindo do duto e perseguindo no corredor vermelho.

## M6 — Final

**Pronto:** `src/compositor.c` com o compositor de fichas no terminal (RESPONDA.): `[VOCÊ FALAR]` (P), `[PORTA ABRIR]` (Q), `[NÃO]`, `[E]`, `[OU]`, `[SE…ENTÃO]`, `[APAGAR]` e `[TRANSMITIR]`, em grade com destaque, no máximo 7 fichas; a frase aparece em fichas e, quando fecha, em notação lógica (por `logica_para_texto`). TRANSMITIR passa por `regras_avalia_transmissao`: inválida → SINTAXE INVÁLIDA com a explicação do parser, sem penalidade; contradição → FIM BOM; satisfazível → FIM RUIM. Final ruim: TRANSMISSÃO ACEITA → "OBRIGADO, PROFESSOR." sobre ele → o vidro se dissolve e as luzes morrem → a silhueta a 1 m → jumpscare → tela preta com o texto. Final bom: os glifos viram fragmentos, ele convulsiona e congela, 2 s de silêncio, as falas do Varela e do Elias, tela preta com o texto. ENTER acelera as cinemáticas. A tela de fim mostra VITÓRIA/DERROTA, placar total, placar por fase (filter + fold), tempo total, erros, "Pressione ENTER" e volta sozinha ao menu em 20 s.

**Verificado:** `make` sem avisos; `make test` 100%; `make smoke` passa (na 1ª partida o piloto transmite primeiro uma frase inválida, apaga e transmite P SE…ENTÃO Q → final ruim; na 2ª, P E NÃO P → final bom). Capturas conferidas: compositor com SINTAXE INVÁLIDA e a anotação no canto, "OBRIGADO, PROFESSOR.", tela preta do final ruim, fragmentos no final bom e a tela de placar.

## M7 — Áudio e atmosfera

**Pronto:** `src/audio_sintese.c` gera em código (22050 Hz, mono, 16 bits) os 18 sons: zumbido de lâmpada, voz do Precursor (3 osciladores FM com formantes variando), VERDADEIRO (acorde grave consonante, 0,6 s), FALSO (guincho dissonante subindo, 0,5 s), bip do interfone, máquina de escrever, interruptor, passos (os da criatura mais graves e lentos), estalo de vidro, estrondo do susto, alarme, respiração no armário, ambiência em laço da sala e das saídas, papel, a energia voltando e a distorção. Um `assets/audio/<nome>.ogg` substitui o som sintetizado; sem placa de som, o jogo segue em silêncio. Laços por fase (ambiência, zumbido quando a lâmpada está acesa, alarme, respiração), silêncio total antes do estrondo e no final bom. `--exporta-sons <pasta>` grava os sons em WAV; `--mudo` tira o som. Atmosfera: 12 lâmpadas (mais nos corredores), lâmpadas vermelhas mais fortes nas saídas, rodapé escuro e faixa de tinta oliva nas paredes, poeira flutuando perto do jogador (além da poeira do apagão, das fagulhas e dos fragmentos), sem apagão nos 4 primeiros segundos de uma saída.

**Verificado:** `make` sem avisos; `make test` 100%; `make smoke` passa (mudo); os 18 sons exportados e medidos (duração certa, pico ≤ 0,8, sem saturação); capturas conferidas da sala e do corredor.

## M8 — Polimento e auditoria

**Pronto:** `CLAUDE.md` (resumo, regras invioláveis, comandos, fluxo Git, regra do Kanban e commits sem menção ao Claude) e `docs/ARQUITETURA.md` (as três camadas, um quadro, puro × impuro, máquina de fases, módulos, como estender). Textos revisados: todos os rótulos de interface foram para `src/dialogo.c`, os sem uso saíram e o objetivo da S2 diz "ao sul da sala". Âncoras `[REQ:…]` para RF2, RF5, RF6, RNF1, RNF2, RNF3 e RNF6. Teclas de debug conferidas (F1, F2, F3, F5, F9, F12; desligadas no modo feira). `tools/check_readme_refs.sh` escrito para o M9.

**Auditoria por grep:** nenhum `raylib.h` em `logica.c`/`regras.c`; nenhum `Draw*` em `*_atualiza`; todo `*_desenha` recebe `const Estado *`; o único movimento sem `dt` é o olhar pelo mouse (correto: o deslocamento do mouse já é por quadro); três laços de desenho sobre coleções ganharam o comentário "laço de desenho"; `entra_fase` e `main` foram divididas (passavam de 40 linhas).

**Auditoria independente (subagente que não viu estas notas):** corrigido o que ele apontou:
1. O placar do HUD/terminal não mudava na hora do erro → os folds de placar e vidas são recalculados a cada quadro.
2. Entrar no armário à vista da criatura pulava o susto e não contava como "pego" → `criatura_captura` única (susto, recomeço, sem os +50).
3. Muitos erros podiam encher o vetor de eventos e engolir os pontos do final → os erros de uma fase somam num evento só (`col_eventos_acumula`) e `MAX_EVENTOS` foi a 96.
4. O código não seguia o `.clang-format` → formatado com `clang-format` (0 avisos em `--dry-run`).
5. O "3 erros reiniciam a rodada" era decidido no próprio teste → regra pura `regras_evento_do_erro`, usada pelo jogo e testada; o test_fluxo ganhou uma partida com recomeços intercalados (pego na S1, vidro rompido na R2) que chega ao FIM_BOM.
6. Fagulhas, poeira e apagões eram sorteados por quadro → `col_sorteia_por_segundo(taxa, dt)`.
7. `terminal_atualiza` estava vazio → cuida da navegação (setas, Esc); `rodadas.c` decide o ENTER. `jogo_finaliza` (vazio) saiu.
8. A perseguição direta podia atravessar móveis baixos → só vai direto se o passo não colide; senão, segue o A*.
9. `[REQ:RF3]` estava no topo de um arquivo → foi para cima das seis coleções em `estado.h`.
10. Laços que fazem map/filter/fold fora das seis coleções ganharam o rótulo do padrão.
11. Testes: precedência ∨ > → > ↔, vetor cheio e acúmulo de eventos, `regras_evento_do_erro`.
Não aplicado: "commits pequenos" (a especificação manda um commit por marco na v0; ver decisão 45) e o README (é o M9).

**Verificado:** `make` sem avisos; `make test`: test_logica 37/37, test_regras 61/61, test_fluxo 46/46; CTest 3/3; `make smoke` passa (colisão, vidro rompido, captura, armário, dois finais).

## M9 — README

**Pronto:** `README.md` com as 12 seções da especificação, na ordem: título com o aviso de nome provisório, pitch e a captura `docs/img/precursor_rodada2.png` (tirada no modo smoke, na R2); contexto acadêmico com a justificativa do 3D; conceito com o twist e a solução dentro de `<details>`; como jogar (controles, teclas de debug e como desligá-las, opções de linha de comando); como compilar e rodar (Windows passo a passo, Linux, macOS, CMake, testes, fonte, problemas comuns); estrutura; arquitetura em Mermaid com o parágrafo puro × impuro; o mapa de requisitos (RF1–RF6, os 12 usos de map/filter/fold, as 13 funções puras com o teste de cada uma, RNF1–RNF6 e os 6 critérios apontando para as tabelas); 9 trechos copiados do código final; lógica proposicional (conceito → mecânica); testes com a saída real; metodologia; créditos. As funções de teste ganharam âncoras `[REQ:PURA-…]`, para o link do teste cair na linha certa. `tools/check_readme_refs.sh` confere também se cada trecho de código do README é cópia exata das linhas citadas.

**Verificado:** `make readme-check`: 43 linhas de tabela, 9 trechos, 0 erro; `make` sem avisos; `make test` 100%; `make smoke` passa; CTest 3/3.

**Falta (fora da v0, para a equipe):** compilar e jogar no Windows (w64devkit) e no Linux, que não foram testados aqui; rodar `tools/baixa_fonte` para ver os acentos (sem a fonte, o texto sai transliterado); preencher os nomes da equipe nos créditos; publicar a `develop` (`git push`), que ficou só local; trocar o nome provisório quando o título for decidido.
