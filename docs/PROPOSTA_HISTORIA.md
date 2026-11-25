# Proposta: a história nova (Chã de Alegria, 1985)

> **O que é este arquivo:** uma proposta para revisão humana. A especificação
> (`docs/ESPECIFICACAO_PROJETO_PRECURSOR.md`) e o `CLAUDE.md` só são editados por pessoas da equipe. Cada bloco
> abaixo diz **onde colar** e **o que substitui**. Depois de colado, este arquivo pode ser apagado.
>
> A mecânica não muda: 3 rodadas na sala, 2 saídas pelos corredores, resposta final por fichas. Mudam a
> premissa, os personagens, as falas, o texto do Arquivo e os finais (de 2 para 3).

---

## Bloco 1 — `CLAUDE.md`, seção "O jogo em 5 linhas" (substitui o texto inteiro da seção)

```
Terror em 1ª pessoa, 3D, em C99 com raylib: Chã de Alegria (PE), agosto de 1985. Uma nave parada sobre a cidade;
no bloco militar embaixo dela, o Precursor, um ser cuja língua é lógica pura (verdadeiro/falso). O jogador é
professor ou professora de linguística da UFPE (gênero e nome escolhidos no início). Na sala: decifra conectivos
pela tabela-verdade e traduz frases (3 rodadas). Nos corredores: religa a energia fugindo e se escondendo da
criatura (2 saídas). No fim, transmite uma frase: contradição, satisfazível ou tautologia decidem os 3 finais.
Partida de 5 a 8 min, feita para feira (volta sozinha ao menu). **A fonte da verdade do design é
`docs/ESPECIFICACAO_PROJETO_PRECURSOR.md`** (só humanos editam). Decisões tomadas: `docs/DECISOES.md`.
```

---

## Bloco 2 — Especificação, `<visao_do_jogo>`: parágrafos "Premissa", "Twist" e "Fluxo de fases"

**Premissa.** Brasil, **agosto de 1985**. Em maio, um objeto voador em forma de besouro gigante, com luz neon
azul (o "OVNI 85"), apareceu sobre **Chã de Alegria**, no interior de Pernambuco, e nunca mais foi embora. Das
19:05 às 22:00 ele emite um som grave, parecido com canto gregoriano. Em 5 de junho, uma plataforma desceu da nave
trazendo uma criatura magra, sem olhos, de braços finos como agulhas: o **Precursor**. A cidade está em lockdown
militar, e o SNI o mantém num bloco de contenção. Ele "fala" por símbolos de luz no ar. O governo convoca
{um professor|uma professora} de linguística da UFPE (o jogador, em primeira pessoa) para traduzi-lo. A língua dele
não tem palavras: é estrutura pura, verdadeiro e falso. É lógica.

**Twist.** Cada tradução correta ensina a nossa língua ao Precursor. No fim da S2 ele já sabe imitar vozes: quem
fala no rádio não é mais o General Varela, é ele, empurrando o jogador para a frase que o liberta. O jogador pode
perceber: o Varela verdadeiro chama **sempre** pelo título e pelo nome ("Professora Inês"); o falso chama **só
pelo nome** ("Inês"), íntimo demais.

**Fluxo de fases:**
`MENU → INTRO → R1 → S1 → R2 → S2 → R3 → RESPOSTA_FINAL → FIM_SILENCIO | FIM_PORTA | FIM_VERDADE → (20 s) → MENU`.

---

## Bloco 3 — Especificação, lista de personagens (substitui a lista inteira)

- **{O professor|A professora}** (jogador): linguista da UFPE, civil, nunca traduziu nada vivo e nunca segurou uma
  arma; é por isso que não há combate. Gênero e nome são escolhidos no início. Não tem modelo visível.
- **General Varela**: fala só pelo rádio (legenda `GEN. VARELA (rádio)` + voz gravada). Sobreviveu ao apagão de
  15 de junho e é o único aliado do jogador. Seco, cansado, honesto, formal: chama **sempre** pelo título e
  pelo nome ("Professora Inês", "Professor João"), nunca só pelo nome.
