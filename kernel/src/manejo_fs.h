#ifndef SRC_MANEJO_FS_H_
#define SRC_MANEJO_FS_H_

#include <diccionario_kernel.h>
#include <commons/string.h>
#include <utils/conexiones.h>
#include <transiciones.h>

// FUNCIONES DE FILE-SYSTEM
void ejecutar_f_open(char* nombre_archivo, char* modo_apertura, t_pcb* pcb);
void ejecutar_f_close(char* nombre_archivo, t_pcb* pcb_f_close);
void ejecutar_f_truncate(void*  args);
void ejecutar_f_seek(char* nombre_archivo, uint32_t nueva_posicion);
void ejecutar_f_read(char* nombre_archivo, uint32_t direccion_fisica, uint32_t numero_pagina_para_read);
void ejecutar_f_write(char* nombre_archivo, uint32_t direccion_fisica, uint32_t numero_pagina_para_write);

// FIN FUNCIONES DE FILE-SYSTEM

// FUNCIONES DE ARCHIVOS FS
t_archivo_abierto* crear_archivo_abierto(t_archivo* archivo);
t_archivo* crear_archivo_local(char* nombre_archivo);
void crear_archivo(char* nombre_archivo);
void abrir_archivo(char* nombre_archivo, char* modo_apertura, t_pcb* pcb);
bool esta_abierto(char* nombre_archivo);
t_archivo* encontrar_archivo(char* nombre_archivo, t_list* lista_archivo);
t_archivo_abierto* encontrar_archivo_abierto(char* nombre_archivo, t_list* lista_archivo);

// FIN FUNCIONES DE ARCHIVOS FS

#endif /* SRC_MANEJO_FS_H_ */