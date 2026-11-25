// ranking.c — leitura e gravação do ranking (texto, uma partida por linha) e das avaliações (CSV).
#include "ranking.h"
#include <stdio.h>
#include <time.h>

void ranking_carrega(Ranking *r) {
    r->n = 0;
    FILE *f = fopen(RANKING_ARQUIVO, "r");
    if (f == NULL) return; // primeira vez: ninguém jogou ainda
    char linha[128];
    EntradaRanking e;
    while (fgets(linha, sizeof(linha), f) != NULL) // padrão FOLD: cada linha boa entra no lugar dela
        if (regras_ranking_le_linha(linha, &e)) regras_ranking_insere(r, &e);
    fclose(f);
}

bool ranking_salva(const Ranking *r) {
    const char *temporario = RANKING_ARQUIVO ".tmp";
    FILE *f = fopen(temporario, "w");
    if (f == NULL) return false;
    bool ok = true;
    char linha[128];
    for (int i = 0; i < r->n && ok; i++) // padrão MAP: uma linha por partida, já em ordem
        ok = regras_ranking_escreve_linha(&r->itens[i], linha, sizeof(linha)) > 0 && fputs(linha, f) >= 0;
    ok = fclose(f) == 0 && ok;
    return ok && rename(temporario, RANKING_ARQUIVO) == 0;
}

bool ranking_registra_avaliacao(const int *notas, int placar, bool vitoria) {
    FILE *teste = fopen(AVALIACOES_ARQUIVO, "r");
    bool novo = teste == NULL;
    if (teste != NULL) fclose(teste);
    FILE *f = fopen(AVALIACOES_ARQUIVO, "a");
    if (f == NULL) return false;
    if (novo) fputs("data;resultado;placar;entendeu;medo;aprendeu\n", f);
    char data[32];
    time_t agora = time(NULL);
    strftime(data, sizeof(data), "%Y-%m-%d %H:%M", localtime(&agora));
    fprintf(f, "%s;%s;%d;%d;%d;%d\n", data, vitoria ? "vitoria" : "derrota", placar, notas[0], notas[1],
            notas[2]);
    return fclose(f) == 0;
}
