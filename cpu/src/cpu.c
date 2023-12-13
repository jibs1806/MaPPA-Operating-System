#include <cpu.h>

int main(int argc, char* argv[]) {

    char* config_path = argv[1]; // TO DO

    inicializarModulo("./cpu.config");

    sem_wait(&no_end_cpu); //semaforo que esta inicializado en 
    
    terminar_conexiones(3, socket_kernel_dispatch, socket_kernel_interrupt, socket_memoria);
    
    terminar_programa(logger_cpu, config);
    return 0;
}

// INICIALIZACIONES
void inicializarModulo(char* config_path){
    levantarLogger();
    levantarConfig(config_path);
    
    interruption = 0;
    interrupcion_consola = 0;
    realizar_desalojo = 0;
    pf_con_funcion_fs = 0;

    inicializar_registros();
    inicializar_semaforos();

    inicializarConexiones();
    return;
}

void inicializar_semaforos(){
    sem_init(&cpu_libre, 0, 0);
    sem_init(&no_end_cpu, 0, 0);
    pthread_mutex_init(&mutex_interrupcion_consola, NULL);
    pthread_mutex_init(&mutex_realizar_desalojo, NULL);
    pthread_mutex_init(&mutex_cde_ejecutando, NULL);
    pthread_mutex_init(&mutex_instruccion_actualizada, NULL);
}

void inicializar_registros(){
    registros_cpu = malloc(sizeof(t_registro));
    
    registros_cpu->AX = 0;
    registros_cpu->BX = 0;
    registros_cpu->CX = 0;
    registros_cpu->DX = 0;

}

void inicializarConexiones(){
    socket_memoria = inicializar_cliente(config_cpu.ip_memoria, config_cpu.puerto_memoria, logger_cpu);
	recibir_tamanio_pagina(socket_memoria); 

    conectarKernel(); //crear servidor y esperar cliente (kernel)
}
// FIN INICIALIZACIONES

// CONEXIONES
void conectarKernel(){
    int socket_servidor_dispatch = crear_servidor(config_cpu.puerto_escucha_dispatch); //TO DO tema dispathc interrupt
    int socket_servidor_interrupt = crear_servidor(config_cpu.puerto_escucha_interrupt);
    
    pthread_t thread_dispatch, thread_interrupt;
    
    pthread_create(&thread_dispatch, NULL, (void* ) dispatchProceso, (void*) (intptr_t) socket_servidor_dispatch);
    pthread_create(&thread_interrupt, NULL, (void* ) interruptProceso, (void* ) (intptr_t) socket_servidor_interrupt);

    pthread_detach(thread_dispatch);
    pthread_detach(thread_interrupt);
}

void dispatchProceso(void* socket_server){

    int socket_servidor_dispatch = (int) (intptr_t) socket_server;
    
    log_info(logger_cpu, "Esperando Kernel DISPATCH....");
    socket_kernel_dispatch = esperar_cliente(socket_servidor_dispatch); //En la librería faltaba utils/conexiones.h, ya no hace falta agreagarlo porque se encuentra en instruccionescpu.h
    log_info(logger_cpu, "Se conecto el Kernel por DISPATCH");
    
    while(1){
        mensajeKernelCpu op_code = recibir_codigo(socket_kernel_dispatch);

        t_buffer* buffer = recibir_buffer(socket_kernel_dispatch);

        switch(op_code){
            case EJECUTAR_PROCESO:
                t_cde* cde_recibido = buffer_read_cde(buffer);
                destruir_buffer_nuestro(buffer);

                pthread_mutex_lock(&mutex_cde_ejecutando);
                pid_de_cde_ejecutando = cde_recibido->pid;
                pthread_mutex_unlock(&mutex_cde_ejecutando);
                
                ejecutar_proceso(cde_recibido);
                break;
            case ALGORITMO_PLANIFICACION:
                algoritmo_planificacion = buffer_read_uint32(buffer);
                destruir_buffer_nuestro(buffer);
                break;
            default:
                destruir_buffer_nuestro(buffer);
                log_error(logger_cpu, "Codigo de operacion desconocido.");
                log_error(logger_cpu, "Finalizando modulo.");
                exit(1);
                break;
        }
    }
}

