// test_regras.c — regras puras e coleções (M0: só o esqueleto).
#include "checagem.h"
#include "regras.h"
#include <string.h>

int main(void) {
    CHECK(strcmp(regras_nome_fase(FASE_MENU), "MENU") == 0);
    CHECK(strcmp(regras_nome_fase(FASE_TOTAL), "?") == 0);
    return resultado_final("test_regras");
}
