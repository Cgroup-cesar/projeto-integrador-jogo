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

// ------------------------------------------------------------------ entrada do quadro
// main.c lê teclado e mouse e preenche isto; os módulos só leem daqui.
// No modo --smoke, quem preenche é o piloto automático.
typedef struct {
    Vector2 mover; // x: lado (A/D), y: frente (W/S)
    Vector2 mouse; // deslocamento do mouse no quadro
    bool correr;
    bool interagir;
    bool voltar;
    bool confirmar;
    bool cima, baixo, esquerda, direita;
    bool tecla1, tecla2, tecla3;
    bool lanterna; // F: liga e desliga a lanterna
    bool ajuda;    // H: abre e fecha a ajuda dos conectivos no terminal
    bool respirar; // ESPAÇO: segura a respiração no armário
    bool f1, f2, f3, f5, f9, f12;
    bool alguma;                   // qualquer tecla ou movimento (modo feira)
    int letras[MAX_LETRAS_QUADRO]; // letras digitadas neste quadro (Unicode), para o campo do nome
    int n_letras;
    bool apagar; // Backspace (repete se ficar apertado)
} Entrada;

// ------------------------------------------------------------------ recursos carregados uma vez
#define SOM_MAXIMO 24

// A mandíbula do precursor.glb (o modelo não tem esqueleto): os vértices do queixo e quanto cada um gira.
typedef struct {
    int n;                // vértices da mandíbula
    int *indice;          // onde cada um está na malha
    float *peso;          // 0..1: quanto acompanha o giro (suaviza a emenda com o resto do rosto)
    Vector3 *original;    // a posição sem giro
    int primeiro, ultimo; // a faixa da malha que muda (só ela vai para a GPU)
    float graus;          // giro aplicado agora (0 = o modelo original)
} BocaModelo;

typedef struct {
    Font fonte;
    bool tem_fonte; // false = fonte padrão da raylib + translitera()
    Shader vhs;
    bool vhs_ok;
    int loc_vhs[VHS_UNIFORMES]; // vhs.c: tempo, resolução e os parâmetros do preset
    Shader luz;
    bool luz_ok;
    int loc_luz_pos, loc_luz_cor, loc_luz_ambiente, loc_luz_nevoa, loc_luz_camera, loc_luz_modelo;
    int loc_luz_cone, loc_luz_contraluz, loc_luz_queda;
    RenderTexture2D alvo;       // cena 3D renderizada a ESCALA_RENDER da janela
    RenderTexture2D alvo_susto; // o rosto do susto (mesmo tamanho), colado por cima da cena
    int alvo_largura, alvo_altura;
    Sound sons[SOM_MAXIMO];
    bool audio_ok;
    float susto_s;          // duração do susto: segue o som carregado (0 = sem áudio; vale JUMPSCARE_S)
    Model modelo_precursor; // opcional: assets/modelos/precursor.glb
    bool tem_modelo;
    BocaModelo boca;       // a mandíbula animada no susto (susto_carrega_boca)
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
    Model modelo_porta; // opcional: assets/modelos/porta_bunker.glb (a porta da sala de observação)
    Mesh porta_batente, porta_folha; // as duas peças do modelo, separadas ao carregar (porta_bunker.c)
    bool tem_porta;
    Model modelo_camera; // opcional: assets/modelos/camera.glb (as câmeras de segurança)
    bool tem_cameras;
    Music trilhas[TRILHA_TOTAL]; // opcionais: sons longos de assets/audio (trilhas.c)
    bool tem_trilha[TRILHA_TOTAL];
    // Texturas do mapa (texturas.c): criadas uma vez em carrega_recursos, liberadas em descarrega_recursos.
    Texture2D material[MATERIAL_TOTAL];
    bool material_de_arquivo[MATERIAL_TOTAL]; // true = assets/texturas/<nome>.png; false = procedural
    Texture2D decalque[DECALQUE_TOTAL];
    Texture2D halo;             // brilho radial das lâmpadas
    Texture2D fita[FITA_FOTOS]; // fotos do avistamento (fita.c); opcionais: sem elas, a intro fica preta
    bool tem_fita[FITA_FOTOS];
} Recursos;

// ------------------------------------------------------------------ nível (gerado do mapa ASCII)
#define MAPA_LARGURA 24
#define MAPA_ALTURA  14
#define MAX_ARMARIOS 8

