// sombreadores.h — os textos GLSL do jogo (sombreadores.c): luz e névoa (PBR) e o tone mapping.
#ifndef SOMBREADORES_H
#define SOMBREADORES_H

#include <stdbool.h>

const char *sombreador_luz_vs(bool v100); // v100: OpenGL ES 2 (fallback sem PBR)
const char *sombreador_luz_fs(bool v100);
const char *sombreador_tom_fs(void);      // só 330: sem HDR no ES 2, o tone mapping não roda
const char *sombreador_emissivo_fs(void); // só 330: luzes acesas (cor × intensidade, acima de 1)

// Os passes de tela (sombreadores_pos.c), só 330.
const char *sombreador_ssao_fs(void);
const char *sombreador_volume_fs(void);
const char *sombreador_bloom_desce_fs(void);
const char *sombreador_bloom_sobe_fs(void);
const char *sombreador_compoe_fs(void);

#endif
