#include <memoria.h>

int main(int argc, char* argv[]) {

	inicializarModulo("./memoria.config");

	sem_wait(&no_end_memoria);

    terminar_conexiones(2, socket_kernel, socket_file_system);
    terminar_programa(logger_memoria, config);
	
    return 0;
}


// INICIALIZAR MODULO -------------------------------------------------------------------
void inicializarModulo(char* config_path){
    levantarLogger();
    levantarConfig(config_path);
	
	sem_init(&no_end_memoria, 0, 0);
    pthread_mutex_init(&mutex_lista_global_procesos, NULL);

	inicializarVariables();
    inicializarConexiones();

	listaMarcos = list_create();
	cantMarcos = config_memoria.tam_memoria / config_memoria.tam_pagina;
	
	for(int i = 0; i < cantMarcos; i++)
		list_add(listaMarcos, NULL);
		
    return;
}

void levantarLogger(){
    logger_memoria = iniciar_logger("./memoria.log", "MEMORIA");
}

void levantarConfig(char* config_path){
    config = iniciar_config(config_path);

    if (config == NULL){
        log_info(logger_memoria, "Error al levantarConfig");
        exit(1);
    }
    config_memoria.puerto_escucha = config_get_int_value(config,"PUERTO_ESCUCHA");
    config_memoria.ip_file_system = config_get_string_value(config,"IP_FILESYSTEM");
    config_memoria.puerto_file_system = config_get_int_value(config, "PUERTO_FILESYSTEM");
    config_memoria.tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
    config_memoria.tam_pagina = config_get_int_value(config,"TAM_PAGINA");
    config_memoria.path_instrucciones = config_get_string_value(config,"PATH_INSTRUCCIONES");
    config_memoria.retardo_respuesta = config_get_int_value(config,"RETARDO_RESPUESTA");
    config_memoria.algoritmo_reemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
}

void inicializarVariables(){
	listaGlobalProceso = list_create();
	tablaGlobalPaginas = list_create();

	sem_init(&finalizacion, 0, 0);
	sem_init(&sem_pagina_cargada, 0, 0);

	memoriaPrincipal = malloc(config_memoria.tam_memoria);
	
	indicePorFifo = 0;
}
// FIN INICIALIZAR MODULO ---------------------------------------------------------------

// CONEXIONES ---------------------------------------------------------------------------
void inicializarConexiones(){
    int socket_servidor = crear_servidor(config_memoria.puerto_escucha);

	conectarFileSystem();
    conectarCpu(socket_servidor);
    conectarKernel(socket_servidor); //crear servidor y esperar cliente (kernel)
}

void conectarKernel(int socket_servidor){

    log_info(logger_memoria, "Esperando Kernel....");
    socket_kernel = esperar_cliente(socket_servidor);
    log_info(logger_memoria, "Se conecto el Kernel");

	pthread_t hilo_kernel;
	pthread_create(&hilo_kernel, NULL, (void *) atender_kernel, NULL);
	pthread_detach(hilo_kernel);
}

void conectarCpu(int socket_servidor){

    log_info(logger_memoria, "Esperando Cpu....");
    socket_cpu = esperar_cliente(socket_servidor);
    log_info(logger_memoria, "Se conecto Cpu");
    
	enviar_tamanio_pagina();

	pthread_t hilo_cpu;
	pthread_create(&hilo_cpu, NULL, (void *) atender_cpu, NULL);
	pthread_detach(hilo_cpu);
}

void conectarFileSystem(){
    socket_file_system = inicializar_cliente(config_memoria.ip_file_system, config_memoria.puerto_file_system, logger_memoria);

	pthread_t hilo_file_system;
	pthread_create(&hilo_file_system, NULL, (void *) atender_file_system, NULL);
	pthread_detach(hilo_file_system);
}

void enviar_tamanio_pagina(){
	t_buffer* buffer = crear_buffer_nuestro();
	buffer_write_uint32(buffer, config_memoria.tam_pagina);
	enviar_buffer(buffer, socket_cpu);
	destruir_buffer_nuestro(buffer);
}

// FIN CONEXIONES -----------------------------------------------------------------------
void atender_kernel(){
	while(1){
		mensajeKernelMem codigoDeKernel = recibir_codigo(socket_kernel);

		switch(codigoDeKernel){
			case INICIAR_PROCESO_SOLICITUD:
				iniciar_proceso();
				break;
			case FINALIZAR_PROCESO_SOLICITUD:
				finalizarProceso();
				break;
			case PAGE_FAULT_SOLICITUD:
				atender_page_fault();
				break;
			default:
				break;
		}
	}
}

