# Gráficos do PRECURSOR

Este documento explica como o jogo desenha cada quadro, como adicionar ou trocar um material, como ajustar a
qualidade e quanto custa cada efeito. A direção de arte está em `analise_referencias.md`, e a escolha de API, em
`decisao_api.md`.

O motor continua sendo a **raylib sobre OpenGL 3.3 core**, com todo o código em C99. O caminho de desenho e os
shaders foram reescritos por cima dela, e a jogabilidade não mudou.

## O quadro, passe a passe

```
jogo.c: desenha_mundo_3d
  1. luzes do quadro         nivel_luzes (até 7 luminárias + luz no jogador) + lanterna  → efeitos_guarda_luzes
  2. sombra da lanterna      efeitos_comeca_sombra … efeitos_termina_sombra  (mapa R32 de distâncias)
  3. reflexo do chão         posfx_comeca_reflexo … posfx_termina_reflexo    (cena espelhada em y = 0, HDR)
  4. cena HDR                BeginMode3D → efeitos_guarda_camera → posfx_comeca_luz → nivel, cenário,
                             criatura, lanterna (shader PBR) → emissivos (luminárias, partículas) → vidro
posfx_resolve → efeitos_compoe
  5. SSAO                    ½ resolução, da profundidade da cena
  6. névoa volumétrica       raymarch, a volume_escala da cena (luminárias + feixe da lanterna com a sombra)
  7. bloom                   5 níveis, só o que passa de BLOOM_CORTE
  8. composição              cena × AO, + névoa, + bloom → exposição → ACES → gradação → vinheta, grão,
                             aberração → sRGB, no quadro LDR
  9. por cima do quadro      ripas do armário, susto, escurecimentos (sem tone mapping), depois VHS e HUD
```

Os módulos envolvidos:

| Módulo | O que faz |
|---|---|
| `posfx.c` | Alvos de render (HDR com profundidade, reflexo, quadro LDR), o shader de luz e seus uniformes, os materiais nas unidades fixas, o tone mapping simples (fallback) e os emissivos |
| `efeitos.c` | A sombra da lanterna e os passes de tela (SSAO, névoa, bloom, composição) |
| `sombreadores.c` | Os textos GLSL: luz PBR (330) e o fallback 100, tone mapping, emissivo |
| `sombreadores_pos.c` | Os textos GLSL dos passes de tela |
| `mapas.c` | Gera os mapas de cada material (cor, normal, ORM) a partir das funções puras |
| `materiais.c`, `materiais_parede.c` | As funções puras dos materiais (testadas em `test_regras`) |
| `qualidade.c` | Os presets e o liga e desliga de cada efeito |
| `nivel.c`, `cenario.c`, `entulho.c`, `goteira.c` | A geometria, que pede o material antes de desenhar |

### O shader de luz (PBR)

O modelo é **metálico/rugoso**: GGX com Smith-Schlick e Fresnel de Schlick, em espaço linear (as texturas
sRGB são convertidas), com saída HDR.
- **Normal map sem tangentes na malha:** o *cotangent frame* tira a base da textura das derivadas da posição
  e da UV. Por isso as caixas em modo imediato da raylib aceitam normal map sem mudança.
- **Até `LUZES_SHADER` (12) luzes** pontuais ou de cone. As que contribuem muito pouco são puladas.
- **Água no chão (`agua`):** manchas em coordenadas de mundo, que não repetem com a textura, mais a água
  parada no rejunte. A água escurece o piso, deixa liso e aplaina a normal.
- **Reflexo planar (`reflexo`):** só no chão. Borra pela rugosidade (*mipmaps*), entorta pela normal e segue o
  Fresnel com rugosidade.
- **Sombra da lanterna:** lida no mapa de distâncias, com PCF de 8 amostras em disco de Poisson.
- **Modos especiais:**
  - `modoSombra`: o mesmo shader grava só a distância, então os modelos glTF entram no passe sem trocar de
    shader;
  - `direto`: faz o tone mapping no próprio shader. Serve para o rosto do susto, que vai para um alvo LDR
    próprio, e para o caminho sem HDR.

### Unidades de textura fixas

A raylib usa a unidade 0 e, dentro do lote, as unidades 1 a 4. O jogo liga os mapas nas unidades livres e
manda o lote pendente (`rlDrawRenderBatchActive`) antes de cada troca.

