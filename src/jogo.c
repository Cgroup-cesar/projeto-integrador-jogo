// jogo.c — o fluxo da partida: aplica a máquina de fases (pura) e despacha atualiza/desenha por fase.
#include "jogo.h"
#include "nivel.h"
#include "jogador.h"
#include "criatura.h"
#include "animacao.h"
#include "susto.h"
#include "lanterna.h"
#include "rodadas.h"
#include "terminal.h"
#include "circuito.h"
#include "compositor.h"
#include "glifos.h"
#include "posfx.h"
#include "audio_sintese.h"
#include "dialogo.h"
#include "identidade.h"
#include "teclas.h"
#include "ui.h"
#include "cenario.h"
#include "respiracao.h"
#include "sanidade.h"
#include "clique.h"
#include "goteira.h"
#include "efeitos.h"
#include <string.h>

// ================================================================== início e fim
static void limpa_partida(Estado *e) {
    e->eventos.n = 0;
    e->rachaduras.n = 0;
    e->erros_totais = 0;
    e->tempo_partida = 0.0f;
    e->diario_lido = false;
    e->ajuda_aberta = false;
    e->comandos_abertos = false;
    memset(e->conectivo_decifrado, 0, sizeof(e->conectivo_decifrado));
}

static void pontua(Estado *e, TipoPonto tipo, int pontos, Fase fase) {
    col_eventos_adiciona(&e->eventos, (EventoPlacar){tipo, pontos, fase});
}

// Pontos que dependem de como a fase terminou (o placar em si é um fold dos eventos).
static void pontua_evento(Estado *e, Fase fase, EventoFase ev) {
    if (ev == EV_RODADA_CONCLUIDA && e->erros_na_fase == 0)
        pontua(e, PONTO_RODADA_SEM_ERRO, PONTOS_RODADA_SEM_ERRO, fase);
    if (ev == EV_SAIDA_CONCLUIDA && e->pegos_na_fase == 0)
        pontua(e, PONTO_SAIDA_LIMPA, PONTOS_SAIDA_SEM_SER_PEGO, fase);
    int pensou = ev == EV_SAIDA_CONCLUIDA ? regras_bonus_movimentos(e->painel.movimentos) : 0;
    if (pensou > 0) pontua(e, PONTO_PAINEL_POUCOS_MOV, pensou, fase);
    bool final = ev == EV_TRANSMITIU_CONTRADICAO || ev == EV_TRANSMITIU_SATISFAZIVEL;
    if (!final) return;
    Fase destino = regras_proxima_fase(fase, ev);
    if (ev == EV_TRANSMITIU_CONTRADICAO) pontua(e, PONTO_FINAL_BOM, PONTOS_FINAL_BOM, destino);
    int bonus = regras_bonus_tempo(e->tempo_partida);
    if (bonus > 0) pontua(e, PONTO_BONUS_TEMPO, bonus, destino);
}

// [REQ:RF5] placar = fold dos eventos; vidas = integridade do vidro (fold das rachaduras).
// Recalculados a cada quadro, para o HUD e o terminal mostrarem o erro na hora.
static void atualiza_folds(Estado *e) {
    e->vidas = regras_integridade_vidro(e->rachaduras.itens, e->rachaduras.n, VIDAS_VIDRO);
    e->placar = regras_placar(e->eventos.itens, e->eventos.n);
}

// ================================================================== entrada nas fases
static void prepara_mundo(Estado *e, Fase fase) {
    e->ruidos.n = 0;
    e->particulas.n = 0;
    nivel_constroi_paredes(&e->nivel, &e->paredes);
    if (!regras_fase_eh_saida(fase)) nivel_tranca_porta(&e->nivel, &e->paredes);
    nivel_constroi_interativos(&e->nivel, &e->interativos);
    col_interativos_filtra_fase(&e->interativos, fase);
    e->jog.modo = MODO_ANDANDO;
    e->jog.folego = JOGADOR_FOLEGO_MAX;
    e->jog.exausto = false;
    e->jumpscare_t = e->silencio_t = e->apagao_t = e->falha_luz_t = e->alarme_t = e->retorno_t = 0.0f;
    e->legendas.n = 0;
}

