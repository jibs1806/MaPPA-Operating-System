#include <utils/instrucciones.h>

uint32_t leerEnteroParametroInstruccion(int indice, t_instruccion* instr){
	char* aux = leerCharParametroInstruccion(indice, instr);
	uint32_t leido = atoi(aux);
	free(aux);
	return leido;
}


void escribirCharParametroInstruccion(int indice, t_instruccion* instr, char* aEscribir){

	int tam = obtenerTamanioString(aEscribir) + 1;

	switch(indice){
		case 1:
			instr->par1 = malloc(tam);
			strcpy(instr->par1, aEscribir);
			string_append(&(instr->par1), "\0");
			break;
		case 2:
			instr->par2 = malloc(tam);
			strcpy(instr->par2, aEscribir);
			string_append(&(instr->par2), "\0");
			break;
		case 3:
			instr->par3 = malloc(tam);
			strcpy(instr->par3, aEscribir);
			string_append(&(instr->par3), "\0");
			break;
		}
}


char* leerCharParametroInstruccion(int indice, t_instruccion* instr){
	char* leido;

	int tam = 0;

	// Se podria delegar para no repetir tanta logica pero me parecio al pedo
	switch(indice){
	case 1:
		tam = obtenerTamanioString(instr->par1);
		leido = malloc(tam);
		strcpy(leido, instr->par1);
		break;
	case 2:
		tam = obtenerTamanioString(instr->par2);
		leido = malloc(tam);
		strcpy(leido, instr->par2);
		break;
	case 3:
		tam = obtenerTamanioString(instr->par3);
		leido = malloc(tam);
		strcpy(leido, instr->par3);
		break;
	}

	return leido;
}


int obtenerTamanioString(char* palabra){
	int tam = 0;

	while(palabra[tam] != NULL)
		tam++;

	return tam;
}
