#include <utils/serializacion.h>

// BUFFER

t_buffer* crear_buffer_nuestro(){
	t_buffer* b = malloc(sizeof(t_buffer));
	b->size = 0;
	b->stream = NULL;
	b->offset = 0;
	return b;
}

void destruir_buffer_nuestro(t_buffer* buffer){
	free(buffer->stream);
	free(buffer);
	
	return;
}

// FIN BUFFER

// UINT32
void buffer_write_uint32(t_buffer* buffer, uint32_t entero){
	buffer->stream = realloc(buffer->stream, buffer->size + sizeof(uint32_t));

	memcpy(buffer->stream + buffer->size, &entero, sizeof(uint32_t));
	buffer->size += sizeof(uint32_t);
}

uint32_t buffer_read_uint32(t_buffer* buffer){
	uint32_t entero;

	memcpy(&entero, buffer->stream + buffer->offset, sizeof(uint32_t));
	buffer->offset += sizeof(uint32_t);

	return entero;
}

// UINT8
void buffer_write_uint8(t_buffer* buffer, uint8_t entero){
	buffer->stream = realloc(buffer->stream, buffer->size + sizeof(uint8_t));

	memcpy(buffer->stream + buffer->size, &entero, sizeof(uint8_t));
	buffer->size += sizeof(uint8_t);
}

uint8_t buffer_read_uint8(t_buffer* buffer){
	uint8_t entero;

	memcpy(&entero, buffer->stream + buffer->offset, sizeof(uint8_t));
	buffer->offset += sizeof(uint8_t);

	return entero;
}

// STRING
void buffer_write_string(t_buffer* buffer, char* cadena){
	uint32_t tam = 0;

	while(cadena[tam] != NULL)
		tam++;

	buffer_write_uint32(buffer,tam);

	buffer->stream = realloc(buffer->stream, buffer->size + tam);

	memcpy(buffer->stream + buffer->size, cadena , tam);
	buffer->size += tam;
}

char* buffer_read_string(t_buffer* buffer, uint32_t* tam){
	(*tam) = buffer_read_uint32(buffer);
	char* cadena = malloc((*tam) + 1);
	

	memcpy(cadena, buffer->stream + buffer->offset, (*tam));
	buffer->offset += (*tam);

	*(cadena + (*tam)) = '\0';

	return cadena;
}

// CONTEXTO DE EJECUCION
void buffer_write_cde(t_buffer* buffer, t_cde* cde){
	buffer_write_uint32(buffer, cde->pid);
	buffer_write_uint32(buffer, cde->pc);
	buffer_write_registros(buffer, cde->registros);
}

t_cde* buffer_read_cde(t_buffer* buffer){
	t_cde* cde = malloc(sizeof(t_cde));
	cde->pid = buffer_read_uint32(buffer);
	cde->pc = buffer_read_uint32(buffer);
	cde->registros = buffer_read_registros(buffer);
	
	return cde;
}

void destruir_cde(t_cde* cde){
    free(cde->registros);
    free(cde);
}

// INSTRUCCION
void buffer_write_instruccion(t_buffer* buffer, t_instruccion* instruccion){
	buffer_write_uint8(buffer, instruccion->codigo);
	if (instruccion->par1 != NULL)
		buffer_write_string(buffer, instruccion->par1);
	else
		buffer_write_string(buffer, "");
	
	if (instruccion->par2 != NULL)
		buffer_write_string(buffer, instruccion->par2);
	else
		buffer_write_string(buffer, "");
	
	if (instruccion->par3 != NULL)
		buffer_write_string(buffer, instruccion->par3);
	else
		buffer_write_string(buffer, "");

}

t_instruccion* buffer_read_instruccion(t_buffer* buffer){
	t_instruccion* instr = malloc(sizeof(t_instruccion));
	instr->par1 = NULL;
	instr->par2 = NULL;
	instr->par3 = NULL;
	uint32_t tam;

	instr->codigo = buffer_read_uint8(buffer);
	
	instr->par1 = buffer_read_string(buffer, &tam);
	instr->par2 = buffer_read_string(buffer, &tam);
	instr->par3 = buffer_read_string(buffer, &tam);

	return instr;
}

void destruir_instruccion(t_instruccion* instruccion){
	free(instruccion->par1);
	free(instruccion->par2);
	free(instruccion->par3);
	free(instruccion);
}


// Registros
void buffer_write_registros(t_buffer* buffer, t_registro* registros){
	buffer_write_uint32(buffer, registros->AX);
	buffer_write_uint32(buffer, registros->BX);
	buffer_write_uint32(buffer, registros->CX);
	buffer_write_uint32(buffer, registros->DX);
}

t_registro* buffer_read_registros(t_buffer* buffer){
	t_registro* regis = malloc(sizeof(t_registro));

	regis->AX =buffer_read_uint32(buffer);
	regis->BX = buffer_read_uint32(buffer);
	regis->CX = buffer_read_uint32(buffer);
	regis->DX = buffer_read_uint32(buffer);

	return regis;
}

// Pagina
void buffer_write_pagina(t_buffer* buffer, void* pagina, uint32_t tamPagina){
	buffer->stream = realloc(buffer->stream, buffer->size + tamPagina);
	memcpy(buffer->stream + buffer->size, pagina, tamPagina);
	buffer->size += tamPagina;

	char* pagAEscribirString = string_new();

	for(int i = 0 ; i < (tamPagina / 4); i++){
		uint32_t k;
		memcpy(&k, pagina + i*4, sizeof(uint32_t));
	}
}

void* buffer_read_pagina(t_buffer* buffer, uint32_t tamPagina){
	void* paginaLeida = malloc(tamPagina);

	memcpy(paginaLeida, buffer->stream + buffer->offset, tamPagina);
	buffer->offset += tamPagina;
	
	for(int i = 0 ; i < (tamPagina / 4); i++){
		uint32_t k;
		memcpy(&k, paginaLeida + i*4, sizeof(uint32_t));
	}

	return paginaLeida;
}