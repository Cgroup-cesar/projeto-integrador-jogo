# Decisão de API gráfica e plano de migração

## Hardware desta máquina (detectado em 24/11/2026)

| Item | Valor |
|---|---|
| GPU | Intel Arc Graphics (integrada, Meteor Lake, Xe-LPG) |
| Driver | 32.0.101.8132 |
| APIs que o driver oferece | OpenGL 4.6, Vulkan 1.3, Direct3D 12 Ultimate (com DXR 1.1) |
| O que o jogo usa hoje | OpenGL 3.3 core (a raylib 6.2 pede 3.3; o log mostra `Version: 3.3.0 - Build 32.0.101.8132`) |
| CPU | Intel Core Ultra 5 125H (14 núcleos) |
| RAM | 15,4 GB (a GPU divide a memória com a CPU) |

## Linha de base medida

`bin/precursor --mudo --fase S1 --olhar 18.6,11,1.5708,-0.05 --medir 5`. A janela tem 1280 × 720 e a cena
é renderizada a 60%, ou seja, 768 × 432. A medição roda sem vsync nem trava de FPS e descarta os primeiros
60 quadros:

| Fase | Média | Pior 1% |
|---|---|---|
| R1 (sala) | 5,86 ms (171 FPS) | 11,8 ms |
| S1 (corredor norte) | 5,42 ms (184 FPS) | 11,1 ms |

Para a meta de 60 FPS no preset Alto sobram uns 11 ms por quadro.

## Como o renderizador funciona hoje

- A raylib desenha em modo imediato (`rlgl`): as paredes e o chão são caixas com UV de mundo
  (`texturas_caixa`), os decalques são quadros e os modelos são glTF.
- Há um shader só, de luz (`posfx.c`), em GLSL 330 e 100:
  - até 5 luzes pontuais, uma delas a lanterna como cone;
  - normal tirada das derivadas da posição, difusa Lambert e névoa exponencial escura;
  - a névoa da cela em *raymarch* analítico;
  - desde o item 122, o relevo tirado da textura e o reflexo especular só no chão molhado.
- As texturas são procedurais, mas só de cor (albedo). Não há mapa de normal, de rugosidade nem de AO.
- A cena vai para um render target LDR (8 bits) de 60% da janela, ampliado com filtro bilinear e passado pelo
  shader do VHS (`vhs.c`).
- Não há buffer de profundidade legível nem HDR. Também não há sombras, reflexos (fora o brilho do item 122),
  oclusão ambiente nem bloom.
- Só entram as **3 lâmpadas mais próximas**, então a fila de luminárias da referência não aparece.

### Limites do visual atual em relação à referência

1. **Materiais:** sem PBR. Azulejo, poça e tinta têm o mesmo tipo de luz, sem rugosidade nem metal.
2. **Reflexos:** só existe um brilho especular pontual. O chão não reflete as luminárias nem o feixe da
   lanterna.
3. **Luz:** 5 luzes no máximo e nenhuma sombra. A lanterna atravessa as paredes no sentido da luz, porque
   não há oclusão.
4. **Escala:** 60% de resolução, ampliado com filtro bilinear. O resultado fica borrado e com serrilhado, e
   não há antisserrilhado na cena.
5. **Cor:** a imagem é LDR (8 bits) e satura cedo. Sem HDR nem tone mapping, os brilhos não estouram como na
   referência.
6. **Profundidade:** a névoa só escurece, enquanto na referência ela clareia o fundo. Não há volumetria no
   feixe.

## A decisão: **raylib + OpenGL 3.3 core**, com o pipeline reescrito por cima

O renderizador vai ser reescrito: novo pipeline, novos shaders e um novo sistema de materiais. A janela, a
entrada, o áudio, as fontes e os modelos glTF continuam com a raylib, sobre OpenGL 3.3 core. Os motivos:

1. **Regra do projeto e cobrança da disciplina.**
   - O `CLAUDE.md` traz como regra inviolável "C99 + raylib; nenhuma outra dependência".
   - O README mapeia requisitos avaliados para funções da raylib: RF1 (`GetFrameTime`), RF4
     (`CheckCollisionBoxes`, `GetRayCollisionBox`) e outros.
   - Trocar a API gráfica por Vulkan ou D3D12 significa tirar a raylib do desenho, o que quebra essa regra e
     abre risco na avaliação.
2. **A equipe também roda no macOS.** O `CLAUDE.md` cita o `caffeinate` do macOS. No Mac, o OpenGL para na
   4.1 e não existe Vulkan nativo nem D3D12. Com o OpenGL 3.3, o mesmo código roda em Windows, Linux e macOS.
3. **O visual da referência cabe na rasterização.** O piso é **um plano só** (y = 0), então uma reflexão
   planar (desenhar a cena espelhada numa textura) dá exatamente o mesmo espelho que o ray tracing daria
   sobre esse plano, com rugosidade, distorção e Fresnel. Num corredor fechado, as sombras por *shadow map*,
   a oclusão ambiente em espaço de tela e a névoa volumétrica em *raymarch* chegam muito perto do que o RT
   entregaria.
