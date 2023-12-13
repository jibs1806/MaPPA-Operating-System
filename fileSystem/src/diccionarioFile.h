#ifndef SRC_DICCIONARIOFILE_H_
#define SRC_DICCIONARIOFILE_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/config.h>
#include <utils/utils.h>
#include <utils/conexiones.h>
#include <pthread.h>
#include <semaphore.h>


typedef struct{
	char* ip_memoria;
	int puerto_memoria;
	int puerto_escucha;
	char* path_fat;         
	char* path_bloques;    
	char* path_fcb;       
	int cantidad_bloques_total;
	int cantidad_bloques_swap;
	int tam_bloque;
	int retardo_acceso_bloque;
	int retardo_acceso_fat;
}t_config_file_system;


// VARIABLES
t_log* logger_file_system;
t_config* config;
t_config_file_system config_file_system;

void* bloquesMapeado;
void* tablaFat;
void* bitmapBloquesSwap;

// Sockets
int socket_memoria;
int socket_kernel;


// FIN VARIABLES

// SEMAFOROS
sem_t no_end_file_system;

// FIN SEMAFOROS

#endif /* SRC_DICCIONARIOFILE_H_ */
