
#ifndef SRC_CPU_H_
#define SRC_CPU_H_

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <utils/utils.h>
#include <utils/conexiones.h>
#include <utils/serializacion.h>
#include <mmu.h>


t_registro* registros_cpu;
int interruption;
int interrupcion_consola;
int realizar_desalojo;
uint32_t tamanio_pagina;

typedef struct{
    char* ip_memoria;
    int puerto_memoria;
    int puerto_escucha_dispatch;
    int puerto_escucha_interrupt;
}t_config_cpu;


// -------------- VARIABLES --------------

// VARIABLES GLOBALES

t_log* logger_cpu;
t_config* config;
t_config_cpu config_cpu;
int socket_kernel_dispatch;
int socket_kernel_interrupt;
int socket_memoria;
uint32_t algoritmo_planificacion;
uint32_t pf_con_funcion_fs;
uint32_t pid_de_cde_ejecutando;
codigoInstruccion instruccion_actualizada;

//FIN VARIABLES GLOBALES

// SEMAFOROS
pthread_mutex_t mutex_realizar_desalojo;
pthread_mutex_t mutex_interrupcion_consola;
pthread_mutex_t mutex_cde_ejecutando;
pthread_mutex_t mutex_instruccion_actualizada;

sem_t no_end_cpu;
sem_t cpu_libre;

//FIN SEMAFOROS

// ----------- FIN VARIABLES -------------

// INICIALIZACIONES
void inicializarModulo(char* config_path);
void inicializarConexiones();
void inicializar_registros();
void inicializar_semaforos();



// FIN INICIALIZACIONES

// CONEXIONES
void conectarKernel();
void levantarLogger();
void levantarConfig(char* config_path);
void conectarKernel();

void dispatchProceso(void* socket_server);
void interruptProceso(void* socket_server);

void recibir_tamanio_pagina(int socket_memoria);

// FIN CONEXIONES

// FUNCIONES AUXILIARES INSTRUCCIONES
void recibir_tamanio_pagina(int socket_memoria);
uint32_t buscar_valor_registro(void* registro);
void cargar_registros(t_cde* cde);

void guardar_cde(t_cde* cde);
void devolver_cde_a_kernel(t_cde* cde, t_instruccion* instruccion_a_ejecutar);
bool es_bloqueante(codigoInstruccion instruccion_actualizada);

// FIN FUNCIONES AUXILIARES INSTRUCCIONES

// FUNCIONES INSTRUCCIONES

void desalojar_cde(t_cde* cde, t_instruccion* instruccion_a_ejecutar);
void ejecutar_proceso(t_cde* cde);
void ejecutar_instruccion(t_cde* cde, t_instruccion* instruccion_a_ejecutar);
char* obtener_nombre_instruccion(t_instruccion* instruccion);

void ejecutar_set(char* registro, uint32_t valor);
void ejecutar_sum(char* reg_dest, char* reg_origen);
void ejecutar_sub(char* reg_dest, char* reg_origen);
void ejecutar_jnz(void* registro, uint32_t nro_instruccion, t_cde* cde);
void ejecutar_sleep(uint32_t tiempo);
void ejecutar_wait(char *recurso);
void ejecutar_signal(char *recurso);
void ejecutar_mov_in(char* registro, uint32_t dirLogica, t_cde* cde);
void ejecutar_mov_out(uint32_t dirLogica, char* registro, t_cde* cde);
void ejecutar_f_open(char* nombre_archivo, char* modo_apertura);
void ejecutar_f_close(char* nombre_archivo);
void ejecutar_f_seek(char* nombre_archivo, char* posicion);
void ejecutar_f_read(char* nombre_archivo, char* direccion_logica, t_cde* cde);
void ejecutar_f_write(char* nombre_archivo, char* direccion_logica, t_cde* cde);
void ejecutar_f_truncate(char* nombre_archivo, char* tamanio);
void ejecutar_exit();

// FIN FUNCIONES INSTRUCCIONES

#endif /* SRC_CPU_H_ */
