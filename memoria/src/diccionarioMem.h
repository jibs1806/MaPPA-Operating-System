#ifndef SRC_DICCIONARIOMEM_H_
#define SRC_DICCIONARIOMEM_H_


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <semaphore.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/string.h>

// DECLARACION DE STRUCTS ---------------------------------------------------------------
typedef struct{
	int puerto_escucha;
	char* ip_file_system;
	int puerto_file_system;
	int tam_memoria;
	int tam_pagina;
	char* path_instrucciones;
	int retardo_respuesta;
	char* algoritmo_reemplazo;
}t_config_memoria;

typedef struct{
	uint32_t pid;
	t_list* instrucciones;
	uint32_t cantMaxMarcos;
}t_proceso;

typedef struct{
	uint32_t pidCreador; // Para logs
	uint32_t pidEnUso; // Para logs

	uint32_t nroMarco; // De tabla
	uint32_t nroPagina; // Para kernel/cpu
	bool bitModificado; // De tabla
	bool bitPresencia; // De tabla
	char* ultimaReferencia; // Ultima vez que se referencio la pagina (para el LRU)

	// Direccion Fisica en memoria principal (ram) en la que empieza la pagina
	void* direccionInicio;

	// Dir logica en FS
	uint32_t posSwap; // De tabla
}t_pagina;

// FIN DE DECLARACION DE STRUCTS --------------------------------------------------------

// VARIABLES
t_log* logger_memoria;
t_config* config;
t_config_memoria config_memoria;

int cantMarcos;
int indicePorFifo;

t_list* listaMarcos; // lista de t_pagina* y NULLs
t_list* tablaGlobalPaginas; // lista de t_pagina*
t_list* listaGlobalProceso; // lista de t_proceso*
void* memoriaPrincipal;

// Semaforos
sem_t no_end_memoria;

// Sockets
int socket_cpu;
int socket_file_system;
int socket_kernel;

pthread_mutex_t mutex_lista_global_procesos;

sem_t finalizacion;
sem_t sem_pagina_cargada;

// FIN VARIABLES


#endif /* SRC_DICCIONARIOMEM_H_ */
