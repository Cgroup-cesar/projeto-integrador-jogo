# Créditos e licenças dos assets gráficos

## Texturas e materiais

Todos os materiais do mapa são **gerados em código, em C**, no carregamento do jogo:
- `src/materiais.c` e `src/materiais_parede.c` têm as funções puras;
- `src/mapas.c` monta os mapas de cor, normal e ORM;
- `src/texturas.c` sobe os mapas para a GPU.

São eles o azulejo, a tinta descascando, a laje do teto, o piso, o metal, a porta e os decalques de água,
poeira, umidade e arrasto. Nenhuma textura de terceiros foi baixada nem embutida.

| Asset | Fonte | Licença |
|---|---|---|
| Todos os materiais e decalques procedurais | código do projeto | a do projeto |
| `assets/texturas/sangue.png` (decalque de sangue) | já existia no repositório antes deste trabalho | registrada pela equipe (não alterada aqui) |
| Modelos `assets/modelos/*.glb` | já existiam no repositório | registrados pela equipe (não alterados aqui) |

## Referências visuais (não fazem parte do jogo)

As imagens em `referencias/` são **só direção de arte**:
- `corredor_hospital.png`
- `parede_reboco_quebrado.jpg`
- `parede_concreto_rachado.jpg`

O jogo não carrega nenhuma delas e nenhuma foi copiada para textura. A principal (`corredor_hospital.png`) tem
marca d'água de banco de imagens (pngtree), ou seja, é uma imagem licenciada. Antes de versionar essas imagens
num repositório público, confirme a licença com quem as trouxe, ou mantenha-as fora do git.

## Se alguém trocar um material por textura externa

O jogo aceita `assets/texturas/<material>.png`, mais `_normal.png` e `_orm.png` (veja `docs/graficos.md`).
Use apenas texturas de licença livre, como CC0 do [ambientCG](https://ambientcg.com) ou do
[Poly Haven](https://polyhaven.com), e registre aqui o nome, a URL de origem e a licença de cada uma.

## Modelos da cena do final bom (Sketchfab)

| Asset | Autor e obra | Licença |
|---|---|---|
| `assets/modelos/base.glb` | *Small Military base*, por **Jungle Jim** (Sketchfab), sem alteração | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| `assets/modelos/arvores.glb` | *Low Poly Tree Scene Free*, por **Nicholas-3D** (Sketchfab), sem alteração (o jogo usa só duas árvores e alguns tufos de capim) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
