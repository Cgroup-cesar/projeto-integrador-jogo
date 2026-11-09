# Progresso da v0

Arquivo de retomada: se a sessão cair, leia este arquivo e o `git log` antes de continuar.

| Marco | Situação | Commit |
| :---- | :---- | :---- |
| M0 — Fundação | ✅ pronto | `feat: fundação do projeto (M0)` |
| M1 — Núcleo puro | ✅ pronto | `feat: núcleo puro de lógica e regras com testes (M1)` |
| M2 — Fatia vertical | ⏳ próximo | |
| M3 — Rodadas completas | | |
| M4 — Saídas completas | | |
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