void atender_file_system(){
	while(1){
		mensajeMemoriaFile codigo_recibido = recibir_codigo(socket_file_system);
		
		switch(codigo_recibido){
		case BUSCAR_PAGINA_OK:
			colocar_pagina_en_marco();
			break;
		case LECTURA_MEMORIA_SOLICITUD:
			enviar_bloque_a_fs();
			break;
		case CREAR_PAGINA_RTA:
			asignar_bloque_swap_a_pag();
			break;
		case ESCRITURA_MEMORIA_SOLICITUD:
			escribir_bloque_para_fs();
			break;
		default:
			break;
		}
	}
}

// UTILS KERNEL -------------------------------------------------------------------------
void iniciar_proceso(){
	
	t_buffer* buffer = recibir_buffer(socket_kernel);

	int tam = 0;

	uint32_t pid = buffer_read_uint32(buffer);
	char* nombreArchInstr = buffer_read_string(buffer, &tam);
	uint32_t tamanio = buffer_read_uint32(buffer);

	destruir_buffer_nuestro(buffer);
 
	char* rutaCompleta = string_new();
	string_append(&rutaCompleta, config_memoria.path_instrucciones);
	string_append(&rutaCompleta, nombreArchInstr);

	t_list* listaInstrucciones = parsearArchivo(rutaCompleta, logger_memoria);

	t_proceso* procesoNuevo = crearProceso(listaInstrucciones, pid, tamanio);

	log_info(logger_memoria, "Creación: PID: %d - Tamaño: 0", pid);

	pthread_mutex_lock(&mutex_lista_global_procesos);
	list_add(listaGlobalProceso, procesoNuevo);
	pthread_mutex_unlock(&mutex_lista_global_procesos);

	enviar_codigo(socket_kernel, INICIAR_PROCESO_OK);

	//free(nombreArchInstr); // trae problemas esto cuando corres INTEGRAL_B
	//free(rutaCompleta);
}	

void instruccion_destroy(t_instruccion* instruccion_a_destruir){
	free(instruccion_a_destruir->par1);
	free(instruccion_a_destruir->par2);
	free(instruccion_a_destruir->par3);
	free(instruccion_a_destruir);

	return;
}

void proceso_destroy(t_proceso* proceso_a_destruir){
	list_destroy_and_destroy_elements(proceso_a_destruir->instrucciones, (void* ) instruccion_destroy);
	free(proceso_a_destruir);

	return;
}

t_proceso* crearProceso(t_list* listaInstrucciones, uint32_t pid, uint32_t tamanio){
	t_proceso* proceso = malloc(sizeof(t_proceso));

	proceso->instrucciones = listaInstrucciones;
	proceso->pid = pid;
	proceso->cantMaxMarcos = tamanio / config_memoria.tam_pagina;

	return proceso;
}

void finalizarProceso(){
	t_buffer* buffer = recibir_buffer(socket_kernel);
	uint32_t pidAFinalizar = buffer_read_uint32(buffer);
	destruir_buffer_nuestro(buffer);
	
	// destruir t_proceso
	t_proceso* proceso = buscarProcesoPorPid(pidAFinalizar);
	liberarPaginasDeUnProceso(proceso);
	proceso_destroy(proceso);

	// Notificar a FS para que libere los bloques SWAP
	// enviar_codigo(socket_file_system, LIBERAR_BLOQUES_SOLICITUD);

	enviar_codigo(socket_kernel, FINALIZAR_PROCESO_OK);
}

void liberarPaginasDeUnProceso(t_proceso* procesoAEliminar){
	int cantPaginas = 0;

	for(int i = 0; i < procesoAEliminar->cantMaxMarcos; i++){

		for(int j = 0; j < list_size(tablaGlobalPaginas); j++){
			t_pagina* pagina = list_get(tablaGlobalPaginas, j);
			if(pagina->pidCreador == procesoAEliminar->pid){
				vaciar_marco(pagina->nroMarco);
				liberar_pos_swap(pagina->posSwap);
				list_remove(tablaGlobalPaginas, j);
				free(pagina);
				cantPaginas++;
			}
		}
	}
	
	log_info(logger_memoria, "Destrucción: PID: %d - Tamaño: %d", procesoAEliminar->pid, cantPaginas);
}

