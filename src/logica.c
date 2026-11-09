// logica.c — motor de lógica proposicional. Não inclui raylib.h.
#include "logica.h"

void logica_pool_limpa(PoolLogico *pool) {
    pool->usados = 0;
}