- **O Precursor**: humanoide de ~2,6 m, magro demais, sem olhos, lâminas finas no lugar dos dedos. Move-se com
  pausas e trancos. "Parece que tenta se comunicar conosco. Ou parece que quer que acreditemos nisso." A partir do
  fim da S2, imita a voz do Varela no rádio (a voz fica mais grave, com canto ao fundo, e a legenda falha).
- **Fora de cena** (só nos textos opcionais): o americano e Dimitri, mortos no apagão de 15 de junho; Batista, que
  se escondeu com Varela e morreu depois; Gaspar, que sobreviveu traumatizado e não sai da sala do gerador.

---

## Bloco 4 — Especificação, `<roteiro_e_fases>`: INTRO e falas (substitui "INTRO (~30 s)" até o fim de "Falas entre fases")

**Marcadores nos textos.** Os textos de `src/dialogo.c` podem ter marcadores, trocados na hora de mostrar:
`{NOME}` (o nome digitado), `{PROF}` ("Professor"/"Professora"), `{prof}` ("professor"/"professora") e
`{masculino|feminino}` para qualquer outra palavra que muda com o gênero (ex.: `viv{o|a}`). O nome aparece ao
longo de toda a partida, sempre com o título (`{PROF} {NOME}`); só o rádio falso e a criatura usam o nome
sozinho. O campo do nome põe a primeira letra de cada palavra em maiúscula ("inês" vira "Inês").

**INTRO (~35 s).**

1. **Identidade** (tela preta, logo depois de INICIAR):
   - "Professor ou professora?" com as duas opções lado a lado (setas e ENTER).
   - "Qual é o seu nome, {prof}?" com um campo de texto de até 16 letras, aceitando acentos. ENTER com o campo
     vazio usa o nome padrão "Moura". No modo feira, cada pergunta segue sozinha depois de ~20 s sem resposta.
   - *Ajuste em relação ao roteiro discutido:* as perguntas vêm **antes** do texto datilografado, porque a linha 6
     já usa a palavra "professor/professora".
2. **Texto datilografado** (um clique de máquina de escrever por letra; ENTER pula):
   1. "CHÃ DE ALEGRIA, PERNAMBUCO — AGOSTO DE 1985."
   2. "Há dois meses, algo desceu da nave que paira sobre a cidade."
   3. "O Exército isolou a cidade. Toda noite, das 19:05 às 22:00, a nave canta."
   4. "O que desceu é alto, quase humano, e fala por luzes. O SNI o mantém preso."
   5. "Da última vez que as luzes caíram, dois homens morreram."
   6. "Você é {o|a} {PROF} {NOME}, de linguística da UFPE. Nunca traduziu nada vivo."
3. **A carta** (fade para a sala; rádio, General Varela):
   - "Olá, {PROF} {NOME}. General Varela. Bem-vind{o|a} ao Programa Precursor."
   - "Seu trabalho: entender o que ele diz. E nada de política, religião... muito menos futebol."
   - "Ele aprende rápido. Rápido demais. Vai tentar te enganar."
   - "Uma regra só: nunca deixe as luzes se apagarem. Estou no rádio. Qualquer coisa, grite."

**Rodadas:** sem mudança (o mesmo terminal ANALISADOR SEMÂNTICO SNI-85, as mesmas frases e opções).

**Falas entre fases.** (F) = o Varela falso: chama só pelo nome, sem o título; a voz do rádio fica mais grave,
com canto ao fundo, e a legenda falha.

- Após R1: "'Nós não somos perigo.' ...É o que todos dizem, {PROF} {NOME}." Apagão → S1.
- Início de S1: "As luzes caíram, {PROF} {NOME}. O gerador fica no fim do corredor. Vá. E não corra: ele ouve."
- Fim de S1: "Luzes de volta, {PROF} {NOME}. Ele está na cela, como se nunca tivesse saído. Volte."
- Após R2: "'Você e nós falamos'... {PROF} {NOME}, ele está aprendendo a nossa língua." Apagão maior → S2.
- Início de S2: "De novo, {PROF} {NOME}. O painel da contenção fica no Arquivo. Eu vou descer até a cela. Se eu
  não voltar..."
  (chiado; o rádio cai).
