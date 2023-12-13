#include <mmu.h>

int obtener_numero_pagina(int direccion_logica){
	return floor(direccion_logica / tamanio_pagina);
}

int obtener_desplazamiento_pagina(int direccion_logica){
	int numero_pagina = obtener_numero_pagina(direccion_logica);
	return direccion_logica - numero_pagina * tamanio_pagina;
}

uint32_t calcular_direccion_fisica(int direccion_logica, t_cde* cde){
	int nro_pagina = obtener_numero_pagina(direccion_logica);
	int desplazamiento = obtener_desplazamiento_pagina(direccion_logica);

	// Con el tam de pagina que recibio el cpu en el handshake con memoria. 
	// Ademas se le pide aca a memoria el nro de marco de la pag calculada
	// Dsps queda hacer la base del marco + desplazamiento ????
	// TO DO

	enviar_codigo(socket_memoria, NUMERO_MARCO_SOLICITUD);
	t_buffer* buffer = crear_buffer_nuestro();
	buffer_write_uint32(buffer, nro_pagina);
	buffer_write_uint32(buffer, cde->pid);
	enviar_buffer(buffer, socket_memoria);
	destruir_buffer_nuestro(buffer);

	mensajeCpuMem codigo_recibido = recibir_codigo(socket_memoria);

	if(codigo_recibido == NUMERO_MARCO_OK){
		buffer = recibir_buffer(socket_memoria);
		uint32_t nro_marco_recibido = buffer_read_uint32(buffer);
		destruir_buffer_nuestro(buffer);
		log_info(logger_cpu, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", cde->pid, nro_pagina, nro_marco_recibido);
		return nro_marco_recibido * tamanio_pagina + desplazamiento; // retorna la direccion_fisica
	}
	else if(codigo_recibido == PAGE_FAULT){
		log_info(logger_cpu, "Page Fault PID: %d - Página: %d", cde->pid, nro_pagina);
        interruption = 1; // devolvemos a kernel el cde del proceso en ejecucion
		cde->pc--;
		return -1;
	}
}