static void entra_rodada(Estado *e, Fase fase, bool reinicio) {
    e->rachaduras.n = 0; // vidro restaurado
    rodadas_inicia(e, regras_numero_rodada(fase));
    criatura_contem(e);
    if (fase != FASE_R1) e->cri.avanco_vidro = 1.0f; // depois da R1 ele ficou mais perto do vidro
    jogador_teleporta(e, e->nivel.inicio, PI);
    if (reinicio) ui_fala(e, FALAS_RODADA_RECOMECA);
}

static void entra_saida(Estado *e, Fase fase, bool reinicio) {
    circuito_inicia(e, fase == FASE_S1 ? 1 : 2);
    criatura_inicia_saida(e);
    jogador_teleporta(e, e->nivel.inicio, PI * 0.5f); // olhando para a porta da sala, a leste (+x)
    ui_fala(e, reinicio ? FALAS_PEGO : fase == FASE_S1 ? FALAS_INICIO_S1 : FALAS_INICIO_S2);
    // A porta da sala abre (e faz barulho); as luzes ficam vermelhas e fracas até a energia voltar.
    col_ruidos_adiciona(&e->ruidos, (Ruido){e->nivel.porta_sala, RUIDO_RAIO_PORTA, 1.0f, RUIDO_PORTA});
    audio_toca(e, SOM_INTERRUPTOR, 1.0f, 0.5f);
}

static void entra_menu(Estado *e) {
    limpa_partida(e);
    criatura_contem(e);
    jogador_teleporta(e, e->nivel.inicio, PI);
    e->menu.cursor = 0;
    e->sanidade = e->sanidade_barra = e->sanidade_eco = SANIDADE_MAX;
    e->sanidade_surto = 0.0f;
}

static void entra_resposta_final(Estado *e) {
    compositor_inicia(e);
    criatura_contem(e);
    e->rod.luz_a = e->rod.luz_b = true; // as duas luzes da cela acesas: ele está ali, encostado no vidro
    e->cri.avanco_vidro = 1.0f;
    e->cri.mao_no_vidro = true;
    jogador_vai_ao_console(e);
    e->jog.modo = MODO_TERMINAL;
}

// [REQ:NOME-SESSAO] o nome só vive durante a partida: zerado no menu (fim, pausa, inatividade) e na
// partida nova (intro); mantido nos recomeços; nas fases de jogo, vazio vira o padrão (F2, --fase, smoke)
static void aplica_regra_do_nome(Estado *e, Fase fase, bool reinicio) {
    if (reinicio) return; // vidro rompido, pego, F5: a mesma pessoa continua
    if (fase == FASE_MENU || fase == FASE_INTRO)
        identidade_reinicia(e);
    else
        identidade_garante_nome(e);
}

static void entra_fase(Estado *e, Fase fase, bool reinicio) {
    aplica_regra_do_nome(e, fase, reinicio);
    e->fase = fase;
    e->tempo_fase = 0.0f;
    e->resp.etapa = RESP_NADA; // nenhuma fase começa no meio da respiração
    if (!reinicio) {
        e->erros_na_fase = 0;
        e->pegos_na_fase = 0;
    }
    e->sanidade_longe_t = 0.0f;
    prepara_mundo(e, fase);
    switch (fase) {
    case FASE_MENU:
        entra_menu(e);
        break;
    case FASE_INTRO:
        memset(&e->intro, 0, sizeof(e->intro)); // começa em INTRO_GENERO (a identidade já foi zerada)
        jogador_teleporta(e, e->nivel.inicio, PI);
        lanterna_reinicia(e); // partida nova: começa apagada
        break;
    case FASE_R1:
    case FASE_R2:
    case FASE_R3:
        entra_rodada(e, fase, reinicio);
        break;
    case FASE_S1:
    case FASE_S2:
        entra_saida(e, fase, reinicio);
        break;
    case FASE_RESPOSTA_FINAL:
        entra_resposta_final(e);
        break;
    case FASE_FIM_RUIM:
    case FASE_FIM_BOM:
        compositor_inicia_fim(e);
        break;
    default:
        break;
    }
    atualiza_folds(e); // depois que a fase limpou o que precisava (o vidro restaurado volta a 3 vidas)
    TraceLog(LOG_INFO, "PRECURSOR: fase %s%s", regras_nome_fase(fase), reinicio ? " (recomeço)" : "");
}