- Fim de S2 (F): "Contenção estável, {NOME}. Pode voltar."
- Após R3 (F): "{NOME}, ele quer conversar. Transmita: se você falar, a porta abre. Confie em mim." O Precursor
  encosta a mão no vidro e o terminal mostra **RESPONDA.**
- Porta trancada: "A porta fica trancada enquanto você trabalha, {PROF} {NOME}. Volte ao console."
- Vidro rompido: "Vidro substituído. Recomece a leitura, {PROF} {NOME}. Com mais cuidado."
- Pego: "...{PROF} {NOME}? Responda! — Ele voltou ao começo do corredor. Tente de novo."
- No alto do terminal: "{OPERADOR|OPERADORA}: {PROF} {NOME}".
- Depois que o Varela desceu (S2, R3 e resposta final), as mesmas situações são do Varela falso (F):
  - Pego (F): "...{NOME}. Você voltou ao começo do corredor. Tente de novo. Eu espero."
  - Vidro rompido (F): "Vidro substituído, {NOME}. Recomece. Com cuidado: eu preciso de você."
  - Porta trancada (F): "A porta fica trancada, {NOME}. Fique comigo."

---

## Bloco 5 — Especificação, `Saídas`: o item "Diário (N)" (substitui o item inteiro)

**Textos opcionais (E para ler; no máximo 3, curtos).** Só o bilhete importa para o final bom; os outros contam a
história para quem quiser.

- **S1, recorte de jornal** (perto do gerador):
  > "REDE BOLA, 10/06/85 — O ALIENÍGENA DESCEU DA NAVE ESPACIAL. Chã de Alegria segue isolada pelo Exército. O
  > governo nega que haja feridos no bloco militar."
- **S2, bilhete do Varela** (no Arquivo, no caminho do painel 2, com luz de destaque). Substitui o diário da
  Dra. Sampaio:
  > "Ele aprende as nossas vozes. Eu nunca te chamo só pelo nome: se o rádio te chamar assim, não sou eu. Ele só age
  > sobre o que PODE ser verdade. Dê a ele uma frase que nunca possa ser. Algo como: você fala e você não fala
  > (P ∧ ¬P). — V."

  Lido o bilhete, o terminal passa a mostrar no canto:
  `ANOTAÇÃO RECUPERADA: "uma frase que nunca possa ser verdade"`.
- **Volta da S2, página do diário do Varela** (na sala de observação):
  > "15/06/85 — As luzes caíram. O americano foi ver o gerador e não voltou. Dimitri enfrentou a coisa no
  > corredor; eu e o Batista nos escondemos nos armários. Quando religuei o gerador, ela estava atrás de mim. O
  > tempo todo. O Gaspar sobreviveu, mas não sai mais da sala do gerador. Não deixe as luzes acabarem."

---

## Bloco 6 — Especificação, resposta final e finais (substitui "Resposta final", "FIM RUIM" e "FIM BOM")

**Resposta final.** As mesmas fichas. Ao transmitir:

- Frase inválida → "SINTAXE INVÁLIDA" (sem penalidade).
- Contradição (ex.: `P E NÃO P`) → **FINAL 1 DE 3, O SILÊNCIO** (vitória).
- Tautologia (ex.: `P OU NÃO P`) → **FINAL 3 DE 3, A VERDADE** (final secreto).
- Qualquer outra frase satisfazível → **FINAL 2 DE 3, A PORTA** (derrota).

Cada final diz claramente o que aconteceu com o jogador e mostra "FINAL n DE 3". Logo abaixo, um parágrafo menor
explica **por que** aconteceu, com a frase transmitida em notação lógica (`%s`), para quem perdeu saber o que tentar e
quem ganhou saber o que fez:

- Silêncio: "POR QUE VOCÊ VENCEU: a sua frase (%s) é uma contradição. Ela é falsa em todas as linhas da
  tabela-verdade. O Precursor só age sobre o que PODE ser verdade: sem nenhuma brecha, ele travou."
- Porta: "POR QUE VOCÊ PERDEU: a sua frase (%s) pode ser verdadeira em pelo menos uma linha da tabela-verdade, e
  essa brecha bastou para ele agir. O rádio que te chamava só pelo nome não era o Varela. Da próxima vez, leia o
  bilhete no Arquivo e transmita uma frase que nunca possa ser verdade."
