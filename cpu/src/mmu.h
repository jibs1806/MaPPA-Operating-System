#ifndef SRC_MMU_H_
#define SRC_MMU_H_

#include <cpu.h>

int obtener_numero_pagina(int direccion_logica);
int obtener_desplazamiento_pagina(int direccion_logica);
uint32_t calcular_direccion_fisica(int direccion_logica, t_cde* cde);

#endif /* SRC_MMU_H_ */