typedef struct {
    char celulas[MAPA_ALTURA][MAPA_LARGURA + 1];
    Vector3 inicio, console, cela_precursor, duto, painel1, painel2, diario, porta_sala;
    Vector3 lampadas[MAX_LAMPADAS];
    int n_lampadas;
    Vector3 armarios[MAX_ARMARIOS];      // centro do armário
    Vector3 armario_saida[MAX_ARMARIOS]; // onde o jogador fica ao sair
    int n_armarios;
    float vidro_z; // plano do vidro da contenção
    float vidro_x0, vidro_x1;
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
    int mira;          // índice do interativo na mira (−1 = nenhum)
    float tempo_passo;
    float balanco;     // balanço da câmera ao andar
    int ultima_celula; // para notar quando ele cruza uma porta
} Jogador;

// A lanterna: o jogador começa com ela; sem bateria (infinita).
typedef struct {
    bool ligada;
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
    float vel_suave; // m/s realmente andados, suavizado: dá a postura (regras_postura_criatura)
    float passada;   // metros andados desde que surgiu: dá o ritmo do balanço
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
    int numero; // 1 ou 2
    bool chaves[3];
    int escolha[2]; // painel 2: TipoPorta escolhida em cada encaixe
    int cursor;
    bool resolvido;
    float resolvido_t;
    int movimentos; // trocas de chave ou de porta (o cursor não conta): poucas = bônus
    float barulho;  // 0..1: sobe a cada clique e desce devagar (o medidor do painel)
} Painel;

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
typedef enum { FALANTE_VARELA, FALANTE_VARELA_FALSO, FALANTE_JOGADOR, FALANTE_SISTEMA } Falante;

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
} Fim;

typedef struct {
    int cursor;
    bool ranking_aberto; // a opção RANKING do menu: a tabela por cima, até ENTER ou ESC
    float troca_t;       // tempo_total da última troca de opção (menu, pausa, gênero): dispara a falha
} Menu;

// ------------------------------------------------------------------ segurar a respiração (armário)
typedef enum {
    RESP_NADA,
    RESP_TUTORIAL, // 1ª vez na partida: o mundo para e a barrinha mostra como faz
    RESP_DESAFIO,  // o Precursor parado na porta; ESPAÇO na hora certa
    RESP_ALIVIO    // 3 acertos: ele se afasta; o E só volta quando ele está longe (aí a pessoa solta o ar)
} EtapaRespiracao;

typedef struct {
    EtapaRespiracao etapa;
    float t;             // tempo na etapa (move a linha vermelha)
    float zona;          // centro da faixa branca (0..1)
    int erros;           // nesta visita ao armário
    int acertos;         // RESP_ACERTOS e ele desiste
    float acerto_t;      // "SEGUROU!" depois de um acerto
    float foi_t;         // "Ele foi embora..." (depois que ele está longe de verdade)
    bool tutorial_visto; // por partida (cada pessoa da feira vê uma vez)
    float demo_relogio;  // tutorial: o tempo da linha (para nas pausas da demonstração)
    float demo_t;        // tutorial: quanto falta da pausa "assim!" atual (0: andando)
    float demo_linha;    // onde a linha parou na demonstração
    int demo_n;          // acertos já demonstrados (a demonstração acerta RESP_ACERTOS vezes)
    float demo_ultimo;   // demo_relogio no último acerto demonstrado (a linha anda um pouco antes do próximo)
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
    float sanidade_barra;    // o que a barra mostra: desce junto, sobe suave nos bônus
    float sanidade_eco;      // o pedaço recém-perdido (mais escuro), que alcança a barra devagar
    int sanidade_conectivos; // conectivos decifrados no quadro anterior (para dar o bônus uma vez só)
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
    bool diario_lido;
    bool ajuda_aberta; // a cola dos conectivos (tecla H), no terminal das rodadas
    int vidas;         // cache do fold das rachaduras
    int placar;        // cache do fold dos eventos
    int erros_na_fase, erros_totais;
    int pegos_na_fase;
    float apagao_t;    // > 0 durante um apagão
    float falha_luz_t; // luzes piscando depois de um erro
    float alarme_t;
    float jumpscare_t;           // > 0 durante o jumpscare
    float silencio_t;            // os 0,2 s de silêncio antes do estrondo
    float retorno_t;             // saída: tempo desde que o painel foi resolvido
    EventoFase evento_pos_susto; // o que acontece quando o jumpscare termina
} Estado;

#endif
