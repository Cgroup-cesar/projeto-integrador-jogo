# CLAUDE.md — PROJETO PRECURSOR

## O jogo em 5 linhas
Terror em 1ª pessoa, 3D, em C99 com raylib: Brasil, 14/03/1985, base secreta do SNI na Serra do Roncador.
O Prof. Elias Moura (o jogador) traduz o Precursor, um ser cuja língua é lógica pura (verdadeiro/falso).
Na sala: decifra conectivos pela tabela-verdade e traduz frases (3 rodadas). Nos corredores: religa a energia
fugindo e se escondendo da criatura (2 saídas). No fim, transmite uma frase: contradição = final bom.
Partida de 5 a 8 min, feita para feira (volta sozinha ao menu). **A fonte da verdade do design é
`docs/ESPECIFICACAO_PROJETO_PRECURSOR.md`** (só humanos editam). Decisões tomadas: `docs/DECISOES.md`.

## Regras invioláveis
- C99 + raylib (5.5 ou mais nova); nenhuma outra dependência. `-Wall -Wextra` sem avisos no nosso código.
- Todo texto do jogo em português do Brasil, com acentos, e só em `src/dialogo.c`.
- Nenhum asset obrigatório: sem arquivo em `assets/`, o jogo usa o placeholder (fonte, som, modelo) e só faz `TraceLog`.
- `src/logica.c` e `src/regras.c` não incluem `raylib.h` (só `raymath.h`): os testes rodam sem janela.
- Nos arquivos que usam a raylib, `raylib.h` (via `estado.h`) vem antes de qualquer cabeçalho que inclua `raymath.h`.
- Todo módulo tem `xxx_atualiza(Estado *e, float dt)` e `xxx_desenha(const Estado *e)`; nenhum `Draw*` em `*_atualiza`.
  Todo movimento, temporizador e animação usa `dt` (ou o tempo).
- Laços sobre coleções levam o comentário `// padrão MAP`, `// padrão FILTER` ou `// padrão FOLD`; filter é por
  compactação no próprio vetor (`v[j++] = v[i]`); nada de `malloc` por quadro.
- Preserve as âncoras `// [REQ:<ID>]`. Ao mover código ancorado, atualize os links do `README.md` e rode `make readme-check`.
- Módulos: até ~500 linhas; funções: até ~40 linhas (exceções comentadas).
- Formate antes do PR: `clang-format -i src/*.c src/*.h tests/*.c` (no macOS: `xcrun clang-format`).

## Comandos
```
make                compila em bin/            make test          testes sem janela (lógica, regras, fluxo)
make run            abre o jogo                make smoke         piloto automático: joga até os dois finais
make readme-check   confere as referências     make clean         apaga obj/ e bin/
```
Alternativa: `cmake -B build && cmake --build build && ctest --test-dir build` (`--target smoke` também existe).
Debug no jogo (`TECLAS_DEBUG` em `src/config.h`): F1 VHS · F2 pula fase · F3 estado e caminho da criatura ·
F5 reinicia a fase · F9 resposta final · F12 captura em `docs/img/`. Linha de comando: `--fase S2`, `--mudo`,
`--feira`, `--captura arquivo.png --captura-em 2 [--captura-fase R1]`, `--exporta-sons pasta`.
No macOS, a janela só abre com a tela acordada (`caffeinate -u -t 5` antes de rodar testes automáticos).

## Fluxo Git
- `main` guarda só versões estáveis (recebe merge da `develop` nas entregas E1–E4 e na build da feira).
- `develop` é a branch de integração. **Nunca commitar direto na `main` nem na `develop`.**
- Cada card ganha uma branch criada a partir da `develop`: `feat/S2-05-ia-precursor`, `fix/BUG-03-armario`.
- Commits pequenos no padrão `tipo: descrição` (`feat:`, `fix:`, `docs:`, `test:`, `refactor:`).
- Commits e PRs sem nenhuma menção ao Claude: não acrescente `Co-Authored-By: Claude` nem linha de atribuição.
- Antes do PR: `git pull --rebase origin develop`, `make`, `make test` e `make smoke`.
- O PR vai para a `develop` e é revisado por um colega. Binários (`.png`, `.ogg`, `.ttf`, `.glb`...) vão pelo Git LFS.

## Kanban (monday.com)
"Se o conector do monday estiver disponível: ao começar uma tarefa, localize o card pelo código (ex.: `S2-05`)
no board do projeto e mude o Status para **Fazendo**. Ao terminar, mude para **Revisão** e publique um update
com: O que foi feito (3 itens) · Arquivos alterados · Branch/commit · Como testar · Próximo passo/riscos.
Só humanos movem para **Feito**, depois de revisar o PR. Trabalho novo descoberto vira card novo (prefixo
`BUG-` para defeitos). Sem conector, escreva o texto do update no fim da resposta para alguém colar."

## Onde mexer
Textos: `src/dialogo.c` · balanceamento e cores: `src/config.h` · mapa: `src/nivel.c` (texto ASCII) ·
rodadas: `src/rodadas.c` · painéis: `src/circuito.c` · criatura: `src/criatura.c` · sons: `src/audio_sintese.c`.
Arquitetura e fluxo de um quadro: `docs/ARQUITETURA.md`. Progresso da v0: `docs/PROGRESSO.md`.
