#ifndef SRC_UTILS_INSTRUCCIONES_H_
#define SRC_UTILS_INSTRUCCIONES_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <ctype.h> // Para isDigit()
#include <stdint.h> // Para uintX_t
#include <stddef.h>



typedef enum{
	SET,
	SUM,
	SUB,
	JNZ,
	SLEEP,
	WAIT,
	SIGNAL,
	MOV_IN,
	MOV_OUT,
	F_OPEN,
	F_CLOSE,
	F_SEEK,
	F_READ,
	F_WRITE,
	F_TRUNCATE,
	EXIT,
	EXIT_POR_CONSOLA,
	NULO_INST
}codigoInstruccion;


typedef struct{
	codigoInstruccion codigo;
	char* par1;
	char* par2;
	char* par3;
}t_instruccion;

uint32_t leerEnteroParametroInstruccion(int indice, t_instruccion* instr);

void escribirCharParametroInstruccion(int indice, t_instruccion* instr, char* aEscribir);
char* leerCharParametroInstruccion(int indice, t_instruccion* instr);

int obtenerTamanioString(char* palabra);



#endif /* SRC_UTILS_INSTRUCCIONES_H_ */