| Unidade | Mapa |
|---|---|
| 9 | normal do material |
| 10 | ORM do material: R = oclusão, G = rugosidade, B = metal |
| 11 | reflexo do chão |
| 12 | sombra da lanterna |

**Cuidado:** um alvo de render nunca pode estar ligado como textura enquanto é desenhado. Se estiver, o driver
sincroniza e o quadro trava. Por isso os passes de reflexo e de sombra desligam a sua unidade antes de abrir o
alvo.

## Materiais

| Material | Abordagem | Por quê |
|---|---|---|
| Azulejo (`MATERIAL_AZULEJO`, metade de baixo das paredes) | Procedural (`materiais_azulejo`) | Cada peça tem tom, inclinação e brilho próprios, e algumas estão trincadas, lascadas ou faltando, sem repetir à vista |
| Tinta descascando (`MATERIAL_CONCRETO`, metade de cima, pilares, verga, mureta) | Procedural (`materiais_tinta`): FBM com o domínio entortado, mais Voronoi nas lascas da borda | A máscara de descascado, a borda levantada e a sombra dela, o tijolo nos buracos fundos, as rachaduras ramificadas e os escorridos saem de uma função só, coerentes entre si |
| Laje do teto (`MATERIAL_TETO`) | Procedural (`materiais_laje`) | O mesmo descascado, mais escuro, com manchas de infiltração |
| Piso (`MATERIAL_PISO`) | Procedural (ladrilho de 50 cm) + água no shader | As placas trincadas e lascadas vêm da textura; a água vem do mundo, para não repetir com a textura |
| Metal e porta | Procedural | Riscos com o metal nu, ferrugem e almofadas rebaixadas |
| Decalques de água, poeira, umidade e arrasto | Procedural (`materiais_mancha`) | Variam por formato e semente, e o chão molhado ganha o reflexo |
| Modelos glTF (criatura, armários, caixotes...) | Os do arquivo, com rugosidade fixa | Os `.glb` atuais não trazem mapas PBR completos |

Nenhuma textura externa é obrigatória. As referências em `referencias/` não entram no jogo (veja
`creditos_assets.md`).

### Como adicionar um material

1. Acrescente o nome em `TipoMaterial` (`materiais.h`) e em `NOMES` (`texturas.c`).
2. Escreva a função pura em `materiais_parede.c` ou `materiais.c`. Ela devolve um `PixelMaterial` com `brilho`,
   `mistura`, `tijolo`, `altura`, `rugosidade` e `metal`, e o caso dela entra em `materiais_pixel`.
3. Em `mapas.c`, defina:
   - as cores em `cores()`;
   - os metros que a textura cobre em `mapas_metros()`;
   - a profundidade do relevo em `profundidade()`.
4. Em `texturas.c`, defina o modo de UV em `texturas_modo()`.
5. Desenhe com `texturas_caixa_mat(e, MATERIAL_X, ...)`, para uma peça solta. Para muitas peças, chame
   `posfx_material(e, MATERIAL_X)` uma vez, desenhe com `texturas_caixa` e termine com `posfx_superficie(...)`.
6. O teste `testa_materiais` já cobre o material novo: brilho médio entre 0,7 e 1,15 e misturas em [0, 1].

**Override por arquivo:** `assets/texturas/<nome>.png` substitui a cor. `<nome>_normal.png` e `<nome>_orm.png`
substituem a normal e o ORM; sem eles, a superfície fica lisa. Assim dá para pôr por cima um PBR CC0 (do
ambientCG ou do Poly Haven) sem mexer no código. Se fizer isso, registre a fonte em `creditos_assets.md`.

## Qualidade

### Presets

Escolha com `--qualidade baixa|media|alta|ultra`. O padrão é o **alto**. Durante o jogo, F7 troca o preset; vale
só com `TECLAS_DEBUG` e fora do modo feira.

| Preset | Escala da cena | Reflexo | Sombra | SSAO | Névoa (passos) | Bloom | Mapas |
|---|---|---|---|---|---|---|---|
| Baixo | 0,6 | — | — | — | — | — | — |
| Médio | 0,75 | 0,35 | 512 px | sim | 10 (a 0,35) | sim | sim |
| Alto | 1,0 | 0,5 | 1024 px | sim | 16 (a 0,5) | sim | sim |
| Ultra | 1,0 | 0,75 | 2048 px | sim | 32 (a 0,5) | sim | sim |

