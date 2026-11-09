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
