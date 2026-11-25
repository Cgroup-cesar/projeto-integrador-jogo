// estado.h — struct Estado: todo o estado do jogo mora aqui.
// Regra de inclusão: raylib.h vem SEMPRE antes de raymath.h (a raylib define Vector3 sem proteção).
// Por isso todo módulo impuro inclui este arquivo primeiro.
#ifndef ESTADO_H
#define ESTADO_H

#include "raylib.h"
#include "config.h"
#include "logica.h"
#include "regras.h"
#include "colecoes.h"
#include "materiais.h"
#include "regras_animacao.h"

// ------------------------------------------------------------------ entrada do quadro
// main.c lê teclado e mouse, controle.c soma o controle, e preenche isto; os módulos só leem daqui.
// No modo --smoke, quem preenche é o piloto automático.
typedef struct {
    Vector2 mover; // x: lado (A/D), y: frente (W/S)
    Vector2 mouse; // deslocamento do mouse no quadro
    Vector2 olhar; // analógico direito já com zona morta e curva (-1..1); vira giro por segundo
    bool correr;
    bool interagir;
    bool voltar; // Esc ou B: sai do terminal, do painel, do bilhete; fecha a pausa
    bool pausa;  // Esc ou Start: pausa a partida andando (o B sozinho não pausa)
    bool confirmar;
    bool cima, baixo, esquerda, direita;
    bool tecla1, tecla2, tecla3;
    bool lanterna; // F: liga e desliga a lanterna
    bool ajuda;    // H: abre e fecha a ajuda dos conectivos no terminal
    bool respirar; // ESPAÇO: segura a respiração no armário
    bool comandos; // B: abre e fecha a lista de comandos no canto da tela
    bool f1, f2, f3, f5, f7, f9, f12;
    bool alguma;                   // qualquer tecla ou movimento (modo feira)
    int letras[MAX_LETRAS_QUADRO]; // letras digitadas neste quadro (Unicode), para o campo do nome
    int n_letras;
    bool apagar; // Backspace (repete se ficar apertado)
    // O mouse nas telas de interface (clique.c): só vale com o cursor à mostra (fora do andar e do armário).
    Vector2 ponteiro;    // posição do cursor na janela
    bool ponteiro_livre; // o cursor está à mostra
    bool ponteiro_moveu; // passou por cima de uma opção: ela fica escolhida
    bool clique;         // botão esquerdo: escolhe e confirma
    bool controle;       // a última coisa mexida foi o controle: as dicas mostram os botões dele
} Entrada;

// ------------------------------------------------------------------ recursos carregados uma vez
#define SOM_MAXIMO 32 // vagas para os sons (audio_sintese.h: SOM_TOTAL não pode passar daqui)

