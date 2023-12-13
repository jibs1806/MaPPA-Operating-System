#ifndef SRC_UTILS_SERIALIZACION_H_
#define SRC_UTILS_SERIALIZACION_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // Para uintX_t
#include <string.h>
#include <utils/instrucciones.h>

typedef struct{
	uint32_t size;
	uint32_t offset;
	void* stream;
} t_buffer;

//Registros
typedef struct{
	uint32_t AX;
	uint32_t BX;
	uint32_t CX;
	uint32_t DX;
}t_registro;

//CDE
typedef struct{
	uint32_t pid;
	uint32_t pc;
	t_registro* registros;
}t_cde;

// SUBPROGRAMAS
t_buffer* crear_buffer_nuestro();
void destruir_buffer_nuestro(t_buffer* buffer);

// UINT32
void buffer_write_uint32(t_buffer* buffer, uint32_t entero);
uint32_t buffer_read_uint32(t_buffer* buffer);

// UINT8
void buffer_write_uint8(t_buffer* buffer, uint8_t entero);
uint8_t buffer_read_uint8(t_buffer *buffer);

// STRING
void buffer_write_string(t_buffer* buffer, char* string_a_escribir);
char* buffer_read_string(t_buffer* buffer, uint32_t* tam);

//REGISTROS
void buffer_write_registros(t_buffer* buffer, t_registro* cde);
t_registro* buffer_read_registros(t_buffer* buffer);

// CDE
void buffer_write_cde(t_buffer* buffer, t_cde* cde);
t_cde* buffer_read_cde(t_buffer* buffer);
void destruir_cde(t_cde* cde);

// INSTRUCCIONES
t_instruccion* buffer_read_instruccion(t_buffer* buffer);
void buffer_write_instruccion(t_buffer* buffer, t_instruccion* instruccion);
void destruir_instruccion(t_instruccion* instruccion);


//Pagina
void buffer_write_pagina(t_buffer* buffer, void* pagina, uint32_t tamPagina);
void* buffer_read_pagina(t_buffer* buffer, uint32_t tamPagina);

#endif /* SRC_UTILS_SERIALIZACION_H_ */