void interruptProceso(void* socket_server){
    int socket_servidor_interrupt = (int) (intptr_t) socket_server;
    
    log_info(logger_cpu, "Esperando Kernel INTERRUPT....");
    socket_kernel_interrupt = esperar_cliente(socket_servidor_interrupt);
    log_info(logger_cpu, "Se conecto el Kernel por INTERRUPT");
    
    while(1){
        // Esperar que termine de ejecutar (semaforo)
        // Chequear si le llego una interrupcion del kernel
        // Si llego una, la atiendo
        // Si no, posteo el semaforo de ejecucion

        mensajeKernelCpu op_code = recibir_codigo(socket_kernel_interrupt);
        t_buffer* buffer = recibir_buffer(socket_kernel_interrupt); // recibe pid o lo que necesite
        uint32_t pid_recibido = buffer_read_uint32(buffer);
        destruir_buffer_nuestro(buffer);
        
        switch (op_code){
            case INTERRUPT:
                // atendemos la interrupcion
                pthread_mutex_lock(&mutex_interrupcion_consola);
                interrupcion_consola = 1;
                pthread_mutex_unlock(&mutex_interrupcion_consola);
                break;
            case DESALOJO:
                // se desaloja proceso en ejecucion
                if(algoritmo_planificacion == 2 && pid_de_cde_ejecutando != pid_recibido){ // significa que el algoritmo es RR
                    break;
                }
                else if(algoritmo_planificacion == 2 && pid_de_cde_ejecutando == pid_recibido){
                    if(es_bloqueante(instruccion_actualizada)){
                        break;
                    }
                }
                pthread_mutex_lock(&mutex_realizar_desalojo);
                realizar_desalojo = 1;
                pthread_mutex_unlock(&mutex_realizar_desalojo);
                break;
            default:
                log_warning(logger_cpu, "entre al case de default");
                break;
        }
    }
}

// FUNCIONES AUXILIARES INSTRUCCIONES
bool es_bloqueante(codigoInstruccion instruccion_actualizada){
    switch(instruccion_actualizada){
    case SLEEP:
        return true;
        break;
    case WAIT:
        return true;
        break;
    case SIGNAL:
        return true;
        break;
    case F_OPEN:
        return true;
        break;
    case F_CLOSE:
        return true;
        break;
    case F_SEEK:
        return true;
        break;
    case F_READ:
        return true;
        break;
    case F_WRITE:
        return true;
        break;
    case F_TRUNCATE:
        return true;
        break;
    case EXIT:
        return true;
        break;
    case EXIT_POR_CONSOLA:
        return true;
        break;
    case NULO_INST:
        return false;
        break;
    default:
        return false;
        break;
    }
}
void recibir_tamanio_pagina(int socket_memoria){
    t_buffer* buffer = recibir_buffer(socket_memoria);
    tamanio_pagina = buffer_read_uint32(buffer);
    destruir_buffer_nuestro(buffer);
}

uint32_t buscar_valor_registro(void* registro){
	uint32_t valorLeido;

	if(strcmp(registro, "AX") == 0)
		valorLeido = registros_cpu->AX;
	else if(strcmp(registro, "BX") == 0)
		valorLeido = registros_cpu->BX;

	else if(strcmp(registro, "CX") == 0)
		valorLeido = registros_cpu->CX;

	else if(strcmp(registro, "DX") == 0)
		valorLeido = registros_cpu->DX;

	return valorLeido;
}

void cargar_registros(t_cde* cde){
    registros_cpu->AX = cde->registros->AX;
    registros_cpu->BX = cde->registros->BX;
    registros_cpu->CX = cde->registros->CX;
    registros_cpu->DX = cde->registros->DX;
}