typedef struct {
    Font fonte;
    bool tem_fonte; // false = fonte padrão da raylib + translitera()
    Shader vhs;
    bool vhs_ok;
    int loc_vhs[VHS_UNIFORMES]; // vhs.c: tempo, resolução e os parâmetros do preset
    Shader luz;
    bool luz_ok;
    int loc_luz_pos, loc_luz_cor, loc_luz_ambiente, loc_luz_nevoa, loc_luz_camera, loc_luz_modelo;
    int loc_luz_molhado, loc_luz_usa_mapas, loc_luz_rugosidade, loc_luz_metalico;
    int loc_luz_reflexo, loc_luz_agua, loc_luz_tela, loc_luz_lanterna, loc_luz_direto, loc_luz_id;
    int loc_luz_triplanar, loc_luz_vidro, loc_luz_recorte;
    int loc_luz_cone, loc_luz_contraluz, loc_luz_queda;
    int loc_luz_cela_min, loc_luz_cela_max, loc_luz_cela_densidade, loc_luz_cela_cor, loc_luz_cela_espalha;
    int loc_luz_cela_lanterna;
    RenderTexture2D hdr;        // a cena 3D em HDR (cor em ponto flutuante + profundidade); id 0 = sem HDR
    RenderTexture2D reflexo;    // a cena espelhada no chão (reflexão planar), a REFLEXO_ESCALA da cena
    Shader tom;                 // tone mapping (HDR → LDR)
    Shader emissivo;            // luzes acesas: cor × intensidade (passa de 1 na cena HDR)
    bool emissivo_ok;
    int loc_emissivo_intensidade;
    bool tom_ok;
    int loc_tom_exposicao;
    RenderTexture2D alvo;       // o quadro (LDR), a ESCALA_RENDER da janela: a cena já com tone mapping
    RenderTexture2D alvo_susto; // o rosto do susto (mesmo tamanho), colado por cima da cena
    int alvo_largura, alvo_altura;
    Sound sons[SOM_MAXIMO];
    bool audio_ok;
    float susto_s;          // duração do susto: segue o som carregado (0 = sem áudio; vale JUMPSCARE_S)
    Model modelo_precursor; // opcional: assets/modelos/precursor_papelao.glb
    bool tem_modelo;
    Model modelo_lanterna; // opcional: assets/modelos/lanterna.glb
    bool tem_lanterna;
    Model modelo_terminal; // opcional: assets/modelos/terminal.glb (o computador do console)
    bool tem_terminal;
    Model modelo_caixas; // opcional: assets/modelos/caixas.glb (as pilhas de caixotes das salas)
    bool tem_caixas;
    Model modelo_mesa, modelo_papeis; // opcionais: a mesa do bilhete e as folhas (mesa_bilhete.c)
    bool tem_mesa, tem_papeis;
    Model modelo_armario; // opcional: assets/modelos/armario.glb (os armários de esconderijo)
    bool tem_armario;
    // a porta dele separada em duas folhas ao carregar (armario_modelo.c), para abrir e fechar de verdade
    Mesh armario_folha[ARMARIO_MAX_PARTES];
    int armario_folha_lado[ARMARIO_MAX_PARTES], armario_folha_mat[ARMARIO_MAX_PARTES], armario_n_folhas;
    bool armario_eh_porta[ARMARIO_MAX_PARTES]; // a malha i do modelo é da porta (vai pelas folhas)
    Vector3 armario_dobradica[2];              // o eixo de cada folha (esquerda, direita), no modelo
    Model modelo_porta; // opcional: assets/modelos/porta_bunker.glb (a porta da sala de observação)
    Mesh porta_batente, porta_folha; // as duas peças do modelo, separadas ao carregar (porta_bunker.c)
    bool tem_porta;
    Model modelo_camera; // opcional: assets/modelos/camera.glb (as câmeras de segurança)
    bool tem_cameras;
    Model modelo_quadro; // opcional: assets/modelos/quadro.glb (o quadro de investigação)
    bool tem_quadro;
    Model modelo_base; // opcional: assets/modelos/base.glb (a base lá fora, na cena do final bom)
    bool tem_base;
    // A mata em volta da base (assets/modelos/arvores.glb, opcional): o modelo é uma cena inteira; ao carregar,
    // guardamos duas árvores (casca e folhas) e alguns tufos de capim, que são repetidos em volta da base.
    Model modelo_mata;
    bool tem_mata;
    int mata_casca[2], mata_folha[2], n_arvores; // as malhas de cada árvore
    Vector3 mata_pe[2];                         // o pé do tronco de cada uma (no modelo)
    int mata_capim[8], n_capim;
    Model modelo_corpo; // opcional: assets/modelos/corpo.glb (o corpo do americano)
    bool tem_corpo;

    Model modelo_lampada; // opcional: assets/modelos/lampada.glb (a lâmpada da sala de observação)
    bool tem_lampada;
    Vector3 lampada_pos; // onde o modelo é desenhado e a escala dele (calculados ao carregar)
    float lampada_escala;

    // O rig do Precursor (rig.c): pesos dos ossos calculados ao carregar e o skinning no shader de luz.
    bool rig_ok;                    // false = o modelo aparece sem se deformar (pose de repouso)
    unsigned int rig_vbo[2];        // ossoIds e ossoPesos (4 por vértice)
    int loc_luz_ossos[OSSOS_TOTAL]; // uniform ossos[i] do shader de luz
    int loc_luz_usa_ossos;
    float rig_escala, rig_base_y; // metros por unidade do modelo e o y dos pés (do .glb ou nominal)

    // Os sons da tubulação (tubulacao_som.c), opcionais: a tampa (dois trechos de um arquivo) e os passos.
    Sound tampa_sai, tampa_entra;
    Music passos_tubo;
    bool tem_tampa, tem_passos_tubo;

    Music trilhas[TRILHA_TOTAL]; // opcionais: sons longos de assets/audio (trilhas.c)
    bool tem_trilha[TRILHA_TOTAL];
    // Texturas do mapa (texturas.c): criadas uma vez em carrega_recursos, liberadas em descarrega_recursos.
    Texture2D material[MATERIAL_TOTAL];        // cor (albedo)
    Texture2D material_normal[MATERIAL_TOTAL]; // normal (espaço da textura: u, v, fora)
    Texture2D material_orm[MATERIAL_TOTAL];    // R = oclusão, G = rugosidade, B = metal
    bool material_de_arquivo[MATERIAL_TOTAL]; // true = assets/texturas/<nome>.png; false = procedural
    Texture2D decalque[DECALQUE_TOTAL];
    Texture2D halo;             // brilho radial das lâmpadas
    Texture2D fita[FITA_FOTOS]; // fotos do avistamento (fita.c); opcionais: sem elas, a intro fica preta
    bool tem_fita[FITA_FOTOS];
    Texture2D botao[BOTOES_TOTAL]; // ícones do controle (botoes.c); opcionais: sem eles, o botão vira texto
    bool tem_botao[BOTOES_TOTAL];
} Recursos;

