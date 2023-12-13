#ifndef SRC_TRANSICIONES_H_
#define SRC_TRANSICIONES_H_

#include <kernel.h>
#include <diccionario_kernel.h>


// VARIABLES GLOBALES
int indice;

// FIN VARIABLES GLOBALES

// TRANSICIONES LARGO PLAZO

void enviar_de_new_a_ready();
void enviar_de_ready_a_exec();
void enviar_de_exec_a_ready();
void enviar_de_exec_a_block();
void enviar_pcb_de_block_a_ready(t_pcb* pcb);

void enviar_de_exec_a_finalizado(char* razon);
char* obtener_nombre_estado(t_estados estado);

// FIN TRANSICIONES LARGO PLAZO


// TRANSICIONES CORTO PLAZO

void enviar_cde_a_cpu();
void evaluar_instruccion(t_instruccion* instruccion_actual);
void recibir_cde_de_cpu();

// FIN TRANSICIONES CORTO PLAZO

// ALGORITMOS DE PLANIFICACION

t_pcb* retirar_pcb_de_ready_segun_algoritmo();
t_pcb* elegir_segun_fifo();
t_pcb* elegir_segun_rr();
t_pcb* elegir_segun_prioridades();
t_pcb* pcb_con_mayor_prioridad_en_ready();

// FIN DE ALGORITMOS DE PLANIFICACION

char* obtener_elementos_cargados_en(t_queue* cola);
int esta_proceso_en_cola_bloqueados(t_pcb* pcb);

#endif /* SRC_TRANSICIONES_H_ */