void liberar_pos_swap(uint32_t posicion_swap){
	enviar_codigo(socket_file_system, LIBERAR_BLOQUES_SOLICITUD);
	t_buffer* buffer = crear_buffer_nuestro();
	buffer_write_uint32(buffer, posicion_swap);
	enviar_buffer(buffer, socket_file_system);
	destruir_buffer_nuestro(buffer);
}

void vaciar_marco(uint32_t nroMarco){
	list_replace(listaMarcos, nroMarco, NULL);
}

t_pagina* liberar_marco(){
	t_buffer* buffer;

	t_pagina* pag_a_matar = elegir_pagina_victima();
	
	if(pag_a_matar->bitModificado){
		enviar_codigo(socket_file_system, ACTUALIZAR_PAGINA_SOLICITUD);
		
		buffer = crear_buffer_nuestro();
		buffer_write_uint32(buffer, pag_a_matar->posSwap);
		buffer_write_pagina(buffer, pag_a_matar->direccionInicio, config_memoria.tam_pagina);
		enviar_buffer(buffer, socket_file_system);
		destruir_buffer_nuestro(buffer);
	}
	
	pag_a_matar->bitPresencia = false;
	
	uint32_t nroMarco = pag_a_matar->nroMarco;
	vaciar_marco(nroMarco);

	log_info(logger_memoria, "SWAP OUT -  PID: %d - Marco: %d - Page Out: %d-%d", pag_a_matar->pidEnUso, pag_a_matar->nroMarco, pag_a_matar->pidCreador, pag_a_matar->nroPagina);

	return pag_a_matar;
}

void atender_page_fault(){
	t_buffer* buffer = recibir_buffer(socket_kernel);
	uint32_t nro_pagina = buffer_read_uint32(buffer);
	uint32_t pid = buffer_read_uint32(buffer);
	destruir_buffer_nuestro(buffer);
	t_pagina* pag_a_matar = NULL;

	if(!hay_marcos_libres())
		pag_a_matar = liberar_marco();

	t_pagina* pagAPedir = buscarPaginaPorNroYPid(nro_pagina, pid);

	// no existe entonces es una nueva
	if(pagAPedir == NULL){
		uint32_t nroMarco = obtener_marco_libre();
		void* dirInicio = memoriaPrincipal + nroMarco * config_memoria.tam_pagina;
		
		pagAPedir = crear_pagina(nro_pagina, nroMarco, dirInicio, pid);
		list_replace(listaMarcos, nroMarco, pagAPedir); // tiene que colocar la pagina creada en marco libre
		log_info(logger_memoria, "SWAP IN - PID: %d - Marco: %d - Page In: %d-%d", pid, nroMarco, pid, nro_pagina);
		
		if(pag_a_matar != NULL)
			log_info(logger_memoria, "REEMPLAZO - Marco: %d - Page Out: %d-%d - Page In: %d-%d", nroMarco, pag_a_matar->pidEnUso, pag_a_matar->nroPagina, pid, nro_pagina);
		
		sem_post(&sem_pagina_cargada);
	}
	else{
		enviar_codigo(socket_file_system, BUSCAR_PAGINA_SOLICITUD);
		
		buffer = crear_buffer_nuestro();
		buffer_write_uint32(buffer, pagAPedir->posSwap);
		
		// atributos para los logs
		buffer_write_uint32(buffer, pid);
		buffer_write_uint32(buffer, pag_a_matar->pidEnUso);
		buffer_write_uint32(buffer, pag_a_matar->nroPagina);
		buffer_write_uint32(buffer, nro_pagina);
		// fin atributos para los logs

		enviar_buffer(buffer, socket_file_system);
		destruir_buffer_nuestro(buffer);
	}
	sem_wait(&sem_pagina_cargada);
	enviar_codigo(socket_kernel, PAGE_FAULT_OK);
}