// ------------------------------------------------------------------ nível (gerado do mapa ASCII)
#define MAPA_LARGURA 24
#define MAPA_ALTURA  14
#define MAX_ARMARIOS 8

typedef struct {
    char celulas[MAPA_ALTURA][MAPA_LARGURA + 1];
    Vector3 inicio, console, cela_precursor, duto, painel1, painel2, diario, porta_sala;
    Vector3 porta_arquivo; // a porta do Arquivo (a 'D' mais perto do painel 2)
    int celula_trancada;   // célula de uma porta trancada (−1 = nenhuma): a criatura não passa por ela
    Vector3 cano_cela;     // a boca da tubulação no chão da cela (por onde ele sai quando as luzes caem)
    Vector3 lampadas[MAX_LAMPADAS];
    bool lampada_em_x[MAX_LAMPADAS]; // a luminária corre ao longo de x (senão, de z)
    // bit i da célula (linha, coluna): a luminária i a alcança sem passar por parede; o bit LUZ_BIT_MESA é a
    // lâmpada da mesa do bilhete. O shader de luz lê isso numa textura (as luzes não atravessam paredes).
    unsigned short alcance_luz[MAPA_ALTURA][MAPA_LARGURA];
    int n_lampadas;
    Vector3 armarios[MAX_ARMARIOS];      // centro do armário
    Vector3 armario_saida[MAX_ARMARIOS]; // onde o jogador fica ao sair
    int n_armarios;
    float vidro_z; // plano do vidro da contenção
    float vidro_x0, vidro_x1;
    Vector3 cela_min, cela_max; // a caixa da cela, do chão ao teto e até o vidro (onde fica a névoa)
} Nivel;

// ------------------------------------------------------------------ jogador
typedef enum { MODO_ANDANDO, MODO_TERMINAL, MODO_PAINEL, MODO_DIARIO, MODO_ARMARIO } ModoJogador;