void jogo_aplica_evento(Estado *e, EventoFase evento) {
    if (evento == EV_NENHUM) return;
    Fase atual = e->fase;
    pontua_evento(e, atual, evento);
    e->sanidade = regras_sanidade_bonus(e->sanidade, evento); // tarefas concluídas, recomeço, partida nova
    Fase proxima = regras_proxima_fase(atual, evento);
    bool reinicia = regras_reinicia_fase(atual, evento);
    if (proxima != atual || reinicia) entra_fase(e, proxima, reinicia);
    atualiza_folds(e);
}

void jogo_reinicia_fase(Estado *e) {
    entra_fase(e, e->fase, true);
}

void jogo_vai_para_fase(Estado *e, Fase fase) {
    if (fase > FASE_MENU && fase < FASE_TOTAL) entra_fase(e, fase, false);
}

void jogo_inicia(Estado *e) {
    nivel_carrega(&e->nivel);
    e->jog.ultima_celula = -1;
    lanterna_reinicia(e);
    entra_fase(e, FASE_MENU, false);
}

// ================================================================== intro
// Conta letras (não bytes): "Ç" ocupa 2 bytes em UTF-8 mas é uma letra só.
static int conta_letras(const char *texto) {
    int n = 0;
    for (const char *p = texto; *p; p++)
        if (((unsigned char)*p & 0xC0) != 0x80) n++;
    return n;
}

static void atualiza_intro(Estado *e, float dt) {
    Intro *it = &e->intro;
    if (it->etapa <= INTRO_NOME) {
        identidade_atualiza(e, dt);
        return;
    }
    if (it->etapa == INTRO_RADIO) {
        it->abre_t += dt;
        bool comeca = !it->radio && (it->abre_t >= INTRO_RADIO_ATRASO_S || e->in.confirmar);
        if (comeca) { // o Varela só fala com a sala aparecendo (ENTER adianta)
            it->radio = true;
            it->abre_t = fmaxf(it->abre_t, INTRO_RADIO_ATRASO_S);
            ui_fala(e, FALAS_INTRO_INTERFONE);
            return;
        }
        if (!it->radio) return;
        if (e->in.confirmar) ui_legenda_pula(e);
        if (ui_legendas_vazias(e)) e->evento = EV_INTRO_TERMINOU;
        return;
    }
    if (it->linha >= INTRO_LINHAS) { // a última frase some, um instante de preto, e a sala abre
        it->saida_t += dt;
        if (it->saida_t >= INTRO_TEXTO_SOME_S + INTRO_PRETO_S || e->in.confirmar) it->etapa = INTRO_RADIO;
        return;
    }
    it->linha_t += dt;
    it->fita_t += dt;
    char linha[256];
    dialogo_personaliza(&e->ident, dialogo_intro(it->linha), linha, sizeof(linha));
    float total = (float)conta_letras(linha);
    float antes = it->letras;
    bool sem_sinal = it->linha == FITA_LINHA_QUEDA && it->linha_t < FITA_QUEDA_S + FITA_ESTATICA_S;
    if (!sem_sinal) it->letras += dt * 30.0f; // a última linha só começa depois que a fita cai
    if ((int)it->letras != (int)antes && it->letras < total)
        audio_toca(e, SOM_MAQUINA_ESCREVER, 0.5f, 0.9f + GetRandomValue(0, 20) / 100.0f);
    if (e->in.confirmar) it->letras = it->letras < total ? total : total + 99.0f;
    if (it->letras >= total) it->espera += dt;
    float pausa = 1.1f;
    if (it->linha < FITA_LINHA_QUEDA) pausa = FITA_PAUSA_S;       // com foto, dá tempo de olhar
    if (it->linha == INTRO_LINHAS - 1) pausa = INTRO_FIM_PAUSA_S; // a frase final pesa
    if (it->espera < pausa && it->letras < total + 99.0f) return;
    it->linha++;
    it->letras = 0.0f;
    it->espera = 0.0f;
    it->linha_t = 0.0f;
}

