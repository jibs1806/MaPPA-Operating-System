#include <utils/conexiones.h>


int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family,
	                    server_info->ai_socktype,
	                    server_info->ai_protocol);

	int val = 1;
	setsockopt(socket_cliente, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1){
		return -1;
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}


int inicializar_cliente(char* ip, int puerto, t_log* logger)
{	
	char* puerto_array = string_itoa(puerto);
	int socket_cliente = crear_conexion(ip, puerto_array);

	if (socket_cliente == -1) {
		log_error(logger, "No se pudo conectar al servidor %s:%s", ip, puerto_array);
		free(puerto_array);
		return -1;
	}

	log_info(logger, "Se conecto al servidor %s:%s", ip, puerto_array);
	free(puerto_array);
	return socket_cliente;
}

int crear_servidor(int puerto)
{

	int socket_servidor;

	char* puerto_recibido = string_itoa(puerto);

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;


	getaddrinfo(NULL, puerto_recibido, &hints, &servinfo);

	socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	int val = 1;
	setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	free(puerto_recibido);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
    struct sockaddr_storage cliente_addr;
    socklen_t addr_size = sizeof(cliente_addr);
	
    int socket_cliente = accept(socket_servidor, (struct sockaddr *)&cliente_addr, &addr_size);
    if(socket_cliente == -1) {
        return -1;
    }
    return socket_cliente;
}

// TERMINAR PROGRAMA
void terminar_conexiones(int num_sockets, ...) {
  va_list args;
  va_start(args, num_sockets);

  for (int i = 0; i < num_sockets; i++) {
    int socket_fd = va_arg(args, int);
    close(socket_fd);
  }

  va_end(args);
}

void terminar_programa(t_log* logger, t_config* config){
	/* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	  con las funciones de las commons y del TP mencionadas en el enunciado */

	if (logger) {
		log_destroy(logger);
	}

	if (config){
		config_destroy(config);
	}
}


// ENVIOS Y RECIBOS
void enviar_buffer(t_buffer* buffer, int socket){
    // Enviamos el tamanio del buffer
    send(socket, &(buffer->size), sizeof(uint32_t), 0);

	if (buffer->size != 0){
    	// Enviamos el stream del buffer
    	send(socket, buffer->stream, buffer->size, 0);
	}
}

t_buffer* recibir_buffer(int socket){

	t_buffer* buffer = crear_buffer_nuestro();

	// Recibo el tamanio del buffer y reservo espacio en memoria
	recv(socket, &(buffer -> size), sizeof(uint32_t), MSG_WAITALL);

	if(buffer->size != 0){
		buffer -> stream = malloc(buffer -> size);

		// Recibo stream del buffer
		recv(socket, buffer -> stream, buffer -> size, MSG_WAITALL);
	}
	return buffer;
}


// Sirve para enviar cualquier enum ya que toma uint8_t, que seria el "nativo" de los enum
void enviar_codigo(int socket_receptor, uint8_t codigo){
	send(socket_receptor, &codigo, sizeof(uint8_t), 0);
}


// Sirve para recibir cualquier enum ya que toma uint8_t, que seria el "nativo" de los enum
uint8_t recibir_codigo(int socket_emisor){
	uint8_t codigo;

	recv(socket_emisor, &codigo, sizeof(uint8_t), MSG_WAITALL);

	return codigo;
}

// FIN ENVIOS Y RECIBOS