typedef struct {
    Vector3 pos; // pés
    float yaw, pitch;
    float folego;
    bool exausto;
    bool correndo;
    ModoJogador modo;
    int armario;       // índice em Nivel.armarios quando escondido
    float armario_yaw; // direção da porta: centro do cone de visão enquanto escondido
    // a animação de entrar e sair do armário (a câmera, a porta e os sons: a regra de estar escondido não
    // espera por ela)
    bool armario_anima;             // há uma entrada ou saída em andamento (até a porta fechar)
    float armario_t;                // s desde que ela começou
    bool armario_saindo;            // true: saindo; false: entrando
    Vector3 armario_de;             // de onde a câmera partiu
    float armario_yaw_de, armario_pitch_de; // para onde ela olhava ao começar a entrar
    // o passo até o console (E nele) e o passo para trás ao sair dele: o corpo anda sozinho, sem teleporte
    bool console_anda;                      // há um passo em andamento
    bool console_vira;                      // chegando: vira para o vidro junto; saindo, o olhar fica livre
    float console_t;                        // s desde que o passo começou
    Vector3 console_de, console_para;       // onde os pés estavam e aonde vão
    float console_yaw_de, console_pitch_de; // para onde ele olhava ao começar a chegar
    int mira;          // índice do interativo na mira (−1 = nenhum)
    float tempo_passo;
    float balanco;     // balanço da câmera ao andar
    int ultima_celula; // para notar quando ele cruza uma porta
} Jogador;

// A lanterna: o jogador começa com ela; sem bateria (infinita).
typedef struct {
    bool ligada;
    bool ja_acesa; // acendeu alguma vez nesta partida (até lá, o F do canto pisca em amarelo)
} Lanterna;

// ------------------------------------------------------------------ o Precursor
typedef struct {
    Vector3 pos;
    float yaw;
    bool ativo;   // nos corredores (saídas)
    bool contido; // na cela (fases de sala)
    EstadoCriatura estado;
    float tempo_estado;
    float tempo_para_surgir;
    Vector3 destino;
    Vector3 ultima_vista;
    int caminho[MAX_CAMINHO]; // células (linha * MAPA_LARGURA + coluna)
    int caminho_n, caminho_i;
    float tempo_replanejar;
    int patrulha_i;
    bool ve_jogador;
    float distorcao; // 0..1 ao ser "puxado" pela contenção
    float passo_t;
    float fala_t;       // > 0 enquanto ele "fala" (fenda aberta)
    float avanco_vidro; // 0..1: o passo em direção ao vidro depois da R1
    bool mao_no_vidro;
    float congelado_t; // final bom
    float surgindo;    // 0..1 enquanto sobe pelo duto
    float som_passo_t;
    float procura_t; // na PROCURA: troca de ponto a cada pouco

    AnimCriatura anim;    // o rig: pose, passos, tiques e pesos das camadas (animacao.c, regras_animacao.c)
    Vector3 anim_desvio;  // o desenho alcança devagar um pulo curto de posição (armário, mão no vidro)
    Vector3 anim_pos_ant; // a posição do quadro anterior (para achar esses pulos)
} Criatura;

// ------------------------------------------------------------------ rodadas (sala de observação)
typedef enum { ETAPA_DECIFRAR, ETAPA_ESCOLHER_CONECTIVO, ETAPA_TRADUZIR, ETAPA_CONCLUIDA } EtapaRodada;

typedef struct {
    int numero; // 1..3
    EtapaRodada etapa;
    bool luz_a, luz_b;
    bool linha_vista[2][4];
    bool linha_valor[2][4];
    int coluna; // na R2 há duas frases: o jogador identifica uma de cada vez
    int cursor;
    float resposta_t; // > 0 enquanto o Precursor "responde" à combinação de luzes
    int resposta_linha;
    int resposta_coluna; // na R2 ele responde às duas frases, uma depois da outra
    bool resposta_valor;
    float clarao_t; // > 0 logo depois que ele responde: o VERDADE/FALSO grande no terminal
    float aviso_t;
    const char *aviso;
    float concluida_t;
    bool rompendo; // 3º erro: jumpscare e recomeço
} Rodada;