void guardar_cde(t_cde* cde){
    cde->registros->AX = registros_cpu->AX;
    cde->registros->BX = registros_cpu->BX;
    cde->registros->CX = registros_cpu->CX;
    cde->registros->DX = registros_cpu->DX;
}

void devolver_cde_a_kernel(t_cde* cde, t_instruccion* instruccion_a_ejecutar){

    t_buffer* buffer = crear_buffer_nuestro();
    buffer_write_cde(buffer, cde);
    buffer_write_instruccion(buffer, instruccion_a_ejecutar);

    // caso de page fault, tiene que volver a kernel con el nroPagina que genero el page fault
    if(instruccion_a_ejecutar->codigo == MOV_IN){
        uint32_t dirLogica = leerEnteroParametroInstruccion(2, instruccion_a_ejecutar);
        uint32_t nroPagina = obtener_numero_pagina(dirLogica);
        buffer_write_uint32(buffer, nroPagina);
    }
    else if(instruccion_a_ejecutar->codigo == MOV_OUT){
        uint32_t dirLogica = leerEnteroParametroInstruccion(1, instruccion_a_ejecutar);
        uint32_t nroPagina = obtener_numero_pagina(dirLogica);
        buffer_write_uint32(buffer, nroPagina);
    }
    // caso de F_WRITE y F_READ, devuelve a kernel la direccion fisica
    else if(instruccion_a_ejecutar->codigo == F_READ || instruccion_a_ejecutar->codigo == F_WRITE){
        uint32_t dirLogica = leerEnteroParametroInstruccion(2, instruccion_a_ejecutar);
        if(pf_con_funcion_fs){
            uint32_t nroPagina = obtener_numero_pagina(dirLogica);
            buffer_write_uint8(buffer, HAY_PAGE_FAULT);
            buffer_write_uint32(buffer, nroPagina);
            pf_con_funcion_fs = 0;
        }
        else{
            uint32_t nroPagina = obtener_numero_pagina(dirLogica);
            uint32_t dirFisica = calcular_direccion_fisica(dirLogica, cde);
            buffer_write_uint8(buffer, DIRECCION_FISICA_OK);
            buffer_write_uint32(buffer, dirFisica);
            buffer_write_uint32(buffer, nroPagina);
        }
    }

    enviar_buffer(buffer, socket_kernel_dispatch);
    destruir_buffer_nuestro(buffer);
}

// FIN FUNCIONES AUXILIARES INSTRUCCIONES