### Desligar um efeito

`--sem ssao,volume,bloom,sombra,reflexo,mapas,pos` desliga os efeitos listados em qualquer preset. `pos`
corresponde à gradação, à vinheta, ao grão e à aberração.

As constantes de ajuste ficam em `config.h`:
- `EXPOSICAO`;
- `GRADACAO_*`: sombras, luzes, saturação, contraste, elevação dos pretos, vinheta, grão e aberração;
- `NEVOA_VOLUME_*`;
- `BLOOM_*`;
- `LUMINARIA_*`;
- `AGUA_NO_CHAO`.

### Custo medido

Medido na Intel Arc Graphics (integrada, Core Ultra 5 125H, driver 32.0.101.8132), com a janela em
1920 × 1080, no ponto de comparação do corredor norte (energia religada, lanterna acesa), sem vsync, por 8 s.

| Preset | Média | FPS | Pior 1% |
|---|---|---|---|
| Baixo | 5,7 ms | 175 | 11,1 ms |
| Médio | 8,8 ms | 113 | 14,3 ms |
| **Alto** | **13,3 ms** | **75** | **23 ms** |
| Ultra | 16,8 ms | 60 | 33 ms |

Antes do upgrade, a média era de 5,4 ms nesta mesma bancada, em 1280 × 720 e com a cena a 60%.

O Alto cumpre a meta de 60 FPS em 1080p com folga, e o Ultra fica no limite dela. Em gameplay, o Alto
continua acima de 60, mas há quedas pontuais do pior 1% para cerca de 40 FPS.

Como medir:

```
bin/precursor --mudo --fase S1 --olhar 18.6,11,1.5708,-0.05 --energia --lanterna --tamanho 1920x1080 --qualidade alta --medir 8
```

Para ver o custo de um efeito, repita a medição com `--sem <efeito>`. Nesta máquina, no Alto em 1080p (média
com tudo ligado: 13,4 ms):

| Efeito desligado | Média | O efeito custa |
|---|---|---|
| névoa volumétrica | 10,9 ms | 2,5 ms |
| reflexo do chão | 11,8 ms | 1,7 ms |
| sombra da lanterna | 12,7 ms | 0,8 ms |
| mapas de normal e ORM | 12,6 ms | 0,8 ms |
| bloom | 12,8 ms | 0,6 ms |
| SSAO | 12,9 ms | 0,5 ms |
| gradação, vinheta, grão e aberração | 13,1 ms | 0,3 ms |
| todos os efeitos acima, menos os mapas | 7,6 ms | (a cena PBR em resolução cheia, sozinha) |

### O que ficou de fora, e por quê

- **Ray tracing por hardware.** Exigiria um renderizador Vulkan ou D3D12 próprio, estimado em 8 a 15 mil
  linhas, e quebraria a regra "C99 + raylib" do projeto (veja `decisao_api.md`). O reflexo planar dá o espelho
  exato do chão, que é o reflexo que mais aparece.
- **TAA e upscaling temporal.** Precisariam de vetores de movimento de todos os objetos, inclusive do glTF
  animado. O antisserrilhado vem da resolução cheia no Alto, e o MSAA da janela não pega no alvo de render.
- **GI em tempo real.** A luz rebatida aparece pelo ambiente e pela névoa. Não há luz natural nem superfícies
  coloridas grandes que justifiquem o custo.

## Comparação com a referência

As capturas lado a lado de cada fase estão em `docs/img/graficos/`, com o arquivo `fase<X>_comparacao.png`.
Todas usam o mesmo ponto:

```
--fase S1 --olhar 18.6,11,1.5708,-0.05 --energia --tamanho 1920x1080, aos 24 s
```

O que ficou perto: a cor e a atmosfera verde-azulada fria, o azulejo embaixo, a tinta descascando em placas em
cima, o teto descascado, a fila de luminárias frias, o chão molhado refletindo as luminárias, a vinheta e o
contraste.

O que ainda difere depende do **mapa**, não do renderizador. A referência tem portas de madeira com janelas
gradeadas ao longo do corredor e uma janela clara no fim. O nosso corredor norte tem paredes contínuas e
termina numa parede, e mudar isso mexe na planta, nas colisões e na IA, ou seja, na jogabilidade.
