#ifndef SRC_MANEJO_BLOQUES_H_
#define SRC_MANEJO_BLOQUES_H_

#include <diccionarioFile.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

void levantar_archivo_bloques();

uint32_t obtener_nro_bloque_libre();
void escribir_en_bitmap(uint32_t nroBloque, uint8_t valor);
uint8_t leer_de_bitmap(uint32_t nroBloque);
void escribir_pagina_en_swap(uint32_t posSwap, void* paginaAEscribir);
void* leer_pagina_en_swap(uint32_t posSwap);

void escribir_en_archivo_bloques(t_config* fcb, void* bloqueAEscribir, uint32_t puntero);
void* leer_en_archivo_bloques(t_config* fcb, uint32_t puntero);
void recorrer_tabla_fat(t_config* fcb, uint32_t puntero, uint32_t* nroBloqueBuscado, uint32_t* offsetDentroDelBloque);

void levantar_tabla_fat();
void escribir_en_tabla_fat(uint32_t nroAEscribir, uint32_t nroDeBloque);
uint32_t leer_de_tabla_fat(uint32_t nroDeBloque);
void agregar_bloque_archivo_fat(uint32_t bloqueInicial);
void sacar_bloque_archivo_fat(uint32_t bloqueInicial);

#endif /* SRC_MANEJO_BLOQUES_H_ */