// FIN CONEXIONES
void ejecutar_proceso(t_cde* cde){
    cargar_registros(cde);
    t_instruccion* instruccion_a_ejecutar;
    
    while(interruption != 1 && interrupcion_consola != 1 && realizar_desalojo != 1){
        //Pedir a memoria la instruccion pasandole el pid y el pc

        log_info(logger_cpu, "PID: %d - FETCH - Program Counter: %d", cde->pid, cde->pc);

        enviar_codigo(socket_memoria, PEDIDO_INSTRUCCION);
        t_buffer* buffer_envio = crear_buffer_nuestro();
        buffer_write_uint32(buffer_envio, cde->pid);
        buffer_write_uint32(buffer_envio, cde->pc);

        enviar_buffer(buffer_envio, socket_memoria);

        destruir_buffer_nuestro(buffer_envio);
        
        cde->pc++;

        t_buffer* buffer_recibido = recibir_buffer(socket_memoria);
        instruccion_a_ejecutar = buffer_read_instruccion(buffer_recibido);
        destruir_buffer_nuestro(buffer_recibido);
        
        pthread_mutex_lock(&mutex_instruccion_actualizada);
        instruccion_actualizada = instruccion_a_ejecutar->codigo;
        pthread_mutex_unlock(&mutex_instruccion_actualizada);

        ejecutar_instruccion(cde, instruccion_a_ejecutar);
    }

    if(interruption){
        interruption = 0;
        pthread_mutex_lock(&mutex_interrupcion_consola);
        interrupcion_consola = 0;
        pthread_mutex_unlock(&mutex_interrupcion_consola);
        pthread_mutex_lock(&mutex_realizar_desalojo);
        realizar_desalojo = 0;
        pthread_mutex_unlock(&mutex_realizar_desalojo);
        log_info(logger_cpu, "PID: %d - Volviendo a kernel por instruccion %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar));
        desalojar_cde(cde, instruccion_a_ejecutar);
    }
    else if(realizar_desalojo == 1 && interruption != 1){ // con RR y PRIORIDADES sale por aca
        interruption = 0;
        pthread_mutex_lock(&mutex_interrupcion_consola);
        interrupcion_consola = 0;
        pthread_mutex_unlock(&mutex_interrupcion_consola);
        pthread_mutex_lock(&mutex_realizar_desalojo);
        realizar_desalojo = 0;
        pthread_mutex_unlock(&mutex_realizar_desalojo);
        if(algoritmo_planificacion == 1) // significa que es PRIORIDADES
            log_info(logger_cpu, "PID: %d - Desalojado por proceso de mayor prioridad", cde->pid);
        else if(algoritmo_planificacion == 2) // significa que es RR
            log_info(logger_cpu, "PID: %d - Desalojado por fin de Quantum", cde->pid); 
        desalojar_cde(cde, instruccion_a_ejecutar);
    }
    else{
        interruption = 0;
        pthread_mutex_lock(&mutex_interrupcion_consola);
        interrupcion_consola = 0;
        pthread_mutex_unlock(&mutex_interrupcion_consola);
        pthread_mutex_lock(&mutex_realizar_desalojo);
        realizar_desalojo = 0;
        pthread_mutex_unlock(&mutex_realizar_desalojo);
        instruccion_a_ejecutar->codigo = EXIT_POR_CONSOLA;
        instruccion_a_ejecutar->par1 = NULL;
        instruccion_a_ejecutar->par2 = NULL;
        instruccion_a_ejecutar->par3 = NULL;
        log_info(logger_cpu, "PID: %d - Volviendo a kernel por FINALIZAR_PROCESO", cde->pid);
        desalojar_cde(cde, instruccion_a_ejecutar);
    }
}

char* obtener_nombre_instruccion(t_instruccion* instruccion){
    switch(instruccion->codigo){
        case SET:
            return "SET";
            break;
	    case SUM:
            return "SUM";
            break;
	    case SUB:
            return "SUB";
            break;
	    case JNZ:
            return "JNZ";
            break;
	    case SLEEP:
            return "SLEEP";
            break;
	    case WAIT:
            return "WAIT";
            break;
	    case SIGNAL:
            return "SIGNAL";
            break;
	    case MOV_IN:
            return "MOV_IN";
            break;
	    case MOV_OUT:
            return "MOV_OUT";
            break;
	    case F_OPEN:
            return "F_OPEN";
            break;
	    case F_CLOSE:
            return "F_CLOSE";
            break;
	    case F_SEEK:
            return "F_SEEK";
            break;
	    case F_READ:
            return "F_READ";
            break;
	    case F_WRITE:
            return "F_WRITE";
            break;
	    case F_TRUNCATE:
            return "F_TRUNCATE";
            break;
	    case EXIT:
            return "EXIT";
            break;
	    case EXIT_POR_CONSOLA:
            return "EXIT_POR_CONSOLA";
            break;
        default:
            return "Instruccion desconocida";
            break;
    }
}

void ejecutar_instruccion(t_cde* cde, t_instruccion* instruccion_a_ejecutar){
    uint32_t par1;
    uint32_t par2;
    switch(instruccion_a_ejecutar->codigo){
        case SET:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            par2 = leerEnteroParametroInstruccion(2, instruccion_a_ejecutar);
            ejecutar_set(instruccion_a_ejecutar->par1, par2);
            if (interruption == 0 && realizar_desalojo == 0 && interrupcion_consola == 0)
                destruir_instruccion(instruccion_a_ejecutar);
            break;
        case SUM:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            ejecutar_sum(instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            if (interruption == 0 && realizar_desalojo == 0 && interrupcion_consola == 0)
                destruir_instruccion(instruccion_a_ejecutar);
            break;
        case SUB:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            ejecutar_sub(instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            if (interruption == 0 && realizar_desalojo == 0 && interrupcion_consola == 0)
                destruir_instruccion(instruccion_a_ejecutar);
            break;
        case MOV_IN:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            par2 = leerEnteroParametroInstruccion(2, instruccion_a_ejecutar);
            ejecutar_mov_in(instruccion_a_ejecutar->par1, par2, cde);
            if (interruption == 0 && realizar_desalojo == 0 && interrupcion_consola == 0)
                destruir_instruccion(instruccion_a_ejecutar);
            break;
        case MOV_OUT:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            par1 = leerEnteroParametroInstruccion(1, instruccion_a_ejecutar);
            ejecutar_mov_out(par1, instruccion_a_ejecutar->par2, cde);
            if (interruption == 0 && realizar_desalojo == 0 && interrupcion_consola == 0)
                destruir_instruccion(instruccion_a_ejecutar);
            break;
        case SLEEP:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1);
            par1 = leerEnteroParametroInstruccion(1, instruccion_a_ejecutar);
            ejecutar_sleep(par1);
            if (interruption == 0 && realizar_desalojo == 0 && interrupcion_consola == 0)
                destruir_instruccion(instruccion_a_ejecutar);
            break;
        case JNZ:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            par2 = leerEnteroParametroInstruccion(2, instruccion_a_ejecutar);
            ejecutar_jnz(instruccion_a_ejecutar->par1, par2, cde);
            if (interruption == 0 && realizar_desalojo == 0 && interrupcion_consola == 0)
                destruir_instruccion(instruccion_a_ejecutar);
            break;
        case WAIT:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1);
            ejecutar_wait(instruccion_a_ejecutar->par1);
            if (interruption == 0 && realizar_desalojo == 0 && interrupcion_consola == 0)
                destruir_instruccion(instruccion_a_ejecutar);
            break;
        case SIGNAL:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1);
            ejecutar_signal(instruccion_a_ejecutar->par1);
            if (interruption == 0 && realizar_desalojo == 0 && interrupcion_consola == 0)
                destruir_instruccion(instruccion_a_ejecutar);
            break;
        case F_OPEN:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            ejecutar_f_open(instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            break;
        case F_CLOSE:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1);
            ejecutar_f_close(instruccion_a_ejecutar->par1);
            break;
        case F_SEEK:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            ejecutar_f_seek(instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            break;
        case F_TRUNCATE: // No sabemos porque no la toma, creemos que puede ser algo de memoria
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            ejecutar_f_truncate(instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            break;
        case F_WRITE:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            ejecutar_f_write(instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2, cde);
            break;
        case F_READ:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s - %s %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar), instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2);
            ejecutar_f_read(instruccion_a_ejecutar->par1, instruccion_a_ejecutar->par2, cde);
            break;
        case EXIT:
            log_info(logger_cpu, "PID: %d - Ejecutando: %s", cde->pid, obtener_nombre_instruccion(instruccion_a_ejecutar));
            ejecutar_exit();
            break;
        default:
            log_warning(logger_cpu, "Instruccion no reconocida");
            break;
    }
}