// ------------------------------------------------------------------ painéis de circuito (saídas)
typedef struct {
    int numero; // 1 ou 2 (os dois têm as chaves A, B e C; muda o circuito)
    bool chaves[3];
    int cursor;
    bool resolvido;
    float resolvido_t;
    int movimentos; // trocas de chave ou de porta (o cursor não conta): poucas = bônus
    float barulho;  // 0..1: sobe a cada clique e desce devagar (o medidor do painel)
} Painel;

// ------------------------------------------------------------------ a porta do Arquivo e a chave (chave.c)
typedef struct {
    bool pega;         // a chave saiu do chaveiro (ela só existe na S2)
    bool porta_aberta; // a porta do Arquivo foi destrancada
    float porta_t;     // s desde que abriu: a folha desliza para dentro da parede
    int aviso;         // TextoId do aviso na tela (dialogo.h)
    float aviso_t;     // > 0 enquanto o aviso aparece
    float aviso_dur;   // quanto o aviso dura (para o fade)
} ChaveArquivo;

// ------------------------------------------------------------------ a porta da sala (porta_sala.c)
typedef struct {
    bool aberta;    // para onde a folha vai (E troca, nas saídas)
    float abertura; // 0 = fechada, 1 = aberta (regras_porta_passo)
    float duracao;  // quanto a folha leva neste movimento (a criatura abre com um tranco)
    bool solida;    // a folha fecha o vão: a PAREDE_PORTA está nas paredes de colisão
} PortaSala;

// ------------------------------------------------------------------ a volta à sala (retorno.c)
typedef struct {
    bool ativo;                  // a cena está com o jogador (o controle volta quando acaba)
    float t;                     // s desde que começou
    Fase saida;                  // de que saída ele voltou: a cena continua na rodada seguinte
    PoseJogador de, porta, sala; // as três poses da coreografia (regras_retorno_pose)
    bool fechou, trocou;         // já mandou a porta fechar; já mandou a fase trocar
} Retorno;

// ------------------------------------------------------------------ resposta final
typedef struct {
    Ficha fichas[MAX_FICHAS_FRASE];
    int n;
    int cursor;
    int acoes; // quantas fichas já foram usadas (o piloto do smoke segue um roteiro por aqui)
    const char *mensagem;
    char detalhe[128];
    float mensagem_t;
    bool transmitindo;
    float transmitindo_t;
    EventoFase resultado;
} Compositor;

// ------------------------------------------------------------------ intro, legendas e fim
// FALANTE_VARELA_FALSO: o Precursor imitando o Varela no rádio (mesmo rótulo, voz grave e legenda falhando).
// FALANTE_VARELA_PRESENTE: o Varela em pessoa, sem o rádio (lá em cima da escotilha, na cena do final bom).
typedef enum { FALANTE_VARELA, FALANTE_VARELA_FALSO, FALANTE_JOGADOR, FALANTE_SISTEMA, FALANTE_VARELA_PRESENTE } Falante;

typedef struct {
    const char *texto;
    float duracao;
    Falante falante;
} Legenda;

// Os sons longos (trilhas.c): o volume que cada um tem agora e a fala do rádio que está tocando.
typedef struct {
    float volume[TRILHA_TOTAL];
    const char *fala_no_radio; // muda a cada legenda nova do Varela: a voz recomeça de outro trecho
} Mixagem;

typedef struct {
    Legenda fila[MAX_LEGENDAS];
    int n;
    float tempo; // tempo da legenda da frente
} FilaLegendas;

// A intro em etapas: as duas perguntas, o texto datilografado e a carta do Varela pelo rádio (já na sala).
typedef enum { INTRO_GENERO, INTRO_NOME, INTRO_DATILOGRAFA, INTRO_RADIO } EtapaIntro;

