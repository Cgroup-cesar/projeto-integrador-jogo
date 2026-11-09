# ESPECIFICAÇÃO — PROJETO PRECURSOR (nome provisório)

> **O que é este arquivo:** a especificação completa do jogo e a fonte da verdade do projeto. Ele fica em `docs/ESPECIFICACAO_PROJETO_PRECURSOR.md` dentro do repositório. Só humanos da equipe o editam; o Claude lê e segue. **Como usar:** no Claude Code (terminal ou extensão do VS Code), aberto na pasta do repositório, cite este arquivo com `@` e envie a mensagem de `MENSAGEM_DE_ENVIO.md`. As seções vão do **contexto** (o que construir e por quê) para a **tarefa** (como trabalhar), que está no fim.

---

Este jogo é o \*\*Projeto Integrador\*\* da disciplina \*\*Programação Imperativa e Funcional (PIF)\*\*, Aula 19 ("Projeto Integrador: um jogo em Raylib"), do Prof. Ronierison Maciel, na CESAR School, semestre 2026.2. Ele também precisa atender ao desafio de \*\*Lógica para Computação\*\*: o jogo tem que estar conectado à lógica proposicional. A tese dos slides, que guia toda a arquitetura, é esta: **o game loop imperativo orquestra (o "como"), a lógica em estilo funcional decide (o "o quê") e o estado em structs guarda tudo.** O professor avalia se o código mostra essas duas competências juntas, então a arquitetura importa tanto quanto a jogabilidade.

Quem vai mexer no código depois de você é uma equipe de **4 a 5 estudantes iniciantes**, trabalhando em paralelo, cada um com o seu Claude Code. Por isso o código precisa ser legível por iniciantes, modular (para evitar conflitos de merge) e autoexplicativo.

O jogo será jogado numa **feira de games da faculdade**, por visitantes com pouco tempo e fila atrás. Por isso a partida dura **5 a 8 minutos**, precisa ser entendida sem explicação externa e precisa voltar sozinha ao menu.

Todo o texto do jogo é em **português do Brasil**, com acentuação correta. Identificadores de código também em português, de forma consistente (`atualiza_criatura`, `desenha_terminal`).

Os slides descrevem o projeto como um "jogo 2D", mas aceitam tema próprio "desde que respeite os requisitos". Este jogo é **3D em primeira pessoa**, e isso não deve mudar. O README explica a escolha e mostra que todos os requisitos são cumpridos. 

\<visao\_do\_jogo\> **Premissa.** Brasil, **14 de março de 1985**, véspera da posse do primeiro presidente civil depois de 21 anos de ditadura. No **Posto 7**, uma base subterrânea secreta do SNI na **Serra do Roncador (MT)**, está contido desde 1977 um ser que o governo chama de **Precursor**. O **Prof. Elias Moura**, lógico da USP (o jogador, em primeira pessoa), é convocado à força para "traduzir" o que ele diz. A língua do Precursor não tem palavras: é estrutura pura, verdadeiro e falso. É lógica.

**Twist.** Cada tradução correta ensina a nossa língua ao Precursor e enfraquece a contenção. A última frase traduzida é, literalmente, a chave da cela. O jogador só descobre isso no fim.

**Tom.** Terror misto: começa lento e psicológico (sala, silêncio, zumbido de lâmpada) e termina em pânico (corredores, perseguição, jumpscare).

**Estrutura.** Alternância: sala de observação (lógica) → saída pela instalação (fugir e se esconder, sem combate) → sala de observação → …

**Fluxo de fases:** `MENU → INTRO → R1 → S1 → R2 → S2 → R3 → RESPOSTA_FINAL → FIM_RUIM | FIM_BOM → (20 s) → MENU`.

**Nome provisório.** Em `src/config.h`: `#define TITULO_JOGO "PROJETO PRECURSOR"` e `#define TITULO_PROVISORIO 1`. Enquanto for 1, o título da janela é `PROJETO PRECURSOR [NOME PROVISÓRIO]`, o menu mostra abaixo do título, menor e em âmbar, `[NOME PROVISÓRIO — título final a definir]`, e a tela de fim mostra no rodapé `Projeto Integrador PIF · nome provisório`. Trocar o nome no futuro deve exigir mudar só essas duas linhas. \</visao\_do\_jogo\>

\- \*\*Prof. Elias Moura\*\* (jogador): 41 anos, civil, vulnerável, nunca segurou uma arma. É por isso que não há combate. Não tem modelo visível. \- \*\*Coronel Augusto Varela\*\*: fala só pelo interfone (legenda na tela \+ bip de rádio). Frio, burocrático, impaciente. Sabe menos do que finge. \- \*\*Dra. Irene Sampaio\*\*: linguista do Museu Nacional que estudou o Precursor antes de Elias. Está desaparecida. Só aparece pelo diário no Arquivo, que traz a pista do final bom. \- \*\*O Precursor\*\*: humanoide de \~2,6 m, magro demais, pele cinza-azulada fosca, rosto liso com uma fenda vertical que se abre quando "fala", dedos longos demais. Move-se com pausas e trancos. \*\*Placeholder:\*\* monte-o com primitivas (cilindros finos, cabeça elipsoide), com jitter procedural na rotação e na altura da cabeça, e mantenha-o na penumbra. \<roteiro\_e\_fases\> Todos os textos ficam em `src/dialogo.c`, numa tabela de strings por fase, para o roteirista editar sem tocar na lógica.

**INTRO (\~30 s).** Tela preta, texto datilografado (um clique de máquina de escrever por letra; Enter pula):

1. "SERRA DO RONCADOR, MATO GROSSO — 14 DE MARÇO DE 1985."  
2. "Amanhã o país terá seu primeiro presidente civil em 21 anos."  
3. "Esta noite, o Serviço Nacional de Informações precisa de um tradutor."  
4. "Prof. Elias Moura. Lógica Matemática, USP. Convocado às 03h12. Sem direito a recusa."

