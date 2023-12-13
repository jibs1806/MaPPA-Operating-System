#include <memInstrucciones.h>

void crearPaginaPrueba(){
	t_pagina* pagina = malloc(sizeof(t_pagina));
	pagina->pidCreador = 3;
	pagina->pidEnUso = 5;
	pagina->nroMarco = 10;
	pagina->nroPagina = 0;

	pagina->bitModificado = false;
	pagina->bitPresencia = true;

	pagina->ultimaReferencia = temporal_get_string_time("%H:%M:%S:%MS");
	pagina->direccionInicio = memoriaPrincipal;
	pagina->posSwap = 0;

	list_add(tablaGlobalPaginas, pagina);
}

t_pagina* buscarPaginaPorNroYPid(uint32_t nroPag, uint32_t pid){

	for(int i = 0; i < list_size(tablaGlobalPaginas); i++){
		t_pagina* pag = list_get(tablaGlobalPaginas, i);

		if(pag->nroPagina == nroPag && pag->pidCreador == pid)
			return pag;
	}

	return NULL;
}

// Si existe page fault, devuelve NULL
t_pagina* existePageFault(uint32_t nroPag, uint32_t pid){
	t_pagina* pagina = buscarPaginaPorNroYPid(nroPag, pid);

	if(pagina == NULL || !(pagina->bitPresencia))
		return NULL;

	return pagina;
}


void atender_cpu(){
	while(1){
		// Recibe primero el codigo
		// Despues recibe y deserializa el buffer adentro de cada caso del switch
		mensajeCpuMem codigoDeCpu = recibir_codigo(socket_cpu);

		switch(codigoDeCpu){
			case MOV_IN_SOLICITUD:
				ejecutarMovIn();
				break;
			case MOV_OUT_SOLICITUD:
				ejecutarMovOut();
				break;
			case NUMERO_MARCO_SOLICITUD:
				devolver_nro_marco();
				break;
			case PEDIDO_INSTRUCCION:
				usleep(config_memoria.retardo_respuesta * 1000);
				enviarInstruccion();
				break;
			default:
				break;
		}
	}
}

void devolver_nro_marco(){
	t_buffer* buffer = recibir_buffer(socket_cpu);
	uint32_t nro_pagina = buffer_read_uint32(buffer);
	uint32_t pid = buffer_read_uint32(buffer);
	destruir_buffer_nuestro(buffer);
	
	t_pagina* pagina = existePageFault(nro_pagina, pid);
	if(pagina == NULL)
		enviar_codigo(socket_cpu, PAGE_FAULT);
	else{
		log_info(logger_memoria, "Acceso a TGP - PID: %d - Página: %d - Marco: %d", pid, pagina->nroPagina, pagina->nroMarco);
		enviar_codigo(socket_cpu, NUMERO_MARCO_OK);
		buffer = crear_buffer_nuestro();
		buffer_write_uint32(buffer, pagina->nroMarco);
		enviar_buffer(buffer, socket_cpu);
		destruir_buffer_nuestro(buffer);
	}
}

//  Lee el valor de memoria correspondiente a la Dirección Lógica y lo almacena en el Registro.
void ejecutarMovIn(){
	t_buffer* buffer = recibir_buffer(socket_cpu);
	uint32_t dirFisica = buffer_read_uint32(buffer);
	uint32_t pid = buffer_read_uint32(buffer);
	uint32_t nroPag = buffer_read_uint32(buffer);
	destruir_buffer_nuestro(buffer);

	uint32_t valorLeido = 0;
	memcpy(&valorLeido, memoriaPrincipal + dirFisica, sizeof(uint32_t));

	enviar_codigo(socket_cpu, MOV_IN_OK);

	t_pagina* pagLeida = buscarPaginaPorNroYPid(nroPag, pid);
	pagLeida->ultimaReferencia = temporal_get_string_time("%H:%M:%S:%MS");

	log_info(logger_memoria, "PID: %d - Acción: LEER - Dirección física: %d", pid, dirFisica);

	//log_warning(logger_memoria, "Valor leido: %d", valorLeido);

	buffer = crear_buffer_nuestro();
	buffer_write_uint32(buffer, valorLeido);
	enviar_buffer(buffer, socket_cpu);
	destruir_buffer_nuestro(buffer);
}