// Caso del atender_file_system
void colocar_pagina_en_marco(){
	t_buffer* buffer = recibir_buffer(socket_file_system);
	
	void* pagina = buffer_read_pagina(buffer, config_memoria.tam_pagina);
	uint32_t pid = buffer_read_uint32(buffer);
	uint32_t pag_a_matar_pid_uso = buffer_read_uint32(buffer);
	uint32_t pag_a_matar_nro_pag = buffer_read_uint32(buffer);
	uint32_t nroPagina = buffer_read_uint32(buffer);

	destruir_buffer_nuestro(buffer);

	uint32_t numeroMarco = obtener_marco_libre();

	uint32_t posEnMemoria =  numeroMarco * config_memoria.tam_pagina;

	t_pagina* pagAPedir = buscarPaginaPorNroYPid(nroPagina, pid);

	list_replace(listaMarcos, numeroMarco, pagAPedir);

	escribir_pagina(posEnMemoria, pagina);
	
	pagAPedir->bitModificado = false;
	pagAPedir->bitPresencia = true;
	pagAPedir->direccionInicio = memoriaPrincipal + posEnMemoria;
	pagAPedir->pidEnUso = pid;
	pagAPedir->nroMarco = numeroMarco;

	log_info(logger_memoria, "SWAP IN -  PID: %d - Marco: %d - Page In: %d-%d", pid, numeroMarco, pagAPedir->pidCreador, pagAPedir->nroPagina);
	log_info(logger_memoria, "REEMPLAZO - Marco: %d - Page Out: %d-%d - Page In: %d-%d", numeroMarco, pag_a_matar_pid_uso, pag_a_matar_nro_pag, pagAPedir->pidEnUso, pagAPedir->nroPagina);
	sem_post(&sem_pagina_cargada);
}

// FIN UTILS KERNEL ---------------------------------------------------------------------

// UTILS FILE SYSTEM --------------------------------------------------------------------
void asignar_bloque_swap_a_pag(){
	t_buffer* buffer = recibir_buffer(socket_file_system);
	uint32_t posSwap = buffer_read_uint32(buffer);
	uint32_t nroPag = buffer_read_uint32(buffer);
	uint32_t pid = buffer_read_uint32(buffer);
	destruir_buffer_nuestro(buffer);

	t_pagina* pagina = buscarPaginaPorNroYPid(nroPag, pid);
	
	pagina->posSwap = posSwap;
}

// Para F_READ
void escribir_bloque_para_fs(){
	t_buffer* b = recibir_buffer(socket_file_system);
	void* bloqueAEscribir = buffer_read_pagina(b, config_memoria.tam_pagina);
	uint32_t dirFisica = buffer_read_uint32(b);
	uint32_t puntero = buffer_read_uint32(b);
	uint32_t tam;
	char* nombreArchivo = buffer_read_string(b, &tam);
	uint32_t numPagina = buffer_read_uint32(b);
	uint32_t pid = buffer_read_uint32(b);
	destruir_buffer_nuestro(b);

	escribir_pagina(dirFisica, bloqueAEscribir);

	enviar_codigo(socket_file_system, ESCRITURA_MEMORIA_OK);
	
	t_pagina* pagModificada = buscarPaginaPorNroYPid(numPagina, pid);
	pagModificada->bitModificado = true;
	pagModificada->ultimaReferencia = temporal_get_string_time("%H:%M:%S:%MS");
	
	b = crear_buffer_nuestro();
	buffer_write_uint32(b, dirFisica);
	buffer_write_uint32(b, puntero);
	buffer_write_string(b, nombreArchivo);
	
	enviar_buffer(b, socket_file_system);
	destruir_buffer_nuestro(b);
	
	free(bloqueAEscribir);
	free(nombreArchivo);
}

// Para F_WRITE
void enviar_bloque_a_fs(){
	t_buffer* buffer = recibir_buffer(socket_file_system);
	uint32_t dirFisica = buffer_read_uint32(buffer);
	uint32_t tam;
	char* nombre_archivo = buffer_read_string(buffer, &tam);
	uint32_t puntero = buffer_read_uint32(buffer);
	uint32_t numPagina = buffer_read_uint32(buffer);
	uint32_t pid = buffer_read_uint32(buffer);
	destruir_buffer_nuestro(buffer);

	void* bloqueLeido = leer_pagina(dirFisica);
	
	t_pagina* pagModificada = buscarPaginaPorNroYPid(numPagina, pid);
	pagModificada->ultimaReferencia = temporal_get_string_time("%H:%M:%S:%MS");
	
	enviar_codigo(socket_file_system, LECTURA_MEMORIA_OK);
	buffer = crear_buffer_nuestro();
	buffer_write_pagina(buffer, bloqueLeido, config_memoria.tam_pagina);
	buffer_write_uint32(buffer, dirFisica);
	buffer_write_string(buffer, nombre_archivo);
	buffer_write_uint32(buffer, puntero);
	enviar_buffer(buffer, socket_file_system);
	destruir_buffer_nuestro(buffer);
	
	free(nombre_archivo);
}
// FIN UTILS FILE SYSTEM ----------------------------------------------------------------