Fade para a sala. Interfone (Varela):

- "Boa noite, professor. Coronel Varela. Não encoste no vidro."  
- "Nós o chamamos de Precursor. Está conosco desde 77\. Não fala como nós. Não pensa como nós."  
- "A Dra. Sampaio concluiu que a língua dele não tem palavras. Só estrutura. Verdadeiro. Falso."  
- "Os interruptores à sua frente controlam as luzes da cela. Ele responde ao que vê. Descubra o que ele diz."

**Rodadas (na sala).** O terminal se chama **ANALISADOR SEMÂNTICO SNI-85**, com o cabeçalho `RODADA n/3 · VIDRO: ■ ■ □ · PLACAR: 000`. Cada rodada tem duas etapas:

1. **Decifrar.** O Precursor exibe uma frase com glifos de luz (LUZ\_A, LUZ\_B) e um conectivo desconhecido. O jogador alterna as luzes A/B no console. A cada combinação nova, o Precursor responde com **zumbido grave consonante \= VERDADEIRO** ou **guincho dissonante \= FALSO**, e a linha correspondente da tabela-verdade se preenche sozinha. O valor vem de `logica_avalia`, nunca de uma tabela fixa. Com a tabela completa, o jogador escolhe o significado entre `NÃO / E / OU / SE…ENTÃO`. Escolha errada \= 1 erro.  
2. **Traduzir.** O Precursor "diz" uma frase com o conectivo agora conhecido e palavras do léxico. O jogador escolhe a tradução certa entre 3 opções. Escolha errada \= 1 erro.

| Rodada | Frase(s) de teste | Conectivo | Frase a traduzir | Opções (✔ \= certa) |
| :---- | :---- | :---- | :---- | :---- |
| R1 (tutorial; só a luz A; 2 linhas) | NAO LUZ\_A | NÃO | NAO NOS PERIGO | ✔ "Nós não somos perigo." · "Nós somos perigo." · "Não há nós sem perigo." |
| R2 (luzes A e B; 4 linhas; duas frases observadas ao mesmo tempo, em duas colunas) | LUZ\_A E LUZ\_B · LUZ\_A OU LUZ\_B | E e OU (o jogador identifica os dois) | VOCE E NOS FALAR | ✔ "Você e nós falamos." · "Você ou nós falamos." · "Se você fala, nós falamos." |
| R3 (4 linhas) | LUZ\_A SE\_ENTAO LUZ\_B | SE…ENTÃO | VOCE FALAR SE\_ENTAO PORTA ABRIR | ✔ "Se você falar, então a porta abre." · "A porta abre e você fala." · "Se a porta abrir, você fala." |

Na R1, o terminal mostra uma dica de 2 linhas sobre o que é uma tabela-verdade. Nas demais rodadas, não há dicas.

**Falas entre fases (Varela):**

- Após R1: "'Nós não somos perigo.' ...Anotado." O Precursor dá um passo em direção ao vidro, as luzes piscam e vem o apagão → S1.  
- Início de S1: "Queda de energia. O gerador auxiliar fica no fim do corredor. Sem energia, a contenção... Vá. Agora. E não corra se não precisar. Ele ouve."  
- Fim de S1: "Energia restabelecida. O campo puxou ele de volta. Volte para a sala."  
- Após R2: "'Você e nós falamos.' Ele está aprendendo rápido demais, professor." Apagão maior → S2.  
- Início de S2: "De novo. Agora o painel do Arquivo. É a sala da Dra. Sampaio... não mexa nas coisas dela."  
- Fim de S2: "Contenção estável. Por enquanto."  
- Após R3 (com alarme): "Professor... desligue o transmissor. Isso é uma ordem\!" O Precursor encosta a mão no vidro e o terminal mostra **RESPONDA.**

**Saídas (fugir e se esconder).**

- A porta da sala abre e as luzes ficam vermelhas e fracas. O Precursor surge pelo duto `V` do corredor 15 s depois do início da saída, dando tempo para o jogador se orientar.  
- **S1, Painel 1 (Gerador):** chaves A, B, C e o circuito fixo `SAÍDA = (A ∧ ¬B) ∧ C`. Cada porta tem um LED com a saída ao vivo. O painel resolve quando a lâmpada de saída acende (solução única: A=1, B=0, C=1), mas a checagem é feita avaliando o circuito, nunca comparando com valores fixos.  
- **S2, Painel 2 (Arquivo):** entradas A, B, com as 4 linhas mostradas no painel. Circuito `SAÍDA = (A [?1] B) [?2] (¬A)`. Para cada encaixe, o jogador escolhe `E / OU / OU-EXCLUSIVO / NÃO-E`. Resolve quando a tabela da saída é igual ao alvo `00→1, 01→1, 10→0, 11→1` (ou seja, A→B, um prenúncio da R3). Há duas soluções válidas, (E, OU) e (E, OU-EXCLUSIVO), e o jogo confere a tabela, não uma combinação.  
- **Diário (N)**, no Arquivo, no caminho do painel 2, com uma luz de destaque. Interagir abre uma folha datilografada:  
    
  > "02/03/85 — Cada frase que traduzimos, ele repete melhor. Não é diálogo: é aula, e nós somos os professores. Ele só consegue agir sobre aquilo que PODE ser verdade. Se um dia for preciso calá-lo, dê a ele uma frase que nunca possa ser verdadeira. P E NÃO P. O Varela não pode saber disso. — I.S." Se o diário for lido, o terminal passa a mostrar no canto: `ANOTAÇÃO RECUPERADA: "uma frase que nunca possa ser verdadeira"`.  
    
- Resolvido o painel, a energia volta, a criatura some numa distorção ("puxada" pela contenção) e o jogador volta à sala.