//  Lee el valor del Registro y lo escribe en la dirección física
void ejecutarMovOut(){
	t_buffer* buffer = recibir_buffer(socket_cpu);
	uint32_t dirFisica = buffer_read_uint32(buffer);
	uint32_t valorAEscribir = buffer_read_uint32(buffer);
	uint32_t pid = buffer_read_uint32(buffer);
	uint32_t numPagina = buffer_read_uint32(buffer);
	destruir_buffer_nuestro(buffer);

	memcpy(memoriaPrincipal + dirFisica, &valorAEscribir, sizeof(uint32_t));
	
	t_pagina* pagModificada = buscarPaginaPorNroYPid(numPagina, pid);
	pagModificada->ultimaReferencia = temporal_get_string_time("%H:%M:%S:%MS");
	pagModificada->bitModificado = true;
	
	enviar_codigo(socket_cpu, MOV_OUT_OK);

	log_info(logger_memoria, "PID: %d - Acción: ESCRIBIR - Dirección física: %d", pid, dirFisica);
}

void enviarInstruccion(){
	t_buffer* buffer = recibir_buffer(socket_cpu);

	uint32_t pid = buffer_read_uint32(buffer);
	uint32_t pc = buffer_read_uint32(buffer);

	destruir_buffer_nuestro(buffer);

	// Suponemos que si se consulta por un proceso es porque ya existe
	pthread_mutex_lock(&mutex_lista_global_procesos);
	t_proceso* proceso = buscarProcesoPorPid(pid);
	pthread_mutex_unlock(&mutex_lista_global_procesos);

	t_instruccion* instruccion = list_get(proceso->instrucciones, pc);
	
	buffer = crear_buffer_nuestro();
	buffer_write_instruccion(buffer, instruccion);
	enviar_buffer(buffer, socket_cpu);
	destruir_buffer_nuestro(buffer);
}

t_proceso* buscarProcesoPorPid(uint32_t pid){
	for(int i = 0; i < list_size(listaGlobalProceso); i++){
		t_proceso* proceso = list_get(listaGlobalProceso, i);
		if(proceso->pid == pid)
			return proceso;
	}
}

//  TODO Falta arreglar lo de la ruta para que solo reciba el nombre del .txt
t_list* parsearArchivo(char* pathArchivo, t_log* logger){
	t_list* listaInstrucciones = list_create();

	FILE* archivoInstrucciones = fopen(pathArchivo, "r");
	int length = 50;

	// Aca guarda un renglon del .txt
	char instruccion[50];

	// Aca guarda los parametros recibidos
	char* parametro;

	t_instruccion* instr;

	while(fgets(instruccion, length, archivoInstrucciones)){
		strtok(instruccion, "\n");

		instr = malloc(sizeof(t_instruccion));
		instr->par1 = NULL;
		instr->par2 = NULL;
		instr->par3 = NULL;

		parametro = strtok(instruccion, " ");

		instr->codigo = obtenerCodigoInstruccion(parametro);

		int indice = 1;

		parametro = strtok(NULL, " ");
		while(parametro != NULL){
			escribirCharParametroInstruccion(indice, instr, parametro);
			indice++;
			parametro = strtok(NULL, " ");
		}

		list_add(listaInstrucciones, instr);
	}
	fclose(archivoInstrucciones);
	return listaInstrucciones;
}


t_instruccion* 	parsearInstruccion(codigoInstruccion cod, void* par1, void* par2, void* par3){
	t_instruccion* instr = malloc(sizeof(t_instruccion));

	return instr;
}


codigoInstruccion obtenerCodigoInstruccion(char* instruccion){
	if(strcmp(instruccion,"SET") == 0)
		return SET;
	if(strcmp(instruccion,"SUM") == 0)
		return SUM;
	if(strcmp(instruccion,"SUB") == 0)
		return SUB;
	if(strcmp(instruccion,"JNZ") == 0)
		return JNZ;
	if(strcmp(instruccion,"SLEEP") == 0)
		return SLEEP;
	if(strcmp(instruccion,"WAIT") == 0)
		return WAIT;
	if(strcmp(instruccion,"SIGNAL") == 0)
		return SIGNAL;
	if(strcmp(instruccion,"MOV_IN") == 0)
		return MOV_IN;
	if(strcmp(instruccion,"MOV_OUT") == 0)
		return MOV_OUT;
	if(strcmp(instruccion,"F_OPEN") == 0)
		return F_OPEN;
	if(strcmp(instruccion,"F_CLOSE") == 0)
		return F_CLOSE;
	if(strcmp(instruccion,"F_SEEK") == 0)
		return F_SEEK;
	if(strcmp(instruccion,"F_READ") == 0)
		return F_READ;
	if(strcmp(instruccion,"F_WRITE") == 0)
		return F_WRITE;
	if(strcmp(instruccion,"F_TRUNCATE") == 0)
		return F_TRUNCATE;
	if(strcmp(instruccion,"EXIT") == 0)
		return EXIT;
}