typedef struct {
    EtapaIntro etapa;
    float tempo_etapa; // no modo feira, cada pergunta segue sozinha depois de IDENTIDADE_FEIRA_S
    int linha;
    float letras; // quantas letras da linha atual já apareceram
    float espera;
    float linha_t; // tempo na linha atual (a queda do sinal da fita conta daqui)
    float fita_t;  // tempo desde o começo do texto datilografado (o tremido e o relógio da câmera)
    float saida_t; // depois da última linha: o texto sumindo e o preto (INTRO_TEXTO_SOME_S + INTRO_PRETO_S)
    float abre_t;  // na etapa do rádio: a sala clareando (regras_intro_escuro)
    bool radio;    // a carta do Varela já começou (antes dela, legendas vazias não encerram a intro)
} Intro;

// Quem o jogador é: escolhido no início da partida e usado nos marcadores {NOME}, {PROF} e {x|y} das falas.
typedef struct {
    bool feminino;             // false = professor, true = professora
    char nome[NOME_MAX_BYTES]; // UTF-8, até NOME_MAX_LETRAS letras
} Identidade;

typedef struct {
    float tempo; // tempo da cinemática
    int etapa;
    float leitura;    // tempo com o texto do final na tela (espera o ENTER, até FIM_LEITURA_MAX_S)
    bool tela_final;  // a cinemática acabou: mostra o placar
    float tempo_tela; // tempo na tela atual do placar (o desempenho ou o ranking)
    int tela;         // 0: desempenho e avaliação; 1: o ranking, com a partida em destaque
    float parado_t;   // tempo sem tecla no desempenho (volta sozinho em FIM_VOLTA_MENU_S)
    bool registrado;  // a partida já entrou no ranking
    int posicao;      // no ranking (0 = 1º lugar; −1 = não coube)
    int notas[AVALIACAO_PERGUNTAS]; // estrelas que a pessoa deu ao jogo (0 = não respondeu)
    int nota_cursor;
    PoseJogador cena_de, cena_pe; // a subida do final bom (regras_cena_pose): do console ao pé da escada
} Fim;

typedef struct {
    int cursor;
    bool ranking_aberto; // a opção RANKING do menu: a tabela por cima, até ENTER ou ESC
    float troca_t;       // tempo_total da última troca de opção (menu, pausa, gênero): dispara a falha
} Menu;

// ------------------------------------------------------------------ segurar a respiração (armário)
typedef enum {
    RESP_NADA,
    RESP_DESAFIO,  // o Precursor parado na porta; ESPAÇO na hora certa
    RESP_ALIVIO    // 3 acertos: ele se afasta; o E só volta quando ele está longe (aí a pessoa solta o ar)
} EtapaRespiracao;

typedef struct {
    EtapaRespiracao etapa;
    bool perseguido;     // ele vinha atrás quando a pessoa entrou no armário (só então ele vem até a porta)
    float t;             // tempo na etapa (move a linha vermelha)
    float zona;          // centro da faixa branca (0..1)
    int erros;           // nesta visita ao armário
    int acertos;         // RESP_ACERTOS e ele desiste
    float acerto_t;      // "SEGUROU!" depois de um acerto
    float foi_t;         // "Ele foi embora..." (depois que ele está longe de verdade)
    float aviso_t;       // "ELE OUVIU!" depois de um erro
} Respiracao;