**O Precursor nos corredores.** Estados: `PATRULHA` (pontos no loop de corredores, 1,6 m/s) → `INVESTIGA` (vai até o ruído percebido mais forte; ver a coleção `ruidos`) → `PERSEGUE` (vê o jogador: cone de 70°, até 14 m, com raycast contra paredes; 3,6 m/s) → `PROCURA` (6 s ao redor da última posição vista) → `PATRULHA`. Ele usa A\* na grade do mapa. O jogador anda a 2,5 m/s e corre a 4,2 m/s, com 4 s de fôlego. Correr gera ruído de raio 10 m; andar, de 3 m.

- **Armário:** E entra e sai. A câmera fica fixa olhando pelas frestas (barras pretas). Escondido, o jogador é invisível para a criatura, mas se entrar a menos de 2 m dela enquanto ela o vê, é pego.  
- **Pego** (distância \< 1 m): jumpscare de no máximo 1,2 s (o rosto com a fenda aberta na câmera, estrondo e 0,2 s de silêncio antes) e volta ao início da saída atual.

**Erros e vidas.** Cada erro na sala racha o vidro: overlay de rachaduras, estalo, as luzes falham. No 3º erro o vidro rompe: jumpscare e a rodada atual recomeça com o vidro restaurado. O HUD mostra a integridade do vidro (vidas) e o placar.

**Resposta final.** O terminal oferece as fichas `[VOCÊ FALAR]` (P), `[PORTA ABRIR]` (Q), `[NÃO]`, `[E]`, `[OU]`, `[SE…ENTÃO]`, `[APAGAR]` e `[TRANSMITIR]`, com no máximo 7 fichas na frase. Ao transmitir:

- Frase inválida → "SINTAXE INVÁLIDA" (sem penalidade).  
- Contradição (ex.: `P E NÃO P`) → **FIM BOM** (vitória).  
- Qualquer frase satisfazível → **FIM RUIM** (derrota).

**FIM RUIM (1/2).** "TRANSMISSÃO ACEITA." Os glifos no vidro se reorganizam em português legível: "OBRIGADO, PROFESSOR." O vidro se dissolve, as luzes morrem, a silhueta fica a 1 m do jogador e vem o jumpscare final. Tela preta: "FIM — 1/2. A contenção do Posto 7 nunca foi restabelecida."

**FIM BOM (2/2).** Os glifos se fragmentam, o Precursor convulsiona e congela, e há silêncio total por 2 s. Varela: "...Ele parou. Professor, o que o senhor disse a ele?" Elias (texto): "Nada que pudesse ser verdade." Tela: "FIM — 2/2. O Posto 7 foi lacrado em 15 de março de 1985\. Oficialmente, ele nunca existiu."

**Tela de fim:** VITÓRIA ou DERROTA, placar total, placar por fase, tempo total, erros, "Pressione ENTER" e retorno automático ao menu em 20 s. \</roteiro\_e\_fases\>

A língua é escrita com glifos de linhas numa grade 5×5 (coordenadas 0..4, y para baixo). Em \`src/glifos.c/.h\`, use exatamente estes traços \`{x1,y1,x2,y2}\` e uma função \`desenha\_glifo(id, pos, tamanho, cor, espessura)\`. Eles foram desenhados para não lembrar os símbolos lógicos reais, para não entregar a resposta. 

```
NAO      : {2,0,2,4}, {0,1,4,1}, {0,4,2,4}
E        : {0,2,2,0}, {2,0,4,2}, {2,0,2,4}, {1,4,3,4}
OU       : {0,1,4,1}, {4,1,4,0}, {2,1,2,4}, {0,4,4,4}
SE_ENTAO : {0,0,4,4}, {0,4,2,2}, {4,0,4,2}, {2,4,4,4}
SSE      : {0,0,2,4}, {2,4,4,0}, {0,4,2,0}, {2,0,4,4}
LUZ_A    : {0,0,4,0}, {2,0,2,4}, {1,4,3,4}
LUZ_B    : {0,4,4,4}, {2,0,2,4}, {1,0,3,0}, {0,2,2,2}
NOS      : {0,0,0,4}, {4,0,4,4}, {0,2,4,2}, {2,2,2,4}
VOCE     : {0,0,4,0}, {4,0,0,4}, {0,4,4,4}
PERIGO   : {2,0,0,4}, {2,0,4,4}, {0,4,4,4}, {2,2,2,3}
FALAR    : {0,1,4,1}, {0,3,4,3}, {2,1,2,3}, {0,0,0,1}, {4,3,4,4}
PORTA    : {0,0,0,4}, {4,0,4,4}, {0,0,4,0}, {3,2,3,3}
ABRIR    : {0,0,2,2}, {2,2,0,4}, {4,0,4,4}, {2,2,4,2}
```

Conectivos ainda desconhecidos aparecem em âmbar; os já decifrados, em verde-fósforo. Quando o Precursor "fala", os glifos também aparecem projetados no vidro, tremendo levemente. 

Em \`src/nivel.c\`, construa o nível a partir deste mapa ASCII, mantido como texto para a equipe editar (1 célula \= 2 m, pé-direito 3 m): 

```
########################
#CCCCCC##########.....##
#CCCVCC##########..1..##
#GGGGGG##########L....##
#OOOTOO############D####
#OOSOOOD....L.......L.##
#OOOOOO#.############.##
########.############.##
########L############.##
#......#.############.##
#.N.2..D.############L##
#......#......V.......##
#L.....#################
########################
```

Legenda: `#` parede · `C` cela (o jogador não entra) · `V` na cela \= posição do Precursor contido; `V` no corredor \= duto por onde ele surge nas saídas · `G` vidro de contenção (colisão sólida, transparente) · `O` sala de observação · `S` início do jogador · `T` console (terminal \+ interruptores 1/2 \+ interfone) · `D` porta (vão aberto; nas fases de sala, a porta da sala de observação fica trancada) · `.` piso · `L` armário (esconderijo; ocupa a célula) · `1` painel de circuito 1 · `2` painel de circuito 2 · `N` diário. O loop de corredores (colunas 8 e 21, linhas 5 e 11\) existe para o jogador conseguir despistar a criatura. 

