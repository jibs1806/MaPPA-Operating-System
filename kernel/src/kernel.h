#ifndef SRC_KERNEL_H_
#define SRC_KERNEL_H_

#include <utils/conexiones.h>
#include <menu.h>
#include <transiciones.h>
#include <diccionario_kernel.h>
#include <deadlock.h>
#include <manejo_fs.h>

// SUBPROGRAMAS

void inicializarModulo(char* config_path);
void levantarLogger();
void levantarConfig(char* config_path);
t_recurso* inicializar_recurso(char* nombre_recu, int instancias_tot);
void inicializarConexiones();
void inicializarListas();
void inicializarSemaforos();

void iniciarPlanificadores();
void planificadorLargoPlazo();
void planificadorCortoPlazo();

void conectarseConCPU();
void crear_hilos_cpu();

// UTILS MENSAJES
void iniciarProceso(char* path, char* size, char* prioridad);
void finalizarProceso(char* pid);
void detenerPlanificacion();
void iniciarPlanificacion();
void procesosPorEstado();
void cambiar_grado_multiprogramacion(char* nuevo_grado);

t_pcb* crear_pcb(char* path, char* prioridad);
void destruir_pcb(t_pcb* pcb);
void terminar_proceso();
void liberar_recursos_pcb(t_pcb* pcb);
void liberar_archivos_pcb(t_pcb* pcb);
void signal_recursos_asignados_pcb(t_pcb* pcb, char* nombre_recurso_pedido);
// FIN UTILS MENSAJES

// SAFE PCB ADD
t_pcb* retirar_pcb_de(t_queue* cola, pthread_mutex_t* mutex);
void agregar_pcb_a(t_queue* procesosNew, t_pcb* pcb_a_new, pthread_mutex_t* mutex);

t_pcb* encontrar_pcb_por_pid(uint32_t pid, int* encontrado_int);
void retirar_pcb_de_su_respectivo_estado(uint32_t pid, int* resultado);

void evaluar_instruccion(t_instruccion* instruccion_actual);
void evaluar_wait(char* nombre_recurso_pedido);
void evaluar_signal(char* nombre_recurso_pedido);
void enviar_a_finalizado(t_pcb* pcb_a_finalizar, char* razon);
void sleep_pcb(void* args);
void evaluar_sleep(t_pcb* pcb_en_exec, uint32_t tiempo_sleep);
void recibir_page_fault(void* args);

// FUNCIONES DE QUANTUM (RR)
void controlar_tiempo_de_ejecucion();
void prender_quantum();

// FIN FUNCIONES DE QUANTUM (RR)

#endif /* SRC_KERNEL_H_ */