4. **Ray tracing por hardware nesta GPU.** A Xe-LPG tem unidades de RT, mas é uma GPU integrada.
   - Para usar RT, seria preciso escrever um renderizador Vulkan ou D3D12 do zero: carregar glTF, desenhar
     texto e interface, gerenciar memória, sincronizar e montar as estruturas de aceleração. Estimo **8 a 15
     mil linhas novas de C**. É mais do que o jogo inteiro hoje (cerca de 12 mil linhas).
   - O ganho seria só nos reflexos fora do plano do chão e na oclusão. Neste mapa, os dois são pequenos.
   - Por isso, o RT por hardware **fica fora**. Se a equipe quiser, ele pode entrar depois atrás da mesma
     camada `render_*`, sem mexer na jogabilidade.

A raylib fica com o contexto, a janela, as texturas, os shaders e os FBOs, via `rlgl`. Os recursos do
OpenGL 3.3 que vão ser usados por ela:
- render targets em ponto flutuante (RGBA16F);
- textura de profundidade;
- vários *samplers* por material;
- *mipmaps*.

O 3.3 não tem *compute shaders*, então tudo que seria compute vira um passe de *fragment shader* em tela
cheia.

## Técnicas escolhidas e o *fallback* de cada uma

| Efeito | Técnica (Alto/Ultra) | Fallback (Baixo/sem suporte) |
|---|---|---|
| Materiais | PBR metálico/rugoso (GGX, Smith, Schlick), mapas de albedo, normal e ORM (oclusão, rugosidade, altura) gerados em C | o mesmo modelo, sem o mapa de normal |
| Normal map | *cotangent frame* das derivadas da tela, sem tangentes na malha | normal da geometria |
| Luzes | até 16 pontuais/spot por quadro: as luminárias mais próximas, a vermelha e a lanterna; algumas com flicker | 8 luzes |
| Sombras | *shadow map* da lanterna com PCF de Poisson (sombra suave) | sem sombra |
| Reflexo do chão | reflexão planar (cena espelhada a ½ da resolução) com *mipmaps* para a rugosidade, distorção pela normal e Fresnel | só o brilho especular e o Fresnel |
| Oclusão ambiente | SSAO no hemisfério da normal, a ½ da resolução, com desfoque bilateral | o AO gravado nas texturas |
| Névoa | volumétrica: *raymarch* do cone da lanterna e das luminárias, com poeira e a sombra da lanterna | névoa exponencial com cor (clareia o fundo) |
| HDR | cena em RGBA16F e tone mapping ACES | — |
| Bloom | cadeia de *downsample/upsample* (filtro duplo) | sem bloom |
| Antisserrilhado | FXAA sobre o resultado final, com a cena em resolução cheia no Alto | resolução reduzida com bilinear |
| Pós | gradação verde-azulada (*lift/gamma/gain*), vinheta, grão e aberração cromática, antes do VHS | — |

Ficam de fora, com o motivo:
- **TAA:** precisaria de vetores de movimento de todos os objetos, inclusive o glTF animado da criatura.
  Fica o FXAA.
- **GI em tempo real:** não há luz natural nem superfícies coloridas grandes, e a luz rebatida aparece pelo
  ambiente e pela névoa.

## A camada de abstração

O jogo não chama OpenGL direto, só os módulos de desenho. A camada nova é o `render.c/.h`, que monta o
quadro em passes:

```
render_comeca_quadro → [sombra da lanterna] → [cena espelhada] → cena HDR (materiais PBR)
  → [SSAO] → [névoa volumétrica] → [bloom] → tone mapping + gradação → FXAA → VHS → HUD
```

- Os módulos de jogo (`nivel`, `cenario`, `criatura`...) continuam desenhando a geometria do mesmo jeito.
  Só a forma de desenhar passa a pedir o material (`material_usa`).
- Jogabilidade, entrada, colisão, sanidade, objetivos e HUD não mudam.
- O HUD continua sendo desenhado por cima, como hoje.

## Plano (um commit por fase, na branch `feat/graficos-realistas`)

- **0.** Este diagnóstico e a bancada (`--olhar`, `--lanterna`, `--energia`, `--medir`).
- **a.** Pipeline HDR com profundidade, materiais PBR e geração de mapas (albedo, normal, ORM) em C.
- **b.** Azulejo na parte de baixo, tinta descascando na de cima e teto.
- **c.** Piso de ladrilho molhado, reflexão planar e decalques e detritos com normal.
- **d.** Luminárias como luzes, flicker, sombra da lanterna, SSAO e névoa volumétrica.
- **e.** Tone mapping, gradação, vinheta, grão, aberração e bloom.
- **f.** Presets Baixo/Médio/Alto/Ultra, liga e desliga por efeito, fallbacks, medição e documentação
  (`docs/graficos.md`).

Ao fim de cada fase entra uma captura do **mesmo ponto**:

```
--fase S1 --olhar 18.6,11,1.5708,-0.05 --energia
```

É o corredor norte, olhando para o leste, com 28 m de fuga. Cada captura é comparada com a referência
principal.

## Texturas: procedurais, com override por arquivo

Todos os materiais continuam **gerados em C**:
- **Regra do projeto:** nenhum asset é obrigatório; sem arquivo, o jogo gera.
- **Sem repetição:** dá para variar por peça, por parede e por altura.
- **Sem licença a rastrear.**

O override por `assets/texturas/<material>.png` continua funcionando, agora também com `_normal` e `_orm`.
Assim, alguém da equipe pode pôr um PBR CC0 (do ambientCG ou do Poly Haven) por cima sem mexer no código. Se
isso acontecer, registre a fonte e a licença em `docs/creditos_assets.md`.