\<estetica\_e\_audio\>

- **Visual:** low-poly moderado, sem exagero retrô. Formas limpas e escuras, com luz quente (lâmpadas incandescentes) contra luz fria (vidro, fósforo).  
- **Filtro VHS sutil** (shader de pós-processamento em `src/posfx.c`, GLSL embutido como string, com versões 330 e 100): ruído \~4%, aberração cromática \~1,5 px, scanlines \~6%, vinheta leve e um "wobble" horizontal raro. Deve ser sentido, não visto. F1 liga/desliga; as constantes ficam em `config.h`.  
- **Paleta:** sombra `#0c0f0c`, concreto `#2b3a33`, verde-oliva `#4a4f3a`, lâmpada `#d9a441`, fósforo `#7dff9a`, vidro/luz fria `#5aa0c8`, alarme `#ff5a4f`, pele do Precursor `#c9d4dc`.  
- **Luz:** shader com até 4 luzes pontuais, ambiente bem baixo e névoa exponencial quase preta. As luzes A e B da cela são as que o jogador controla; com elas apagadas, o Precursor é quase invisível.  
- **Resolução:** janela 1280×720 redimensionável, renderização interna a 60% da janela (configurável), escalonamento bilinear.  
- **Som** (`src/audio_sintese.c`): todos os sons gerados em código (22050 Hz, mono): zumbido de lâmpada, voz do Precursor (2–3 osciladores FM com formantes variando), VERDADEIRO (acorde grave consonante, 0,6 s), FALSO (guincho dissonante subindo, 0,5 s), bip de interfone, máquina de escrever, interruptor, passos (os da criatura mais graves e lentos), estalo de vidro, estrondo de jumpscare, alarme, respiração no armário, ambiência em loop por fase. Se existir `assets/audio/<nome>.ogg`, ele substitui o som sintetizado.  
- **Controles:** WASD · mouse · Shift correr · E interagir/armário · Esc sair do terminal/pausar · 1/2 luzes A/B no console · setas \+ Enter no terminal. **Debug** (liga e desliga em `config.h` com `TECLAS_DEBUG`): F1 VHS · F2 pular fase · F3 mostrar estado e caminho da criatura · F5 reiniciar fase · F9 ir à resposta final · F12 captura de tela para `docs/img/`. \</estetica\_e\_audio\>

\<requisitos\_da\_disciplina\> Estes são os itens cobrados na avaliação de PIF. Cada um precisa ser cumprido de forma genuína, ou seja, fazendo trabalho real na jogabilidade. O professor avalia "uso genuíno dos padrões", então código que só existe para marcar presença conta contra.

Marque cada trecho que cumpre um requisito com o comentário-âncora `// [REQ:<ID>] <explicação curta>`, logo acima dele. IDs: `RF1`…`RF6`, `RNF1`…`RNF6`, `MAP-<coleção>`, `FILTER-<coleção>`, `FOLD-<coleção>` e `PURA-<função>` (ex.: `[REQ:FOLD-ruidos]`, `[REQ:PURA-regras_placar]`). Um trecho pode ter mais de uma âncora. O README usa essas âncoras para apontar arquivo e linhas.

**Requisitos funcionais**

| ID | Exigência dos slides | Como cumprir neste jogo |
| :---- | :---- | :---- |
| RF1 | Janela e game loop a 60 FPS com `GetFrameTime()` | `SetTargetFPS(60)`; `dt = GetFrameTime()` limitado a 0,05 s; todo movimento, temporizador e animação multiplicado por `dt` |
| RF2 | Controle do jogador por teclado/mouse | Câmera em 1ª pessoa, WASD, Shift, E, 1/2, terminal por teclado |
| RF3 | Coleção (vetor) de múltiplos objetos | As coleções abaixo, com capacidade fixa e contador |
| RF4 | Colisões com funções da Raylib | `CheckCollisionBoxes` (jogador × paredes, vidro, armários), `GetRayCollisionBox` (visão da criatura e mira de interação), `CheckCollisionSpheres` (captura) |
| RF5 | Placar, vidas, telas de início e fim | Placar \= fold dos eventos; vidas \= integridade do vidro; menu, intro e tela de fim |
| RF6 | Condição de vitória/derrota | Final bom \= vitória; final ruim \= derrota |

**Coleções** (em `src/colecoes.c/.h`; struct por objeto, vetor de capacidade fixa e contador, nada de `malloc` por quadro). Cada laço recebe o comentário `// padrão MAP`, `// padrão FILTER` ou `// padrão FOLD`, como nos slides, e o filter é feito por compactação no próprio vetor (`v[j++] = v[i]`):

- `Ruido ruidos[MAX_RUIDOS]` (passos, corrida, portas, interruptores): **map** reduz a intensidade com `dt`; **filter** remove os esgotados; **fold** escolhe o ruído percebido mais forte (atenuado pela distância), que leva a criatura ao estado INVESTIGA.  
- `Particula particulas[MAX_PARTICULAS]` (faíscas dos glifos no vidro, poeira no apagão): **map** move e envelhece; **filter** remove as mortas e as fora dos limites.  
- `Rachadura rachaduras[MAX_RACHADURAS]` (cada erro cria de 1 a 3): **map** anima o crescimento; **fold** calcula a integridade do vidro, ou seja, as vidas.  
- `EventoPlacar eventos[MAX_EVENTOS]` (rodada sem erro \+100, cada erro −25, saída sem ser pego \+50, bônus de tempo, final bom \+200): **fold** dá o placar total; **filter \+ fold** dá os pontos por fase na tela de fim.  
- `Interativo interativos[MAX_INTERATIVOS]` (console, painéis, diário, armários, portas): **filter** seleciona os ativos na fase atual; **fold** acha o mais próximo no campo de visão, que é o que a tecla E aciona.  
- `Parede paredes[MAX_PAREDES]` (geradas do mapa): **fold** responde "colidiu com alguma?" usando `CheckCollisionBoxes`.