- Verdade: "POR QUE ELE LEVOU VOCÊ: a sua frase (%s) é uma tautologia. Ela é verdadeira em todas as linhas da
  tabela-verdade: você não deu uma brecha a ele, deu todas. Para calá-lo, a frase precisa ser falsa em todas as
  linhas."

**FINAL 1 DE 3, O SILÊNCIO.** Os símbolos se quebram, o Precursor congela e o canto para (2 s de silêncio total).
O rádio volta com a voz normal: "...{PROF} {NOME}? É o Varela. O de verdade. O que você disse a ele?" Jogador (texto):
"Nada que pudesse ser verdade." Tela: "FINAL 1 DE 3 — O SILÊNCIO. Você calou o Precursor e saiu viv{o|a} de Chã de
Alegria. Às 22:00, a nave foi embora. Oficialmente, nada desceu dela."

**FINAL 2 DE 3, A PORTA.** "TRANSMISSÃO ACEITA." Os símbolos do vidro se reorganizam em português legível:
"OBRIGADO, {NOME}." (só o nome, sem o título, como o rádio falso). O vidro se dissolve, as luzes morrem, a silhueta fica
a 1 m do jogador e vem o jumpscare final. Tela: "FINAL 2 DE 3 — A PORTA. Você abriu a porta para ele. Você nunca
saiu do bloco. Em agosto de 1985, todas as TVs de Pernambuco exibiram o mesmo símbolo."

**FINAL 3 DE 3, A VERDADE (secreto).** Ele atravessa o vidro devagar, sem quebrá-lo, e para na frente do jogador;
a luz azul da nave invade a sala. Tela: "FINAL 3 DE 3 — A VERDADE. Você deu a ele uma verdade que nunca falha. Ele
levou você. {PROF} {NOME} nunca foi encontrad{o|a}."

**Tela de fim:** VITÓRIA (Silêncio), DERROTA (Porta) ou **LEVAD{O|A}** (Verdade), placar total, placar por fase,
tempo total, erros, "Pressione ENTER" e retorno automático ao menu em 20 s. O +200 de final bom vale só para o
Silêncio.

---

## Cânone da história (consulta da equipe; não vai inteiro para o jogo)

Resumo do documento "HISTÓRIA", com as inconsistências já resolvidas:

- **Maio de 1985:** dois objetos sobrevoam Chã de Alegria (~120 testemunhas: "um inseto voador da altura de uma
  casa de dois andares", luz neon). Uma semana depois a FAB confirma quatro objetos sobre São Paulo e, no mesmo dia,
  sobre Recife ("barulhos de anjos"). Três semanas depois, TVs desligadas da tomada mostram símbolos estranhos em
  Casa Amarela. O **SNI** (no texto original, "AIB") começa a agir em silêncio e fabrica uma justificativa.
- **Junho:** a nave fica parada sobre Chã de Alegria. Das 19:05 às 22:00, o canto. Lockdown; o Oficial José
  Pereira do Carmo controla quem entra e sai. O presidente Sarney recusa a ajuda estrangeira.
- **5 de junho:** um facho de luz na praça por 8 min 32 s; a plataforma desce com o Precursor.
- **10 de junho:** manchete da Rede Bola. Soldados que o transportaram aparecem mortos; os corpos vão para São
  Paulo em segredo.
- **15 de junho, o apagão:** as luzes caem e ele atravessa o vidro. **Morrem o americano e Dimitri.** Varela e
  **Batista** se escondem nos armários. **Gaspar** sobrevive, traumatizado, e não sai mais da sala do gerador.
  Religado o gerador, ele aparece na cela sem sangue nas lâminas.
- **Depois:** **Batista** morre (por isso foi enterrado junto com Dimitri; a versão oficial é um acidente de
  carro).
- **Agosto:** chamam um especialista em linguagens: o jogador. Varela escreve a carta de boas-vindas e o guia
  pelo rádio.

**Ponto para a equipe confirmar:** no documento, o narrador é o "líder da equipe de máquinas", que conserta o
gerador com as próprias mãos; no jogo, ele é o **General** Varela. Uma saída simples: um general engenheiro do
Exército, posto à frente das máquinas do bloco porque nada funcionava.
