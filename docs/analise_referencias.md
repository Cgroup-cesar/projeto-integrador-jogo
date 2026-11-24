# Análise das referências visuais

As três imagens em `referencias/` são **direção de arte**: nenhuma delas entra no jogo como textura. Esta é a
leitura de cada uma, e o que vira requisito do renderizador e dos materiais.

## 1. `corredor_hospital.png` — a principal (meta de estilo e qualidade)

**Composição.** Corredor longo com um ponto de fuga só, no centro, um pouco acima do meio da tela. O fundo
é a parte mais clara: uma janela/porta de vidro iluminada no fim e a névoa que clareia com a distância. As
bordas e os cantos são quase pretos (vinheta forte). Tudo converge para o centro: luminárias do teto em fila,
juntas do piso, a linha onde o azulejo encontra a tinta.

**Paredes.** Duas camadas bem distintas:
- *Metade de baixo* (até ~1,4 m): azulejo cerâmico quadrado (~15 cm), esmaltado e **brilhante**: cada peça
  tem um reflexo especular próprio, levemente diferente da vizinha (as peças não são coplanares). O rejunte
  é escuro e afundado. Há peças sujas, manchadas por escorridos, e algumas quebradas ou faltando. No topo, um
  arremate (uma fiada de peças mais estreita, ou um friso) separa o azulejo da tinta.
- *Metade de cima*: tinta clara descascando em **placas grandes e irregulares**, deixando ver uma camada
  escura (reboco úmido/mofo) por baixo. A borda de cada placa é clara e levantada, com uma sombra fina:
  relevo visível. As manchas escuras dominam onde a umidade é maior (perto do teto e nos cantos).
- *Teto*: o mesmo descascado, mais escuro, com manchas de umidade grandes.

**Piso.** Ladrilhos grandes (~40–50 cm) com juntas escuras. Está **molhado**: a faixa central reflete as
luminárias do teto como manchas claras alongadas, e as poças viram espelhos borrados. Onde está seco, o
piso é fosco e escuro. O brilho cresce com a distância (Fresnel: vista rasante reflete mais).

**Detritos.** Papéis, cacos de azulejo e entulho espalhados, **concentrados junto às paredes e nas portas**;
o meio do corredor (por onde se anda) está mais limpo.

**Elementos.** Portas de madeira velha, com batente escuro grosso e uma janela de vidro gradeada; algumas
entreabertas. Luminárias retangulares de teto (fluorescentes), frias, fracas, em fila; nem todas acesas.

**Cor e luz.** Quase monocromática: verde-azulado frio e dessaturado (as sombras puxam para o azul-petróleo,
as luzes para o verde-claro). Contraste alto: os pretos são pretos de verdade, e os brilhos (luminárias e
seus reflexos) estouram. Profundidade construída pela névoa e pela atenuação, não pela cor.

**O que vira requisito:** azulejo PBR com especular por peça e rejunte rebaixado; descascado com relevo
(normal) na borda; piso com máscara de umidade que baixa a rugosidade e reflete as luzes; luminárias como
luzes de verdade em fila (mais que as 3 mais próximas de hoje); névoa que clareia o fundo; vinheta e
gradação verde-azulada fria; detritos concentrados nas bordas.

## 2. `parede_reboco_quebrado.jpg` — reboco quebrado

**Superfície.** Concreto/reboco cinza-quente, muito sujo. **Escorridos pretos verticais** finos e numerosos,
de cima a baixo, com comprimentos e opacidades diferentes (água que desceu com sujeira por anos).

**Reboco caindo.** Placas de reboco soltaram e deixaram ver **pedra/tijolo** atrás, em tons ferrugem e
terracota; a borda do buraco é grossa (o reboco tem espessura) e projeta sombra. Manchas de óxido
alaranjado em volta dos buracos.

**Base.** Uma faixa mais escura perto do chão (a umidade sobe; o rodapé encardido).

**O que vira requisito:** máscara de escorridos verticais de largura e comprimento variáveis; buracos de
reboco com profundidade (height/normal) revelando tijolo/pedra; óxido em volta; base mais escura.

## 3. `parede_concreto_rachado.jpg` — concreto rachado

**Rachaduras.** Uma **rede fina e ramificada**: rachaduras principais longas e tortas, com ramos menores que
saem delas e morrem. A largura varia: grossas no meio, finas nas pontas.

**Superfície.** Rugosidade que muda de região para região (mais lisa em placas, áspera em outras); manchas
de água em vários tons de cinza, verticais, sobrepostas; reboco irregular, com pequenas lascas.

**Luz.** Vinheta forte e o centro mais claro: o mesmo tratamento de luz da principal.

**O que vira requisito:** gerador de rachaduras ramificadas (não só uma linha de ruído); rugosidade por
região; manchas de água em camadas; o relevo das rachaduras (afundadas no height).

## Resumo do que perseguir

| Aspecto | Hoje no jogo | Alvo |
|---|---|---|
| Parede de baixo | barrado de tinta oliva | azulejo esmaltado com especular por peça |
| Parede de cima | concreto com escorridos | tinta descascando com relevo, sobre reboco escuro |
| Piso | granilite fosco | ladrilho grande, molhado, com reflexo das luminárias |
| Luzes | 3 lâmpadas mais próximas + lanterna | todas as luminárias próximas, fila visível, algumas falhando |
| Profundidade | névoa exponencial escura | névoa que clareia o fundo + volumetria no feixe |
| Cor | verde fósforo + VHS | gradação verde-azulada fria, pretos fundos, brilhos estourando |