`src/funcional.h` traz versões genéricas pequenas (`fx_map`, `fx_filter_inplace`, `fx_fold`, com `void*`, tamanho do elemento e ponteiro de função), usadas em pelo menos dois lugares. As versões tipadas continuam onde deixam o código mais claro para iniciantes.

**Funções puras e testáveis** (o nível "Excelente" da rubrica pede "regras em funções puras testáveis"):

- `src/regras.c/.h` reúne as regras como funções puras: dada a entrada, devolvem a saída, sem ler teclado, desenhar, tocar som, usar globais ou chamar `GetFrameTime`. São obrigatórias: `regras_nova_posicao`, `regras_criatura_ve_jogador` (cone e distância; recebe pronto o resultado do raycast), `regras_ruido_percebido`, `regras_proximo_estado_criatura`, `regras_placar`, `regras_integridade_vidro`, `regras_avaliar_circuito` e `regras_proxima_fase(fase, evento)`, que implementa a máquina de fases inteira como uma função pura.  
- `logica.c` e `regras.c` não incluem `raylib.h`. Podem usar só `raymath.h`, que é puro e só de cabeçalho, ou um `Vec3` próprio. Assim os testes rodam sem abrir janela.

**Requisitos não funcionais**

| ID | Exigência dos slides | Como cumprir |
| :---- | :---- | :---- |
| RNF1 | Legibilidade | Nomes claros e consistentes, funções de até \~40 linhas (exceções comentadas), `.clang-format` incluído |
| RNF2 | Modularização | Struct por objeto; uma função por responsabilidade (`atualiza_…`, `desenha_…`, `colidiu_…`); um módulo por sistema; nenhum arquivo acima de \~500 linhas |
| RNF3 | Separação update/render | Todo módulo expõe `xxx_atualiza(Estado *e, float dt)` e `xxx_desenha(const Estado *e)`. O `const` garante que desenhar não muda estado; nenhum `Draw*` dentro de `*_atualiza`. O loop é: entrada → atualiza → desenha |
| RNF4 | Robustez | Vetor cheio (descarta com `TraceLog`, sem crash), lista vazia, objeto fora do mapa, `dt` limitado, asset ausente (placeholder) |
| RNF5 | Documentação | README conforme `<readme>` |
| RNF6 | Versionamento \+ Makefile | Makefile como build principal; commits pequenos e descritivos |

Os slides listam erros comuns a evitar: lógica toda dentro do `while` do loop; desenhar misturado com atualizar; esquecer `GetFrameTime()`; não tratar bordas; README sem instruções de compilação.

Critérios de avaliação (o README mapeia cada um): jogo funcional e jogável 25% · padrões funcionais na lógica 25% · Raylib e C 20% · qualidade do código 15% · documentação e versionamento 10% · apresentação 5%. \</requisitos\_da\_disciplina\>

\<arquitetura\_e\_restricoes\>

- **Linguagem:** C99, sem C++. `-Wall -Wextra` sem warnings no código do projeto.  
- **Raylib 6.0.** No Windows, ela vem do instalador oficial (que traz o compilador w64devkit). Pelo CMake, vem via `FetchContent`, tag `6.0`. O código também deve compilar com a 5.5. Não copie o código da raylib para o repositório.  
- **Makefile na raiz** (build principal, exigido pelos slides), com os alvos `make`, `make run`, `make test` (roda `test_logica`, `test_regras` e `test_fluxo`, todos sem janela), `make smoke`, `make readme-check` e `make clean`. Funciona no Windows com w64devkit (`RAYLIB_PATH`, padrão `C:/raylib/raylib`; confira onde estão `raylib.h` e `libraylib.a` na instalação; linkar `-lraylib -lopengl32 -lgdi32 -lwinmm`) e no Linux (`pkg-config raylib` ou `RAYLIB_PATH`). Detecte o sistema com `ifeq ($(OS),Windows_NT)`.  
- **CMake ≥ 3.20 como alternativa**, para quem não tem a raylib instalada, com os mesmos alvos e testes no CTest. Inclua também `build.bat` e `build.sh`, que só chamam o `make`.  
- **Nenhum asset obrigatório:** geometria de primitivas, sons sintetizados, glifos de linhas. Se existir um arquivo opcional em `assets/` (ex.: `assets/modelos/precursor.glb`), ele é carregado; se não, entra o placeholder sem erro para o jogador (só `TraceLog`).  
- **Fonte com acentos:** a fonte padrão da raylib não tem acentos. `tools/baixa_fonte.sh`/`.bat` baixa uma fonte monoespaçada de licença OFL (VT323 ou IBM Plex Mono) para `assets/fontes/`, carregada com `LoadFontEx` incluindo os codepoints Latin-1 e `→ ↔ ¬ ∧ ∨ ■ □`. Fallback obrigatório: sem a fonte, use a padrão e passe todo texto por `translitera()` (á→a, ç→c…), para nunca aparecer caractere quebrado.  
- **Desempenho:** 60 fps num notebook com vídeo integrado.  
- **Sem dependências** além da raylib e da biblioteca padrão de C.  
- **Motor de lógica** (`src/logica.c/.h`, puro): `typedef enum { OP_ATOMO, OP_NAO, OP_E, OP_OU, OP_IMPLICA, OP_SSE } OpLogico;` e nós alocados num pool estático. Funções: `logica_avalia`, `logica_tabela_verdade`, `logica_eh_tautologia`, `logica_eh_contradicao`, `logica_eh_satisfazivel`, `logica_para_texto` (com ¬ ∧ ∨ → ↔) e `logica_parse_fichas`, com gramática `expr := termo (BINOP termo)*` e `termo := NAO termo | ATOMO`, precedência ¬ \> ∧ \> ∨ \> → \> ↔, associatividade à esquerda (→ à direita) e mensagem de erro amigável.  
- **Testes** (sem framework, com macros `CHECK`):  
  - `test_logica.c`: tabelas dos 5 conectivos; `P∧¬P` contradição; `P∨¬P` tautologia; `P→Q` satisfazível e não tautologia; parser (precedência e erros); as duas soluções do Painel 2 e a solução única do Painel 1\.  
  - `test_regras.c`: regras puras e coleções, incluindo bordas: lista vazia, vetor cheio, ruído esgotado, criatura sem ruídos, placar sem eventos.  
  - `test_fluxo.c`: simula uma partida inteira só com `regras_proxima_fase` e eventos (acertar tudo e transmitir uma contradição chega ao FIM\_BOM; transmitir uma frase satisfazível chega ao FIM\_RUIM; 3 erros reiniciam a rodada; ser pego reinicia a saída).  
