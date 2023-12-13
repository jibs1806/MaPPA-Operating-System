#ifndef SRC_FILESYSTEM_H_
#define SRC_FILESYSTEM_H_

#include <diccionarioFile.h>
#include <manejo_bloques.h>

// INICIALIZAR MODULO
void inicializarModulo(char* config_path);

void inicializarVariables();
void levantar_archivo_bloques();
void levantar_tabla_fat();

void inicializarSemaforos();
void inicializarConexiones();
void conectarMemoria(int socket_servidor);
void conectarKernel(int socket_servidor);
void levantarLogger();
void levantarConfig(char* config_path);

// FIN INICIALIZAR MODULO

// UTILS MEMORIA
void atender_memoria();
void reservar_bloque_swap();
void liberar_bloque_swap();
void devolver_pagina();
void actualizar_pagina();
// FIN UTILS MEMORIA

// UTILS KERNEL
void atender_kernel();
void abrir_archivo();
void crear_archivo();
void truncar_archivo();

void escribir_archivo();
void recibir_bloque_de_memoria();
void escribir_en_archivo_bloques(t_config* fcb, void* bloqueAEscribir, uint32_t puntero);

void leer_archivo();
void enviar_confirmacion_a_kernel_de_escritura();

char* obtener_ruta_archivo(char* nombre_archivo);
t_config* crear_fcb_archivo(char* nombreArchivo);
void cambiar_tamanio_archivo(char* nombreArchivo, uint32_t nuevo_tamanio);
// FIN UTILS KERNEL

#endif /* SRC_FILESYSTEM_H_ */