// ------------------------------------------------------------------ o estado inteiro
// [REQ:RNF2] uma struct por objeto (Jogador, Criatura, Rodada, Painel...) e um módulo .c por sistema
typedef struct Estado {
    Fase fase;
    float tempo_fase;
    float tempo_partida;
    float tempo_total;
    EventoFase evento; // pedido de mudança de fase feito neste quadro (aplicado no fim do atualiza)
    bool sair, pausado, debug_criatura, pedir_captura;
    int vhs_preset;          // filtro VHS em uso (0 = desligado; ver vhs.c e VHS_PRESETS em config.h)
    float vhs_rotulo_t;      // > 0: mostra o nome do preset (acabou de trocar)
    float sanidade;          // 0..SANIDADE_MAX: cai com a criatura perto, volta nas tarefas (sanidade.c)
    float sanidade_longe_t;  // s seguidos sem a criatura no raio (a recuperação espera um pouco)
    float sanidade_grao;     // 0..1: força do efeito na tela (segue a barra; vai ao shader como insanidade)
    float sanidade_surto;    // 0..1: sanidade zerada (falhas na visão e vinheta vermelha, no shader do VHS)
    float sanidade_dist;     // distância da criatura no quadro anterior (para saber se ela está chegando)
    float sanidade_barra;    // o que a barra mostra: desce junto, sobe suave nos bônus
    float sanidade_eco;      // o pedaço recém-perdido (mais escuro), que alcança a barra devagar
    int sanidade_conectivos; // conectivos decifrados no quadro anterior (para dar o bônus uma vez só)
    int goteira_gota;        // a última gota da goteira que já pingou (para tocar o som uma vez só)
    // câmeras de segurança (cameras.c): o quanto cada uma virou (graus, a partir de para onde aponta de
    // fábrica), o quanto inclinou além do normal, há quanto tempo perdeu o jogador e quando refaz o motor
    float camera_giro[MAX_CAMERAS], camera_inclina[MAX_CAMERAS];
    float camera_perdeu_t[MAX_CAMERAS];
    float camera_ve_t[MAX_CAMERAS];                                 // há quanto tempo está vendo o jogador
    float camera_mira_giro[MAX_CAMERAS], camera_mira_inclina[MAX_CAMERAS]; // para onde quer virar (atrasado)
    bool modo_smoke, modo_feira;
    float tempo_inativo;

    Entrada in;
    Recursos rec;
    Camera3D camera;
    Nivel nivel;
    Jogador jog;
    Lanterna lanterna;
    Criatura cri;
    Rodada rod;
    Painel painel;
    ChaveArquivo chave;
    PortaSala porta;
    Retorno retorno;
    Compositor comp;
    Intro intro;
    Identidade ident;
    Respiracao resp; // o desafio de segurar a respiração no armário (respiracao.c)
    Ranking ranking; // carregado do disco ao abrir o jogo (ranking.c); vazio no --smoke
    FilaLegendas legendas;
    Mixagem mix;
    Fim fim;
    Menu menu;

    // [REQ:RF3] as seis coleções: struct por objeto, vetor de capacidade fixa e contador (sem malloc por
    // quadro)
    ColRuidos ruidos;
    ColParticulas particulas;
    ColRachaduras rachaduras;
    ColEventos eventos;
    ColInterativos interativos;
    ColParedes paredes;

    bool conectivo_decifrado[4]; // NÃO, E, OU, SE…ENTÃO
    bool comandos_abertos;       // a lista de comandos (tecla B), no canto inferior esquerdo
    bool diario_lido;
    bool ajuda_aberta; // a cola dos conectivos (tecla H), no terminal das rodadas
    int vidas;         // cache do fold das rachaduras
    int placar;        // cache do fold dos eventos
    int erros_na_fase, erros_totais;
    int pegos_na_fase;
    bool cano;         // ele está saindo pela tubulação da cela (só apresentação: animacao.c)...
    float cano_t;      // ...há quanto tempo (desde que as luzes começaram a cair)
    float tubo_ant[2]; // cano_t e tempo_para_surgir do quadro anterior (a tampa: tubulacao_som.c)
    float apagao_t;    // > 0 durante um apagão
    float falha_luz_t; // luzes piscando depois de um erro
    float alarme_t;
    float jumpscare_t;           // > 0 durante o jumpscare
    float silencio_t;            // os 0,2 s de silêncio antes do estrondo
    EventoFase evento_pos_susto; // o que acontece quando o jumpscare termina
} Estado;

#endif
