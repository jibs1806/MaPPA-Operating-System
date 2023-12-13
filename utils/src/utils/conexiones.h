#ifndef SRC_UTILS_CONEXIONES_H_
#define SRC_UTILS_CONEXIONES_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <utils/serializacion.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>

typedef enum{
	EJECUTAR_PROCESO,
	INTERRUPT,
	DESALOJO,
	CDE,
	ALGORITMO_PLANIFICACION,
	HAY_PAGE_FAULT,
	DIRECCION_FISICA_OK
}mensajeKernelCpu;

typedef enum{
	ABRIR_ARCHIVO_SOLICITUD,
	ABRIR_ARCHIVO_OK,
	NO_EXISTE_ARCHIVO,

	CREAR_ARCHIVO_SOLICITUD,
	CREAR_ARCHIVO_OK,

	REALIZAR_LECTURA_SOLICITUD,
	REALIZAR_LECTURA_OK,

	REALIZAR_ESCRITURA_SOLICITUD,
	REALIZAR_ESCRITURA_OK,

	CAMBIAR_TAMANIO_SOLICITUD,
	CAMBIAR_TAMANIO_OK
}mensajeKernelFile;


typedef enum{
	INICIAR_PROCESO_SOLICITUD,
	INICIAR_PROCESO_OK,
	INICIAR_PROCESO_ERROR,
	PAGE_FAULT_SOLICITUD,
	PAGE_FAULT_OK,
	FINALIZAR_PROCESO_SOLICITUD,
	FINALIZAR_PROCESO_OK
}mensajeKernelMem;

typedef enum{
	MOV_IN_SOLICITUD,
	MOV_IN_OK, // Cuando no hay pageFault
	MOV_OUT_SOLICITUD,
	MOV_OUT_OK, // Cuando no hay pageFault
	PAGE_FAULT,
	NUMERO_MARCO_SOLICITUD,
	NUMERO_MARCO_OK,
	PEDIDO_INSTRUCCION
}mensajeCpuMem;

typedef enum{
	CREAR_PAGINA_SOLICITUD,
	CREAR_PAGINA_RTA,

	BUSCAR_PAGINA_SOLICITUD,
	BUSCAR_PAGINA_OK,

	ACTUALIZAR_PAGINA_SOLICITUD,
	ACTUALIZAR_PAGINA_OK,
	
	LECTURA_MEMORIA_SOLICITUD,
	LECTURA_MEMORIA_OK,

	ESCRITURA_MEMORIA_SOLICITUD,
	ESCRITURA_MEMORIA_OK,

	LIBERAR_BLOQUES_SOLICITUD
}mensajeMemoriaFile;

// SUBPROGRAMAS
int crear_conexion(char *ip, char* puerto);
int inicializar_cliente(char* ip, int puerto, t_log* logger);
int crear_servidor(int puerto);
int esperar_cliente(int socket_servidor);

void terminar_conexiones(int num_sockets, ...);
void terminar_programa(t_log* logger, t_config* config);

// FIN SUBPROGRAMAS

// ENVIOS Y RECIBOS
void enviar_buffer(t_buffer* buffer, int socket);
t_buffer* recibir_buffer(int socket);

void enviar_codigo(int socket_receptor, uint8_t codigo);
uint8_t recibir_codigo(int socket_emisor);
// FIN ENVIOS Y RECIBOS

#endif /* SRC_UTILS_CONEXIONES_H_ */
