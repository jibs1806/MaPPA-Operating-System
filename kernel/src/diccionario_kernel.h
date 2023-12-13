#ifndef SRC_DICCIONARIO_KERNEL_H_
#define SRC_DICCIONARIO_KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <utils/serializacion.h>
#include <commons/temporal.h>

typedef struct{
	char* ip_memoria;
	int puerto_memoria;
	char* ip_file_system;
	int puerto_file_system;
	char* ip_cpu;
	int puerto_cpu_dispatch;
	int puerto_cpu_interrupt;
	char* algoritmo;
	int quantum;
	t_list* recursos; // lista de t_recurso*
	int grado_max_multiprogramacion;
}t_config_kernel;

// Estados
typedef enum{
	NULO,
	NEW, 
	READY,
	EXEC,
	BLOCKED,
	FINISH
}t_estados;

typedef struct{
    char* nombre;
    int instancias;
	t_list* procesos_bloqueados;
    sem_t sem_recurso;
}t_recurso;

//Estructura PCB
typedef struct{
	t_cde* cde;
	t_estados estado;
	char* path;
	int prioridad;
	t_list* archivos_abiertos;
	t_list* archivos_solicitados;
	t_list* recursos_asignados;
	t_list* recursos_solicitados;
	bool flag_clock;
	bool fin_q;
}t_pcb;

// ARCHIVOS
typedef struct{
	char* nombre;
	uint32_t tamanio;
	sem_t lock_escritura; // si esta en 1 es que hay un procesos escribiendo
	sem_t lock_lectura; // si su valor es > 0 es que hay x cantidad de procesos leyendo
	t_queue* procesos_bloqueados_para_lectura;
	t_queue* procesos_bloqueados_para_escritura;
	t_queue* participantes_del_lock;
	t_queue* procesos_bloqueados; // aca van todos los procesos, indistintamente de si quiere leer o escribir
	sem_t sem_archivo;
}t_archivo;

// ESTRUCTURA ARCHIVO ABIERTO
typedef struct{
	t_archivo* archivo;
	uint32_t puntero_archivo;
}t_archivo_abierto;

// ESTRUCTURA PROCESO SLEEPEADO
typedef struct{
	t_pcb* pcb_a_dormir;
	uint32_t tiempo_del_sleep;
}t_pcb_durmiendo;

// MODO APERTURA ARCHIVO
typedef enum{
	LECTURA,
	ESCRITURA
}t_modo_apertura;

t_log* logger_kernel;
t_config* config;
t_config_kernel config_kernel;

t_pcb* pcb_en_ejecucion;

// LISTAS Y COLAS

t_list* procesos_globales;
t_list* tablaArchivosAbiertos;
t_list* listaArchivosGlobales;

t_queue* procesosNew;
t_queue* procesosReady;
t_queue* procesosBloqueados;
t_queue* procesosFinalizados;
t_list* procesosBloqueadosPageFault;

// FIN LISTAS Y COLAS

// VARIABLES GLOBALES
int pid_a_asignar;
int planificacion_detenida;
int tengo_que_liberar_archivos;

// FIN VARIABLES GLOBALES

// SEMAFOROS
pthread_mutex_t mutex_procesos_globales;
pthread_mutex_t mutex_pagefault;

pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_block;
pthread_mutex_t mutex_finalizados;
pthread_mutex_t mutex_exec;

pthread_mutex_t mutex_indice;

pthread_mutex_t mutex_archivos_globales;
pthread_mutex_t mutex_archivos_abiertos;

pthread_mutex_t mutex_pcb_en_ejecucion;

sem_t sem_iniciar_quantum;
sem_t sem_reloj_destruido;
sem_t no_end_kernel;
sem_t grado_de_multiprogramacion;
sem_t procesos_en_new;
sem_t procesos_en_ready;
sem_t procesos_en_blocked;
sem_t procesos_en_exit;
sem_t cpu_libre;
sem_t cont_exec;
sem_t bin_recibir_cde;
sem_t sem_liberar_archivos;

sem_t pausar_new_a_ready;
sem_t pausar_ready_a_exec;
sem_t pausar_exec_a_finalizado;
sem_t pausar_exec_a_ready;
sem_t pausar_exec_a_blocked;
sem_t pausar_blocked_a_ready;

// FIN SEMAFOROS

// Sockets
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_memoria;
int socket_file_system;

#endif /* SRC_DICCIONARIO_KERNEL_H_ */