- **Modo smoke** (`precursor --smoke`, alvo `make smoke`): abre a janela, joga sozinho a partida inteira em velocidade acelerada usando um "piloto automático" que injeta os mesmos eventos do `test_fluxo`, passa pelos dois finais e sai com código 0 (ou 1 com a fase onde travou). É a forma de provar que o jogo roda de ponta a ponta sem alguém jogando.

**Estrutura de arquivos**

```
Makefile  CMakeLists.txt  build.bat  build.sh  README.md  CLAUDE.md  .clang-format  .gitignore  .gitattributes
src/main.c          game loop imperativo: entrada → atualiza → desenha; modo feira; --smoke
src/config.h        constantes ajustáveis (título, velocidades, tempos, VHS, debug, MAX_*)
src/estado.h        struct Estado (todo o estado do jogo)
src/jogo.c/.h       aplica regras_proxima_fase e despacha atualiza/desenha por fase
src/logica.c/.h     motor de lógica proposicional (puro)
src/regras.c/.h     regras do jogo (puras)
src/colecoes.c/.h   ruídos, partículas, rachaduras, eventos, interativos, paredes
src/funcional.h     fx_map / fx_filter_inplace / fx_fold
src/glifos.c/.h     glifos e desenho
src/rodadas.c/.h    dados e fluxo das 3 rodadas
src/terminal.c/.h   UI do analisador SNI-85
src/compositor.c/.h resposta final por fichas
src/circuito.c/.h   painéis 1 e 2
src/nivel.c/.h      mapa ASCII → geometria, paredes, luzes
src/jogador.c/.h    câmera, movimento, fôlego, armário
src/criatura.c/.h   IA, A*, placeholder procedural
src/dialogo.c/.h    todos os textos
src/audio_sintese.c/.h  sons procedurais + carregamento opcional
src/posfx.c/.h      luz, névoa, VHS
src/ui.c/.h         menu, HUD, legendas, tela de fim
tests/test_logica.c  tests/test_regras.c  tests/test_fluxo.c
assets/{fontes,modelos,audio,texturas}/.gitkeep
docs/ARQUITETURA.md  docs/DECISOES.md  docs/PROGRESSO.md  docs/img/
tools/baixa_fonte.(sh|bat)  tools/check_readme_refs.sh
```

`.gitattributes` coloca `*.glb *.gltf *.png *.jpg *.wav *.ogg *.ttf *.otf` no Git LFS. \</arquitetura\_e\_restricoes\>

1. O \`README.md\` é avaliado (documentação e versionamento, 10%) e é a vitrine do projeto. Se o repositório já tiver um README gerado pelo GitHub, substitua-o. Escreva em português, em Markdown do GitHub, no fim do trabalho (marco M9), quando o código estiver estável. Seções, nesta ordem: **Título e status:** `# PROJETO PRECURSOR`, aviso de nome provisório, pitch de uma frase e uma captura de tela em `docs/img/` (tirada com F12 ou no modo smoke). Se não for possível capturar, deixe um espaço reservado comentado.  
2. **Contexto acadêmico:** Projeto Integrador de PIF (Aula 19, Prof. Ronierison Maciel, CESAR School, 2026.2), ligado também a Lógica para Computação. Em 3 a 5 linhas: a tese dos slides e como o jogo a segue. Inclua a justificativa do 3D.  
3. **Conceito:** premissa, gênero, duração, mecânica central e os dois finais. Twist e solução do final bom dentro de `<details><summary>Spoilers</summary>…</details>`.  
4. **Como jogar:** objetivo, fases e tabela de controles (incluindo as teclas de debug e como desligá-las).  
5. **Como compilar e rodar:** passo a passo para iniciantes no Windows (instalador da raylib \+ w64devkit → `make` → `make run`) e no Linux; alternativa com CMake; `make test`, `make smoke`; problemas comuns (raylib não encontrada, fonte ausente, tela preta por driver).  
6. **Estrutura do projeto:** árvore com uma linha por arquivo.  
7. **Arquitetura:** diagrama Mermaid com as três camadas dos slides (game loop imperativo → lógica funcional → estado em structs) e os módulos de cada uma, mais um parágrafo sobre a separação puro × impuro.  
8. **Como o projeto atende ao que é cobrado** (a seção mais importante). Tabelas para: requisitos funcionais RF1–RF6; cada uso de map, filter e fold; funções puras e onde são testadas; requisitos não funcionais RNF1–RNF6; e os 6 critérios de avaliação, cada um apontando para as linhas das tabelas anteriores que o comprovam. Colunas: **Requisito** · **Onde** (link relativo com linhas, ex.: `[src/colecoes.c#L42-L55](src/colecoes.c#L42-L55)`) · **Âncora** · **Como foi cumprido** (1 frase). Abaixo das tabelas, trechos reais do código final (blocos `c` de até \~15 linhas, com caminho e linhas num comentário na primeira linha), no mínimo: o game loop (RF1), uma colisão com a Raylib (RF4), o placar (RF5), um map, um filter e um fold, uma função pura e o teste dela, e um `*_desenha(const Estado *)` (RNF3).  
9. **A lógica proposicional no jogo:** tabela "conceito → mecânica" apontando para `logica.c`.  
10. **Testes:** o que `make test` e `make smoke` cobrem, com um exemplo de saída.  
11. **Metodologia e versionamento:** `main` (versões estáveis) e `develop` (integração), branch por card a partir da `develop`, PR com revisão, Kanban no monday e como o trabalho se encaixa nas etapas E1–E4 dos slides.  
12. **Créditos e licenças:** raylib (zlib), a fonte (OFL, autor e link) e assets de terceiros, se houver (os slides pedem para creditar). Nomes da equipe como espaço reservado `<!-- preencher -->`.

