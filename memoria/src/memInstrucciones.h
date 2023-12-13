#ifndef SRC_MEMINSTRUCCIONES_H_
#define SRC_MEMINSTRUCCIONES_H_

#include <memoria.h>
#include <utils/instrucciones.h>
#include <diccionarioMem.h>

// UTILS INSTRUCCIONES ------------------------------------------------------------------
void ejecutarMovIn();
void ejecutarMovOut();
// FIN UTILS INSTRUCCIONES --------------------------------------------------------------

// UTILS PAGINAS ------------------------------------------------------------------------
void crearPaginaPrueba();
void devolver_nro_marco();
t_pagina* existePageFault(uint32_t nroPag, uint32_t pid);
t_pagina* buscarPaginaPorNroYPid(uint32_t nroPag, uint32_t pid);

// UTILS PROCESOS -----------------------------------------------------------------------
t_proceso* buscarProcesoPorPid(uint32_t pid);
// FIN UTILS PROCESOS -------------------------------------------------------------------

t_list* parsearArchivo(char* pathArchivo, t_log* logger);
t_instruccion* 	parsearInstruccion();
codigoInstruccion obtenerCodigoInstruccion(char* instruccion);

void atender_cpu();


#endif /* SRC_MEMINSTRUCCIONES_H_ */
