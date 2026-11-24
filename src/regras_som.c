// regras_som.c — o som no espaço como função pura (declarada em regras.h). Não inclui raylib.h.
// A raylib não posiciona som em 3D: o volume e o lado (pan) de cada som saem daqui.
#include "regras.h"
#include <math.h>

// [REQ:PURA-regras_som_espacial] volume em decibéis pela distância (curva), mais baixo atrás e do outro lado de
// uma parede; pan pelo lado em relação a onde a pessoa olha (−1 esquerda, 1 direita)
SomEspacial regras_som_espacial(Vector3 ouvinte, float yaw, Vector3 fonte, float alcance,
                                bool atras_da_parede) {
    Vector3 para = {fonte.x - ouvinte.x, 0.0f, fonte.z - ouvinte.z};
    float dist = Vector3Length(para);
    if (dist >= alcance) return (SomEspacial){0.0f, 0.0f};
    if (dist < 0.001f) return (SomEspacial){1.0f, 0.0f}; // em cima da pessoa: cheio e no centro
    // A queda é em dB (como o ouvido mede) e curva: pouca nos primeiros metros, para ele perto soar perto,
    // e cada vez mais forte até sumir no alcance.
    float longe = Clamp((dist - SOM_PERTO_M) / (alcance - SOM_PERTO_M), 0.0f, 1.0f);
    float db = -SOM_QUEDA_DB * powf(longe, SOM_CURVA);
    Vector3 dir = Vector3Scale(para, 1.0f / dist);
    Vector3 frente = regras_frente(yaw);
    Vector3 direita = {-frente.z, 0.0f, frente.x}; // frente × cima, a mesma de regras_nova_posicao
    float atras = fmaxf(0.0f, -Vector3DotProduct(dir, frente)); // 0 na frente e dos lados, 1 bem atrás
    db -= SOM_ATRAS_DB * atras + (atras_da_parede ? SOM_PAREDE_DB : 0.0f);
    // A menos de 1 m o lado vai para o centro (nem tudo num ouvido só); nunca 100% de um lado.
    float lado = Vector3DotProduct(dir, direita) * SOM_PAN_MAX * fminf(1.0f, dist);
    return (SomEspacial){powf(10.0f, db / 20.0f), lado};
}