void desalojar_cde(t_cde* cde, t_instruccion* instruccion_a_ejecutar){
    // Devoluelvo el cde en ejecucuion
    // Devuelve la razon de interrupcion ??
    guardar_cde(cde); //cargar registros de cpu en el cde
    devolver_cde_a_kernel(cde, instruccion_a_ejecutar);
    destruir_cde(cde);
    
    pthread_mutex_lock(&mutex_cde_ejecutando);
    pid_de_cde_ejecutando = UINT32_MAX;
    pthread_mutex_unlock(&mutex_cde_ejecutando);

    pthread_mutex_lock(&mutex_instruccion_actualizada);
    instruccion_actualizada = NULO_INST;
    pthread_mutex_unlock(&mutex_instruccion_actualizada);

    destruir_instruccion(instruccion_a_ejecutar);
}

// FUNCIONES INSTRUCCCIONES
void ejecutar_set(char* registro, uint32_t valor_recibido){
    if(strcmp(registro, "AX") == 0){
        registros_cpu->AX = valor_recibido;   
    }
    else if(strcmp(registro, "BX") == 0)
        registros_cpu->BX = valor_recibido;

    else if(strcmp(registro, "CX") == 0)
        registros_cpu->CX = valor_recibido;

    else if(strcmp(registro, "DX") == 0)
        registros_cpu->DX = valor_recibido;
        
    else
        log_error(logger_cpu, "No se reconoce el registro %s", registro);
}

