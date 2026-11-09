// test_fluxo.c — a partida inteira só com regras_proxima_fase e eventos (M0: só o esqueleto).
#include "checagem.h"
#include "regras.h"
#include <string.h>

int main(void) {
    CHECK(strcmp(regras_nome_evento(EV_PEGO), "PEGO") == 0);
    return resultado_final("test_fluxo");
}