// ================================================================== pausa, debug e modo feira
static bool jogando(const Estado *e) {
    return e->fase != FASE_MENU && e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM;
}

static void atualiza_pausa(Estado *e) {
    int antes = e->menu.cursor;
    if (e->in.cima || e->in.baixo) e->menu.cursor = 1 - e->menu.cursor;
    bool confirma = clique_escolhe(&e->in, CLIQUE_PAUSA, &e->menu.cursor) || e->in.confirmar;
    if (e->menu.cursor != antes) e->menu.troca_t = e->tempo_total; // a opção nova entra com a falha (falha.c)
    if (e->in.voltar || (confirma && e->menu.cursor == 0))
        e->pausado = false;
    else if (confirma && e->menu.cursor == 1) {
        e->pausado = false;
        entra_fase(e, FASE_MENU, false);
    }
}

static void atualiza_debug(Estado *e) {
#if TECLAS_DEBUG
    if (e->modo_feira) return;
    if (e->in.f2) e->evento = EV_PULAR_FASE;
    if (e->in.f3) e->debug_criatura = !e->debug_criatura;
    if (e->in.f5 && jogando(e)) jogo_reinicia_fase(e);
    if (e->in.f9) e->evento = EV_IR_RESPOSTA_FINAL;
    if (e->in.f12) e->pedir_captura = true;
#else
    (void)e;
#endif
}

// Conta também na intro e na pausa: quem sair dali deixa o jogo voltar ao menu (e o nome é apagado).
static void atualiza_feira(Estado *e, float dt) {
    if (!e->modo_feira || !jogando(e)) {
        e->tempo_inativo = 0.0f;
        return;
    }
    e->tempo_inativo = e->in.alguma ? 0.0f : e->tempo_inativo + dt;
    EventoFase ev = regras_evento_inatividade(e->modo_feira, e->fase, e->tempo_inativo);
    if (ev == EV_NENHUM) return;
    e->tempo_inativo = 0.0f;
    e->pausado = false;
    jogo_aplica_evento(e, ev);
}