As referências precisam estar corretas, porque o professor vai clicar nelas. Monte as tabelas a partir de `grep -rn "\[REQ:" src tests`, usando os números de linha reais. Copie os trechos de código do arquivo, sem editar nem simplificar. `tools/check_readme_refs.sh` (alvo `make readme-check`) confere, para cada âncora citada no README, se ela existe no arquivo indicado e se a linha citada está no máximo 3 linhas antes dela. O script também lista os IDs obrigatórios que não aparecem no README. 

\<claude\_md\> Crie o `CLAUDE.md` na raiz. É ele que mantém o Claude Code de cada membro da equipe trabalhando do mesmo jeito nas sessões futuras. Mantenha-o curto (até \~80 linhas) e com:

1. Resumo do jogo em 5 linhas e a indicação de que `docs/ESPECIFICACAO_PROJETO_PRECURSOR.md` é a fonte da verdade do design.  
2. Regras invioláveis: C99 \+ raylib; textos em PT-BR; nenhum asset obrigatório; `logica.c` e `regras.c` sem raylib; `*_desenha` recebe `const Estado *`; laços de coleção comentados com o padrão; preservar as âncoras `[REQ:…]` e, ao mover código ancorado, atualizar o README e rodar `make readme-check`.  
3. Comandos: `make`, `make run`, `make test`, `make smoke`, `make readme-check` (e a alternativa CMake).  
4. Fluxo Git: `main` guarda só versões estáveis (recebe merge da `develop` nas entregas E1–E4 e na build da feira); `develop` é a branch de integração. Nunca commitar direto na `main` nem na `develop`. Cada card ganha uma branch criada a partir da `develop` (`feat/S2-05-ia-precursor`, `fix/BUG-03-armario`). Commits pequenos no padrão `tipo: descrição`. Antes do PR, `git pull --rebase origin develop`. O PR vai para a `develop` e é revisado por um colega. Binários vão pelo LFS.  
5. Regra do Kanban (monday.com): "Se o conector do monday estiver disponível: ao começar uma tarefa, localize o card pelo código (ex.: `S2-05`) no board do projeto e mude o Status para **Fazendo**. Ao terminar, mude para **Revisão** e publique um update com: O que foi feito (3 itens) · Arquivos alterados · Branch/commit · Como testar · Próximo passo/riscos. Só humanos movem para **Feito**, depois de revisar o PR. Trabalho novo descoberto vira card novo (prefixo `BUG-` para defeitos). Sem conector, escreva o texto do update no fim da resposta para alguém colar." \</claude\_md\>

---

\<sua\_tarefa\> Gere neste repositório a **primeira versão jogável (v0)** do PROJETO PRECURSOR, exatamente como descrito acima: do menu até os dois finais, sem nenhum arquivo de arte ou som externo, compilando no Windows e no Linux, com todos os requisitos da disciplina cumpridos e ancorados, testes passando, modo smoke completando a partida e o README com o mapa de requisitos verificado.

"Jogável" significa: alguém que nunca viu o jogo abre o executável, entende o que fazer pelas falas e dicas, resolve as 3 rodadas e os 2 painéis, foge ou se esconde da criatura e chega a um dos finais em 5 a 8 minutos. \</sua\_tarefa\>

\<como\_trabalhar\>

