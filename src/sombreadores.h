// sombreadores.h — os textos GLSL do jogo (sombreadores.c): luz e névoa (PBR) e o tone mapping.
#ifndef SOMBREADORES_H
#define SOMBREADORES_H

#include <stdbool.h>

const char *sombreador_luz_vs(bool v100); // v100: OpenGL ES 2 (fallback sem PBR)
const char *sombreador_luz_fs(bool v100);
const char *sombreador_tom_fs(void); // só 330: sem HDR no ES 2, o tone mapping não roda

#endif