void ejecutar_sum(char* reg_dest, char* reg_origen){
    uint32_t valor_reg_origen = buscar_valor_registro(reg_origen);
    
    if(strcmp(reg_dest, "AX") == 0)
        registros_cpu->AX += valor_reg_origen;

    else if(strcmp(reg_dest, "BX") == 0)
        registros_cpu->BX += valor_reg_origen;

    else if(strcmp(reg_dest, "CX") == 0)
        registros_cpu->CX += valor_reg_origen;

    else if(strcmp(reg_dest, "DX") == 0)
        registros_cpu->DX += valor_reg_origen;

    else
        log_warning(logger_cpu, "Registro no reconocido");
}

void ejecutar_sub(char* reg_dest, char* reg_origen){
    uint32_t valor_reg_origen = buscar_valor_registro(reg_origen);

    if(strcmp(reg_dest, "AX") == 0)
        registros_cpu->AX -= valor_reg_origen;

    else if(strcmp(reg_dest, "BX") == 0)
        registros_cpu->BX -= valor_reg_origen;
    
    else if(strcmp(reg_dest, "CX") == 0)
        registros_cpu->CX -= valor_reg_origen;
    
    else if(strcmp(reg_dest, "DX") == 0)
        registros_cpu->DX -= valor_reg_origen;
    
    else
        log_warning(logger_cpu, "Registro no reconocido");
}

void ejecutar_jnz(void* registro, uint32_t nro_instruccion, t_cde* cde){
    if(strcmp(registro, "AX") == 0){
        if(registros_cpu->AX != 0)
            cde->pc = nro_instruccion;
    }

    else if(strcmp(registro, "BX") == 0){
        if(registros_cpu->BX != 0)
            cde->pc = nro_instruccion;
    }

    else if(strcmp(registro, "CX") == 0){
        if(registros_cpu->CX != 0)
            cde->pc = nro_instruccion;
    }

    else if(strcmp(registro, "DX") == 0){
        if(registros_cpu->DX != 0)
            cde->pc = nro_instruccion;
    }

    else
        log_warning(logger_cpu, "Registro no reconocido");
}

void ejecutar_sleep(uint32_t tiempo){ //devolver cde al kernel con la cant de segundos que el proceso se va a bloquear
    interruption = 1;
}

void ejecutar_wait(char* recurso){ //solicitar a kernel que se asigne una instancia del recurso
    interruption = 1;
}

void ejecutar_signal(char* recurso){ //solicitar a kernel que se libere una instancia del recurso
    interruption = 1;
}

//  Lee el valor de memoria correspondiente a la Direccion Logica y lo almacena en el Registro.
void ejecutar_mov_in(char* registro, uint32_t dirLogica, t_cde* cde){
    uint32_t dirFisica = calcular_direccion_fisica(dirLogica, cde);
    uint32_t numPagina = obtener_numero_pagina(dirLogica);

    if(interruption){ // significa que hubo page fault
        return;
    }

    enviar_codigo(socket_memoria, MOV_IN_SOLICITUD);
    
    t_buffer* buffer = crear_buffer_nuestro();
    buffer_write_uint32(buffer, dirFisica);
    buffer_write_uint32(buffer, cde->pid);
    buffer_write_uint32(buffer, numPagina);
    enviar_buffer(buffer, socket_memoria);
    destruir_buffer_nuestro(buffer);
    
    mensajeCpuMem codigoMemoria = recibir_codigo(socket_memoria);

    if(codigoMemoria == MOV_IN_OK){
        buffer = recibir_buffer(socket_memoria);
        uint32_t valorLeido = buffer_read_uint32(buffer);
        ejecutar_set(registro, valorLeido);

        destruir_buffer_nuestro(buffer);

        log_info(logger_cpu,"PID: %d - Acción: LEER - Dirección Física: %d - Valor: %d", cde->pid, dirFisica, valorLeido);
    }
}

