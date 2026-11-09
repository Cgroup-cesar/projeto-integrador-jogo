// test_logica.c — motor de lógica proposicional (M0: só o esqueleto).
#include "checagem.h"
#include "logica.h"

int main(void) {
    PoolLogico pool;
    pool.usados = 5;
    logica_pool_limpa(&pool);
    CHECK(pool.usados == 0);
    return resultado_final("test_logica");
}