1. **Leia este arquivo inteiro antes de escrever qualquer código.** Depois resuma para si o plano de módulos.  
2. **Verifique o ambiente primeiro:** `git --version`, `gcc --version`, `make --version` e onde estão `raylib.h` e `libraylib.a` (ex.: `C:/raylib/raylib/src`). Se não houver compilador ou `make`, **pare e diga exatamente o que instalar** (é o único motivo para parar). Se faltar só a raylib, use o caminho do CMake.  
3. **Branch:** esta primeira versão é feita direto na branch `develop`, que já existe (`git checkout develop` e `git pull`). Confirme com `git branch --show-current` antes do primeiro commit. Não faça `push`, não toque na `main` e não edite este arquivo de especificação.  
4. **Progresso persistente:** crie `docs/PROGRESSO.md` e atualize-o ao fim de cada marco (o que está pronto, o que falta, decisões, próximos passos). A tarefa é longa e o contexto pode ser compactado ou a sessão pode cair. Ao retomar, leia primeiro `docs/PROGRESSO.md` e o `git log`.  
5. **Fatia vertical primeiro:** o marco M2 deixa o jogo inteiro navegável de ponta a ponta, mesmo que tosco. Depois disso, cada marco aprofunda um sistema sem nunca quebrar o fluxo completo. Se o tempo ou o contexto acabar, o jogo continua jogável.  
6. **Requisitos desde o começo:** escreva o código já com as âncoras `[REQ:…]`, `atualiza`/`desenha` separados e coleções com map/filter/fold. Não deixe isso para refatorar no fim.  
7. **Verifique cada marco antes de seguir:** `make` sem warnings, `make test` 100% e, a partir do M2, `make smoke` saindo com 0\. Se algo falhar, corrija antes de avançar. Faça um commit por marco (`feat: …`, `test: …`, `docs: …`).  
8. **API da raylib:** não invente funções. Se tiver dúvida sobre uma assinatura, abra o `raylib.h` instalado (ou o de `build/_deps`) e confira.  
9. **Ambiguidades:** escolha a opção mais simples que mantenha o jogo jogável e os requisitos cumpridos, registre-a em `docs/DECISOES.md` e siga. Não pare para perguntar entre marcos.  
10. **Prioridade ao cortar:** primeiro o polimento visual; depois o áudio elaborado. Nunca corte a lógica, os requisitos da disciplina, os testes nem o README.  
11. **Auditoria independente:** no M8, se você puder criar subagentes, peça a um deles que revise o código contra `<requisitos_da_disciplina>` sem ver suas conclusões, e corrija o que ele apontar. \</como\_trabalhar\>

\<plano\_de\_execucao\>

- **M0 — Fundação:** Makefile \+ CMake, `.gitignore`/`.gitattributes`/`.clang-format`, `config.h`, `estado.h`, game loop (60 FPS, `dt` limitado, entrada → atualiza → desenha), janela com título provisório, render target \+ VHS (F1), fonte com fallback, `docs/PROGRESSO.md`.  
- **M1 — Núcleo puro:** `logica.c`, `regras.c` (incluindo `regras_proxima_fase`), `funcional.h`, `colecoes.c`, e `test_logica`, `test_regras` e `test_fluxo` passando.  
- **M2 — Fatia vertical:** todas as fases existem e são navegáveis (menu → intro → R1 → S1 → R2 → S2 → R3 → resposta → os dois finais → menu), com versões mínimas de cada uma; mapa em 3D, jogador andando com colisão; `make smoke` passando.  
- **M3 — Rodadas completas:** glifos, terminal, luzes A/B, tabela-verdade ao vivo, traduções, erros, rachaduras, vidas, placar, falas do interfone.  
- **M4 — Saídas completas:** painéis 1 e 2, diário, apagões, portas, retorno à sala.  
- **M5 — Precursor:** placeholder procedural, IA com A\*, ruídos (map/filter/fold), visão por raycast, armários, captura, jumpscare, checkpoints.  
- **M6 — Final:** compositor de fichas, análise por contradição, os dois finais, tela de fim com placar por fase, volta automática ao menu.  
- **M7 — Áudio e atmosfera:** sons procedurais em todos os eventos, iluminação e névoa ajustadas, partículas.  
- **M8 — Polimento e auditoria:** textos PT-BR revisados, teclas de debug, `CLAUDE.md`, `docs/ARQUITETURA.md`. Audite os requisitos com `grep` (`Draw` dentro de `*_atualiza`, movimento sem `dt`, `raylib.h` em `regras.c`/`logica.c`, laços sem o comentário do padrão) e faça a auditoria independente.  
- **M9 — README:** escreva conforme `<readme>`, gere a captura de tela, rode `make readme-check` até 0 erros e faça o commit `docs: README com mapa de requisitos`. \</plano\_de\_execucao\>

\<criterios\_de\_aceite\> Marque cada item no relatório final, com ✅ ou ❌ e uma observação:

- [ ] `make` sem erros e sem warnings no código do projeto; o build CMake também funciona.  
- [ ] `make test` 100% (lógica, regras com bordas, fluxo completo).  
- [ ] `make smoke` completa a partida pelos dois finais e sai com 0\.  
- [ ] Menu e janela mostram o nome provisório conforme `<visao_do_jogo>`.  
- [ ] Textos em PT-BR com acentos corretos (ou transliterados pelo fallback, nunca quebrados).  
- [ ] As tabelas-verdade vêm de `logica_avalia`; o Painel 2 aceita as duas soluções; a resposta final distingue contradição de frase satisfazível.  
- [ ] O Precursor persegue, investiga ruídos, perde o jogador escondido e nunca atravessa paredes.  
- [ ] 3 erros reiniciam a rodada; ser pego reinicia a saída; nenhum softlock.  
- [ ] RF1–RF6 e RNF1–RNF6 cumpridos e ancorados; as 6 coleções usam map/filter/fold de verdade.  
- [ ] `regras.c` e `logica.c` sem `raylib.h`; todo `*_desenha` recebe `const Estado *`; nenhum `Draw*` em `*_atualiza`.  
- [ ] O jogo roda sem nenhum arquivo em `assets/` (a fonte tem fallback).  
- [ ] README completo, com trechos reais, e `make readme-check` com 0 erros.  
- [ ] `CLAUDE.md` com as regras, o fluxo Git e a regra do Kanban; `docs/PROGRESSO.md` atualizado. \</criterios\_de\_aceite\>

\<formato\_da\_resposta\_final\> Ao terminar, responda em português com:

1. **Resumo** do que foi entregue (5 linhas).  
2. **Checklist de aceite** preenchida.  
3. **Como rodar** em até 3 comandos, para Windows e para Linux.  
4. **Mapa de requisitos** resumido (ID → arquivo:linhas), igual ao do README.  
5. **Limitações e placeholders** que a equipe deve substituir, cada um ligado a um card do Kanban (S2-01 … S4-11).  
6. **Texto de update para o card S1-03** do monday (ou publicado no card, se o conector estiver disponível). \</formato\_da\_resposta\_final\>