// UTILS PAGINAS ------------------------------------------------------------------------
bool hay_marcos_libres(){
	for(int i = 0; i < cantMarcos; i++){
		t_pagina* pagina = list_get(listaMarcos, i);
		if(pagina == NULL)
			return true;
	}
	return false;
}

uint32_t obtener_marco_libre(){
	for(int i = 0; i < cantMarcos; i++){
		t_pagina* pagina = list_get(listaMarcos, i);
		if(pagina == NULL)
			return i;
	}
}

t_pagina* elegir_pagina_victima(){
	t_pagina* pagReemplazada;
	if(string_equals_ignore_case(config_memoria.algoritmo_reemplazo, "LRU"))
		pagReemplazada = retirarPaginaPorLRU();
	if(string_equals_ignore_case(config_memoria.algoritmo_reemplazo, "FIFO"))
		pagReemplazada = retirarPaginaPorFIFO();
    
	return pagReemplazada;
}

t_pagina* retirarPaginaPorLRU(){ // tiene que sacar la pagina menos referenciada recientemente
    t_pagina* pagAReemplazar;
    int tiempoMinimoMilisegundos;

    for(int i = 0; i < list_size(listaMarcos); i++){
        t_pagina* pAux = list_get(listaMarcos, i);
        int tiempoAuxMilisegundos = obtenerTiempoEnMiliSegundos(pAux->ultimaReferencia);
        if(i == 0 || tiempoAuxMilisegundos < tiempoMinimoMilisegundos){
            pagAReemplazar = pAux;
            tiempoMinimoMilisegundos = obtenerTiempoEnMiliSegundos(pagAReemplazar->ultimaReferencia);
        }
    }

    return pagAReemplazar;
}

int obtenerTiempoEnMiliSegundos(char* tiempo){
    char** horaNuevaDesarmada = string_split(tiempo,":");

    int horas = atoi(horaNuevaDesarmada[0]);
    int minutos = atoi(horaNuevaDesarmada[1]) + horas * 60;
    int segundos = atoi(horaNuevaDesarmada[2]) + minutos * 60;
    int miliSegundos = atoi(horaNuevaDesarmada[3]) + segundos * 1000;

    string_array_destroy(horaNuevaDesarmada);

    return miliSegundos;
}

t_pagina* retirarPaginaPorFIFO(){ // tiene que sacar la primera pagina que fue cargada
	t_pagina* pagAReemplazar;
	
	pagAReemplazar = list_get(listaMarcos, indicePorFifo);
	indicePorFifo++;

	if(indicePorFifo == list_size(listaMarcos))
		indicePorFifo = 0;

	return pagAReemplazar;
}

void escribir_pagina(uint32_t posEnMemoria, void* pagina){
	memcpy(memoriaPrincipal + posEnMemoria, pagina, config_memoria.tam_pagina);
}

void* leer_pagina(uint32_t posEnMemoria){
	return (memoriaPrincipal + posEnMemoria);
}

t_pagina* crear_pagina(uint32_t nroPag, uint32_t nroMarco, void* dirreccionInicio, uint32_t pid){
	t_pagina* p = malloc(sizeof(t_pagina));

	p->bitModificado = false;
	p->bitPresencia = true;
	p->direccionInicio = dirreccionInicio;
	p->nroMarco = nroMarco;
	p->nroPagina = nroPag;
	p->pidCreador = pid;
	p->pidEnUso = pid;
	p->ultimaReferencia = temporal_get_string_time("%H:%M:%S:%MS");

	list_add(tablaGlobalPaginas, p);
	
	enviar_codigo(socket_file_system, CREAR_PAGINA_SOLICITUD);
	
	t_buffer* b = crear_buffer_nuestro();
	buffer_write_uint32(b, nroPag);
	buffer_write_uint32(b, p->pidCreador);
	enviar_buffer(b, socket_file_system);
	destruir_buffer_nuestro(b);
	
	return p;
}

// FIN PAGINAS ----------------------------------------------------------