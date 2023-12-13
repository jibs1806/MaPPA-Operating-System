#ifndef SRC_DEADLOCK_H_
#define SRC_DEADLOCK_H_

#include <diccionario_kernel.h>
#include <commons/string.h>

// FUNCIONES DE DEADLOCK
void detectarDeadlock();
t_list* obtener_recursos_con_proc_bloqueados(int cant_de_rec);
t_list* obtener_procesos_con_deadlock_de_recursos(t_list* recursos_con_procesos_en_espera);
char* obtener_lista_de_recursos(t_list* lista_recursos);
bool tiene_asignado_algun_recurso_en_deadlock(t_pcb* pcb, t_list* recursos_con_procesos_en_espera);
void destruir_lista(t_list* lista);

t_list* obtener_archivos_con_proc_bloqueados(int cant_de_archivo);
t_list* obtener_procesos_con_deadlock_de_archivos(t_list* archivos_con_procesos_en_espera);
char* obtener_lista_de_archivos_abiertos(t_list* lista_archivos_abiertos);
char* obtener_lista_de_archivos(t_list* lista_archivos);
bool tiene_asignado_algun_archivo_en_deadlock(t_pcb* pcb, t_list* archivos_con_procesos_en_espera);

// FIN FUNCIONES DE DEADLOCK

#endif /* SRC_DEADLOCK_H_ */