// O cursor fica preso (e some) só andando ou escondido no armário, quando o mouse vira o olhar. Nas telas
// de interface (menu, pausa, perguntas, terminal, painel, bilhete, fim) ele aparece para clicar, e vira a
// mãozinha sobre o que é clicável (clique.c).
static void atualiza_cursor(const Estado *e) {
    bool explorando = e->jog.modo == MODO_ANDANDO || e->jog.modo == MODO_ARMARIO;
    bool na_sala = e->fase != FASE_INTRO || e->intro.etapa == INTRO_RADIO;
    bool prende = jogando(e) && na_sala && explorando && !e->pausado && !e->modo_smoke;
    if (prende && !IsCursorHidden()) DisableCursor();
    if (!prende && IsCursorHidden()) EnableCursor();
    SetMouseCursor(clique_sobre_algo(&e->in) ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

// ================================================================== atualiza
static void atualiza_fase(Estado *e, float dt) {
    switch (e->fase) {
    case FASE_INTRO:
        atualiza_intro(e, dt);
        break;
    case FASE_R1:
    case FASE_R2:
    case FASE_R3:
        terminal_atualiza(e, dt); // navegação primeiro; depois a rodada decide o ENTER
        rodadas_atualiza(e, dt);
        break;
    case FASE_S1:
    case FASE_S2:
        circuito_atualiza(e, dt);
        break;
    case FASE_RESPOSTA_FINAL:
        compositor_atualiza(e, dt);
        break;
    case FASE_FIM_RUIM:
    case FASE_FIM_BOM:
        compositor_atualiza_fim(e, dt);
        break;
    default:
        break;
    }
}

static void atualiza_temporizadores(Estado *e, float dt) {
    e->tempo_fase += dt;
    if (jogando(e) && e->fase != FASE_INTRO) e->tempo_partida += dt;
    e->apagao_t = e->apagao_t > 0.0f ? e->apagao_t - dt : 0.0f;
    e->falha_luz_t = e->falha_luz_t > 0.0f ? e->falha_luz_t - dt : 0.0f;
    e->alarme_t = e->alarme_t > 0.0f ? e->alarme_t - dt : 0.0f;
}

void jogo_atualiza(Estado *e, float dt) {
    atualiza_cursor(e);
    atualiza_debug(e);
    atualiza_feira(e, dt);
    if (e->pausado) {
        atualiza_pausa(e);
        return;
    }
    bool anda_livre = e->fase != FASE_MENU && e->fase != FASE_FIM_BOM && e->fase != FASE_FIM_RUIM &&
                      !(e->fase == FASE_INTRO && e->intro.etapa != INTRO_RADIO);
    if (e->in.pausa && anda_livre && e->jog.modo == MODO_ANDANDO) { // Esc ou Start (o B não pausa)
        e->pausado = true;
        e->menu.cursor = 0;
        return;
    }
    atualiza_temporizadores(e, dt);
    susto_atualiza(e, dt);
    if (anda_livre && !susto_ativo(e)) {
        jogador_atualiza(e, dt);
        lanterna_atualiza(e, dt);
        respiracao_atualiza(e, dt);
    }
    nivel_atualiza(e, dt);
    atualiza_fase(e, dt);
    if (!respiracao_segura_criatura(e)) criatura_atualiza(e, dt); // na porta do armário, ele só espera
    animacao_atualiza(e, dt); // a animação segue mesmo com a IA parada (ele escuta na porta)
    sanidade_atualiza(e, dt); // depois da criatura e do jogador: usa a distância deste quadro
    goteira_atualiza(e, dt);
    glifos_atualiza(e, dt);
    col_ruidos_atualiza(&e->ruidos, dt);
    col_particulas_atualiza(
        &e->particulas, dt, (Vector3){0, -0.5f, 0},
        (Vector3){MAPA_LARGURA * TAMANHO_CELULA, PE_DIREITO, MAPA_ALTURA * TAMANHO_CELULA});
    ui_atualiza(e, dt);
    teclas_atualiza(e, dt);
    jogador_atualiza_camera(e);
    atualiza_folds(e);
    EventoFase ev = e->evento;
    e->evento = EV_NENHUM;
    jogo_aplica_evento(e, ev);
}

// ================================================================== desenho
static void desenha_mundo_3d(const Estado *e) {
    Vector3 pos[LUZES_SHADER], cor[LUZES_SHADER];
    Vector4 cone[LUZES_SHADER];
    float queda[LUZES_SHADER];
    for (int i = 0; i < LUZES_SHADER; i++) // padrão MAP: as lâmpadas do cenário têm a queda comum
        queda[i] = 1.0f;
    int ids[LUZES_SHADER];
    int n = nivel_luzes(e, pos, cor, cone, ids), lanterna = -1;
    if (lanterna_luz(e, &pos[n], &cor[n], &cone[n], &queda[n])) { // a última: a lanterna (tem sombra própria)
        ids[n] = -1;
        lanterna = n++;
    }
    bool sem_energia = regras_fase_eh_saida(e->fase) && !e->painel.resolvido;
    float ambiente = LUZ_AMBIENTE * (sem_energia ? SEM_ENERGIA_AMBIENTE : 1.0f);
    if (e->apagao_t > 0.0f) ambiente = LUZ_AMBIENTE * 0.3f * (sem_energia ? SEM_ENERGIA_AMBIENTE : 1.0f);
    efeitos_guarda_luzes(pos, cor, cone, queda, ids, n, lanterna);
    Vector3 frente = {cone[lanterna >= 0 ? lanterna : 0].x, cone[lanterna >= 0 ? lanterna : 0].y,
                      cone[lanterna >= 0 ? lanterna : 0].z};
    if (lanterna >= 0 && efeitos_comeca_sombra(e, pos[lanterna], frente)) { // o mapa de sombra da lanterna
        nivel_desenha_reflexo(e); // o chão não faz sombra em nada: fica de fora
        criatura_desenha(e);
        efeitos_termina_sombra(e);
    } else {
        efeitos_sem_sombra(e);
    }
    if (posfx_comeca_reflexo(e)) { // a cena espelhada no chão (y = 0): câmera embaixo dele, olhando para cima
        Camera3D espelho = e->camera;
        espelho.position.y = -espelho.position.y;
        espelho.target.y = -espelho.target.y;
        BeginMode3D(espelho);
        posfx_comeca_luz(e, pos, cor, cone, queda, ids, n, lanterna, ambiente);
        posfx_olho(e, espelho.position);
        nivel_desenha_reflexo(e);
        criatura_desenha(e);
        posfx_termina_luz(e);
        nivel_desenha_emissivos(e);
        EndMode3D();
        posfx_termina_reflexo(e);
    }
    BeginMode3D(e->camera);
    efeitos_guarda_camera(); // projeção e vista: os passes de tela reconstroem a posição pela profundidade
    posfx_comeca_luz(e, pos, cor, cone, queda, ids, n, lanterna, ambiente);
    nivel_desenha(e);
    cenario_desenha(e);
    criatura_desenha(e);
    lanterna_desenha(e);
    posfx_termina_luz(e);
    nivel_desenha_emissivos(e);
    lanterna_desenha_brilho(e);
    circuito_desenha_3d(e);
    glifos_desenha(e);
    nivel_desenha_vidro(e);
    if (regras_fase_eh_rodada(e->fase) || e->fase == FASE_RESPOSTA_FINAL)
        rodadas_desenha(e); // rachaduras no plano do vidro (3D)
    EndMode3D();
}

void jogo_desenha_cena(const Estado *e) {
    ClearBackground(BLACK);
    bool tela_preta = e->fase == FASE_INTRO && e->intro.etapa != INTRO_RADIO;
    if (!tela_preta) desenha_mundo_3d(e);
    posfx_resolve(e); // a cena HDR vira o quadro; daqui em diante, desenho direto no quadro
    jogador_desenha(e);
    susto_desenha(e);
    if (e->fase == FASE_INTRO && e->intro.etapa == INTRO_RADIO) // a sala saindo do preto
        DrawRectangle(0, 0, e->rec.alvo_largura, e->rec.alvo_altura,
                      Fade(BLACK, regras_intro_escuro(e->intro.abre_t)));
    if (e->fase == FASE_FIM_BOM || e->fase == FASE_FIM_RUIM) compositor_desenha_fim(e);
}

void jogo_desenha_interface(const Estado *e) {
    if (e->jumpscare_t <= 0.0f) {
        terminal_desenha(e);
        compositor_desenha(e);
        circuito_desenha(e);
    }
    sanidade_desenha(e);
    identidade_desenha(e);
    respiracao_desenha(e); // a barrinha do armário (e o tutorial por cima de tudo)
    teclas_desenha(e);     // o B — Comandos do canto (e a lista), por baixo das legendas e da pausa
    ui_desenha(e);
}