//  Lee el valor del Registro y lo escribe en la dirección física
void ejecutar_mov_out(uint32_t dirLogica, char* registro, t_cde* cde){
    uint32_t valorAEscribir = buscar_valor_registro(registro);

    uint32_t dirFisica = calcular_direccion_fisica(dirLogica, cde);
    uint32_t numPagina = obtener_numero_pagina(dirLogica);

    if(interruption){ // significa que hubo page fault
        return;
    }

    enviar_codigo(socket_memoria, MOV_OUT_SOLICITUD);
    
    // Le manda a memoria la df y el valor del registro encontrado
    t_buffer* buffer = crear_buffer_nuestro();
    buffer_write_uint32(buffer, dirFisica);
    buffer_write_uint32(buffer, valorAEscribir);
    buffer_write_uint32(buffer, cde->pid);
    buffer_write_uint32(buffer, numPagina);
    enviar_buffer(buffer, socket_memoria);
    destruir_buffer_nuestro(buffer);

    // Memoria escribe lo que le llego en la df
    mensajeCpuMem codigoMemoria = recibir_codigo(socket_memoria);

    if(codigoMemoria == MOV_OUT_OK)
        log_info(logger_cpu,"PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d", cde->pid, dirFisica, valorAEscribir);
}

void ejecutar_f_open(char* nombre_archivo, char* modo_apertura){
    interruption = 1;
}

void ejecutar_f_close(char* nombre_archivo){
    interruption = 1;
}

void ejecutar_f_seek(char* nombre_archivo, char* posicion){
    interruption = 1;
}

void ejecutar_f_read(char* nombre_archivo, char* direccion_logica, t_cde* cde){
    uint32_t dirLogica = atoi(direccion_logica);
    uint32_t dirFisica = calcular_direccion_fisica(dirLogica, cde);

    if(interruption){ // significa que hay page fault
        pf_con_funcion_fs = 1;
        return;
    }
    else{
        interruption = 1;
        return;
    }
}

void ejecutar_f_write(char* nombre_archivo, char* direccion_logica, t_cde* cde){
    uint32_t dirLogica = atoi(direccion_logica);
    uint32_t dirFisica = calcular_direccion_fisica(dirLogica, cde);

    if(interruption){ // significa que hay page fault
        pf_con_funcion_fs = 1;
        return;
    }
    else{
        interruption = 1;
        return;
    }
}

void ejecutar_f_truncate(char* nombre_archivo, char* tamanio){
    interruption = 1;
}

void ejecutar_exit(){
    //devolver el cde actualizado al kernel
    //cambiar el valor de interruption
    interruption = 1;
}

// FIN FUNCIONES INSTRUCCIONES


void levantarLogger(){
    logger_cpu = iniciar_logger("./cpu.log", "CPU");
}

void levantarConfig(char* config_path){
    config = iniciar_config(config_path);

    if (config == NULL){
        log_warning(logger_cpu, "Error al levantarConfig");
        exit(1);
    }
    config_cpu.ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    config_cpu.puerto_memoria = config_get_int_value(config,"PUERTO_MEMORIA");
    config_cpu.puerto_escucha_dispatch = config_get_int_value(config,"PUERTO_ESCUCHA_DISPATCH");
    config_cpu.puerto_escucha_interrupt = config_get_int_value(config,"PUERTO_ESCUCHA_INTERRUPT");

}

