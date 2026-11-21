// vhs.h — filtro VHS de pós-processamento: presets (VHS_PRESETS em config.h), troca com F1 e rótulo.
#ifndef VHS_H
#define VHS_H

#include "estado.h"

void vhs_carrega(Recursos *rec);
void vhs_descarrega(Recursos *rec);

int vhs_total(void);                 // quantos presets existem (o 0 é "desligado")
const char *vhs_nome(int preset);    // nome do preset (de dialogo.c)
void vhs_escolhe(Estado *e, int preset); // troca de preset (dá a volta) e mostra o nome por alguns segundos

// F1 passa para o próximo preset (só com TECLAS_DEBUG e fora do modo feira); conta o tempo do rótulo.
void vhs_atualiza(Estado *e, float dt);

// Em volta do DrawTexturePro do render target: liga o shader com o preset atual. Devolve false (e não liga
// nada) se o preset for "desligado" ou se o shader não compilou.
bool vhs_comeca(const Estado *e);
void vhs_termina(const Estado *e);

// O nome do preset no canto da tela, logo depois de trocar (fica por cima da interface).
void vhs_desenha(const Estado *e);

#endif
