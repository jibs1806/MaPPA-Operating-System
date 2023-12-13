#include <kernel.h>

int main(int argc, char* argv[]){
    char* config_path = argv[1]; // TO DO
    inicializarModulo("./kernel.config");
    
    menu(logger_kernel);

    sem_wait(&no_end_kernel);

    terminar_conexiones(4, socket_cpu_dispatch, socket_cpu_interrupt, socket_memoria, socket_file_system);
    terminar_programa(logger_kernel, config);
    
    return 0;
}

void inicializarModulo(char* config_path){
    levantarLogger();
    levantarConfig(config_path);
    
    pid_a_asignar = 0; // para crear pcbs arranca en 0 el siguiente en 1
    planificacion_detenida = 0;
    tengo_que_liberar_archivos = 0;
    
    inicializarListas();
    inicializarSemaforos();

    inicializarConexiones();

    enviar_codigo(socket_cpu_dispatch, ALGORITMO_PLANIFICACION);
    t_buffer* buffer = crear_buffer_nuestro();
    // si el algoritmo es FIFO envia 0
    // si el algoritmo es PRIORIDADES envia un 1
    // si el algoritmo es ROUND ROBIN envia un 2
    if(strcmp(config_kernel.algoritmo, "FIFO") == 0) 
        buffer_write_uint32(buffer, 0);
    else if(strcmp(config_kernel.algoritmo, "PRIORIDADES") == 0)
        buffer_write_uint32(buffer, 1);
    else
        buffer_write_uint32(buffer, 2);
    enviar_buffer(buffer, socket_cpu_dispatch);
    destruir_buffer_nuestro(buffer);

    if(strcmp(config_kernel.algoritmo, "RR") == 0){
        prender_quantum();
    }
    
    iniciarPlanificadores();
    
    return;
}

void inicializarListas(){
    procesos_globales = list_create();
    procesosBloqueadosPageFault = list_create();
    tablaArchivosAbiertos = list_create();
    listaArchivosGlobales = list_create();

    procesosNew = queue_create();
    procesosReady = queue_create();
    procesosBloqueados = queue_create();
    procesosFinalizados = queue_create();

}

void inicializarSemaforos(){
    // Mutex
    pthread_mutex_init(&mutex_new, NULL);
    pthread_mutex_init(&mutex_ready, NULL);
    pthread_mutex_init(&mutex_block, NULL);
    pthread_mutex_init(&mutex_finalizados, NULL);
    pthread_mutex_init(&mutex_exec, NULL);
    pthread_mutex_init(&mutex_procesos_globales, NULL);
    pthread_mutex_init(&mutex_indice, NULL);
    pthread_mutex_init(&mutex_pagefault, NULL);
    pthread_mutex_init(&mutex_archivos_globales, NULL);
    pthread_mutex_init(&mutex_archivos_abiertos, NULL);

    pthread_mutex_init(&mutex_pcb_en_ejecucion, NULL);

    sem_init(&pausar_new_a_ready, 0, 0);
    sem_init(&pausar_ready_a_exec, 0, 0);
    sem_init(&pausar_exec_a_finalizado, 0, 0);
    sem_init(&pausar_exec_a_ready, 0, 0);
    sem_init(&pausar_exec_a_blocked, 0, 0);
    sem_init(&pausar_blocked_a_ready, 0, 0);

    sem_init(&sem_iniciar_quantum, 0, 0);
    sem_init(&sem_reloj_destruido, 0, 1);
    sem_init(&no_end_kernel, 0, 0);
    sem_init(&grado_de_multiprogramacion, 0, config_kernel.grado_max_multiprogramacion);
    sem_init(&procesos_en_new, 0, 0);
    sem_init(&procesos_en_ready, 0, 0);
    sem_init(&procesos_en_blocked, 0, 0);
    sem_init(&procesos_en_exit, 0, 0);
    sem_init(&cpu_libre, 0, 1);
    sem_init(&cont_exec, 0, 0);
    sem_init(&bin_recibir_cde, 0, 0);
    sem_init(&sem_liberar_archivos, 0, 0);
}

void inicializarConexiones(){

    conectarseConCPU();
   
    socket_memoria = inicializar_cliente(config_kernel.ip_memoria, config_kernel.puerto_memoria, logger_kernel);
    socket_file_system = inicializar_cliente(config_kernel.ip_file_system, config_kernel.puerto_file_system, logger_kernel);
}

void conectarseConCPU(){
    socket_cpu_dispatch = inicializar_cliente(config_kernel.ip_cpu, config_kernel.puerto_cpu_dispatch, logger_kernel);
    if(socket_cpu_dispatch >= 0){
        crear_hilos_cpu();
    }
    
    socket_cpu_interrupt = inicializar_cliente(config_kernel.ip_cpu, config_kernel.puerto_cpu_interrupt, logger_kernel);
    if(socket_cpu_interrupt < 0){
        log_error(logger_kernel, "Error al inicializar_cliente en cpu interrupt");
    }
}

void crear_hilos_cpu(){
    pthread_t recibir_de_cpu;
    pthread_create(&recibir_de_cpu, NULL, (void *) recibir_cde_de_cpu, NULL);
    pthread_detach(recibir_de_cpu);
}

void levantarLogger(){
    logger_kernel = iniciar_logger("./kernel.log", "KERNEL");
}

void levantarConfig(char* config_path){
    config = iniciar_config(config_path);

    if (config == NULL){
        log_info(logger_kernel, "Error al levantar el Config");
        exit(1);
    }
    config_kernel.ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    config_kernel.puerto_memoria = config_get_int_value(config,"PUERTO_MEMORIA");
    
    config_kernel.ip_file_system = config_get_string_value(config,"IP_FILESYSTEM");
    config_kernel.puerto_file_system = config_get_int_value(config,"PUERTO_FILESYSTEM");

    config_kernel.ip_cpu = config_get_string_value(config,"IP_CPU");
	config_kernel.puerto_cpu_dispatch = config_get_int_value(config,"PUERTO_CPU_DISPATCH");
    config_kernel.puerto_cpu_interrupt = config_get_int_value(config,"PUERTO_CPU_INTERRUPT");

    config_kernel.algoritmo = config_get_string_value(config,"ALGORITMO_PLANIFICACION");

    config_kernel.quantum = config_get_int_value(config, "QUANTUM");

    config_kernel.grado_max_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION_INI");
    
   	char **recursos = config_get_array_value(config,"RECURSOS");
    
	char **instancias = config_get_array_value(config,"INSTANCIAS_RECURSOS");

	config_kernel.recursos = list_create();

	int a = 0;
	while(recursos[a]!= NULL){
		int num_instancias = strtol(instancias[a],NULL,10);
		t_recurso* recurso = inicializar_recurso(recursos[a], num_instancias);
		list_add(config_kernel.recursos, recurso);
		a++;
	}

    string_array_destroy(recursos);
    string_array_destroy(instancias);
}

void prender_quantum(){
    pthread_t clock_rr;
    pthread_create(&clock_rr, NULL, (void*) controlar_tiempo_de_ejecucion, NULL);
    pthread_detach(clock_rr);
}

t_recurso* inicializar_recurso(char* nombre_recu, int instancias_tot){
	t_recurso* recurso = malloc(sizeof(t_recurso));
	int tam = 0;

	while(nombre_recu[tam] != NULL )
		tam++;

	recurso->nombre = malloc(tam);
	strcpy(recurso->nombre, nombre_recu);

	recurso->instancias = instancias_tot;
    recurso->procesos_bloqueados = list_create();
    sem_t sem;
    sem_init(&sem, instancias_tot, 1);
    recurso->sem_recurso = sem;
	
	return recurso;
}

// UTILS MENSAJES
t_pcb* crear_pcb(char* path, char* prioridad){
    t_pcb* pcb_creado = malloc(sizeof(t_pcb));
    
    int nro_prioridad = atoi(prioridad);

    pcb_creado->cde = malloc(sizeof(t_cde)); // si valgrind tira ml aca esta mal, se hace free de path y prioridad en menu, atender consola!
    pcb_creado->cde->registros = malloc(sizeof(t_registro));

    pcb_creado->cde->pid = pid_a_asignar;
    pcb_creado->cde->pc = 0;
    
    pcb_creado->cde->registros->AX = 0;
    pcb_creado->cde->registros->BX = 0;
    pcb_creado->cde->registros->CX = 0;
    pcb_creado->cde->registros->DX = 0;
    
    pcb_creado->path = path;

    pcb_creado->flag_clock = false;
    pcb_creado->fin_q = false;
    
    pcb_creado->prioridad = nro_prioridad;
    pcb_creado->archivos_abiertos = list_create();
    pcb_creado->archivos_solicitados = list_create();
    pcb_creado->recursos_asignados = list_create();
    pcb_creado->recursos_solicitados = list_create();
    
    pcb_creado->estado = NULO; // pongo asi vemos si nos olvidamos de cambiarle el estado en alguna transicion

    pid_a_asignar ++;
    return pcb_creado;
}

// destruir_cde esta en serializacion.c

void destruir_pcb(t_pcb* pcb){
    destruir_cde(pcb->cde);
    list_destroy(pcb->archivos_abiertos);
    list_destroy(pcb->archivos_solicitados);
    list_destroy(pcb->recursos_asignados);
    list_destroy(pcb->recursos_solicitados);
    free(pcb);
}

void iniciarProceso(char* path, char* size, char* prioridad){
	t_pcb* pcb_a_new = crear_pcb(path, prioridad);

    enviar_codigo(socket_memoria, INICIAR_PROCESO_SOLICITUD);

    t_buffer* buffer = crear_buffer_nuestro();

    buffer_write_uint32(buffer, pcb_a_new->cde->pid);
    buffer_write_string(buffer, path); // el path no se esta eliminando en menu.c ?? Por ahora anda si pasa algo puede ser esto !!!
    uint32_t tamanio = atoi(size);
    buffer_write_uint32(buffer, tamanio);
	
    enviar_buffer(buffer, socket_memoria);

    destruir_buffer_nuestro(buffer);

    mensajeKernelMem cod_op = recibir_codigo(socket_memoria);

    if(cod_op == INICIAR_PROCESO_OK){
        // Poner en new
        agregar_pcb_a(procesosNew, pcb_a_new, &mutex_new);

        pthread_mutex_lock(&mutex_procesos_globales);
        list_add(procesos_globales, pcb_a_new); // agrego a lista global de pcbs
        pthread_mutex_unlock(&mutex_procesos_globales);

        pcb_a_new->estado = NEW;
        log_info(logger_kernel, "Se crea el proceso %d en NEW", pcb_a_new->cde->pid);
        sem_post(&procesos_en_new);
    }
    else if(cod_op == INICIAR_PROCESO_ERROR)
        log_info(logger_kernel, "No se pudo crear el proceso %d", pcb_a_new->cde->pid);
    
}

void terminar_proceso(){
    while(1){
        sem_wait(&procesos_en_exit);

        t_pcb* pcb = retirar_pcb_de(procesosFinalizados, &mutex_finalizados);

        pthread_mutex_lock(&mutex_procesos_globales);
	    list_remove_element(procesos_globales, pcb);
	    pthread_mutex_unlock(&mutex_procesos_globales);
                
        liberar_recursos_pcb(pcb);
        liberar_archivos_pcb(pcb);

        // Solicitar a memoria liberar estructuras
        enviar_codigo(socket_memoria, FINALIZAR_PROCESO_SOLICITUD);

        t_buffer* buffer = crear_buffer_nuestro();
        buffer_write_uint32(buffer, pcb->cde->pid);
        enviar_buffer(buffer, socket_memoria);
        destruir_buffer_nuestro(buffer);

        mensajeKernelMem rta_memoria = recibir_codigo(socket_memoria);

        if(rta_memoria == FINALIZAR_PROCESO_OK){
            log_info(logger_kernel, "PID: %d - Destruir PCB", pcb->cde->pid);
            destruir_pcb(pcb);
        }
        else{
            log_error(logger_kernel, "Memoria no logró liberar correctamente las estructuras");
            exit(1);
        }
        
    }
}

void liberar_recursos_pcb(t_pcb* pcb){
    t_recurso* recurso;
    for(int i = 0; i < list_size(pcb->recursos_asignados); i++){
        recurso = list_get(pcb->recursos_asignados, i);
        signal_recursos_asignados_pcb(pcb, recurso->nombre);
    }
    
    while(list_size(pcb->recursos_asignados) != 0){
        list_remove(pcb->recursos_asignados, 0);
    }

    while(list_size(pcb->recursos_solicitados) != 0){ // Aca entraria solo en el caso de que se finalice el proceso por consola
        recurso = list_remove(pcb->recursos_solicitados, 0);
        list_remove_element(recurso->procesos_bloqueados, pcb);
    }
}

void signal_recursos_asignados_pcb(t_pcb* pcb, char* nombre_recurso_pedido){
    int posicion_recurso;
    for(int i=0; i < list_size(config_kernel.recursos); i++){
        t_recurso* recurso = list_get(config_kernel.recursos, i);
        char* nombre_recurso = recurso->nombre;
        if(strcmp(nombre_recurso_pedido, nombre_recurso) == 0){
            posicion_recurso = i;
            break;
        }
    }
    t_recurso* recurso = list_get(config_kernel.recursos, posicion_recurso);
    recurso->instancias++; //podria considerarse chequear que no se pase de las instancias totales del recurso, pero me parecio innecesario
    log_info(logger_kernel, "PID: %d - LIBERANDO INSTANCIAS DEL RECURSO: %s - INSTANCIAS DISPONIBLES: %d", pcb->cde->pid, nombre_recurso_pedido, recurso->instancias);

    if(list_size(recurso->procesos_bloqueados) > 0){ // Desbloquea al primer proceso de la cola de bloqueados del recurso
	    sem_t semaforo_recurso = recurso->sem_recurso;

	    sem_wait(&semaforo_recurso);

		t_pcb* pcb_a_retirar = list_remove(recurso->procesos_bloqueados, 0);
		
		sem_post(&semaforo_recurso);
        
        // le asigno al pcb que se "libero" el recurso asi puede ejecutar
        recurso->instancias--;
        list_add(pcb_a_retirar->recursos_asignados, recurso);
        
        enviar_pcb_de_block_a_ready(pcb_a_retirar);
	}
}

void liberar_archivos_pcb(t_pcb* pcb){
    t_archivo_abierto* archivo_abierto;
    t_archivo* archivo;
    
    for(int i = 0; i < list_size(pcb->archivos_abiertos); i++){ // aca entraria solamente si se finaliza un proceso por consola
        tengo_que_liberar_archivos = 1;
        archivo_abierto = list_get(pcb->archivos_abiertos, i);
        sem_post(&sem_liberar_archivos);
        ejecutar_f_close(archivo_abierto->archivo->nombre, pcb);
    }

    while(list_size(pcb->archivos_solicitados) != 0){ // Aca entraria solo en el caso de que se finalice el proceso por consola
        archivo = list_remove(pcb->archivos_solicitados, 0);
        list_remove_element(archivo->procesos_bloqueados->elements, pcb);
    }

    tengo_que_liberar_archivos = 0;
}

t_pcb* encontrar_pcb_por_pid(uint32_t pid, int* encontrado){
    t_pcb* pcb;
    int i = 0;
    
    *(encontrado) = 0;

    for(int i = 0; i < list_size(procesos_globales); i++){
        pcb = list_get(procesos_globales, i);
        if(pcb->cde->pid == pid){
            *(encontrado) = 1;
            break;
        }
    }

    if(*(encontrado))
        return pcb;
    else
        log_warning(logger_kernel, "PCB no encontrado de PID: %d", pid);
}

void retirar_pcb_de_su_respectivo_estado(uint32_t pid, int* resultado){
    t_pcb* pcb_a_retirar = encontrar_pcb_por_pid(pid, resultado);

    if(resultado){
        switch(pcb_a_retirar->estado){
            case NEW:
                sem_wait(&procesos_en_new);
                pthread_mutex_lock(&mutex_new);
                list_remove_element(procesosNew->elements, pcb_a_retirar);
                pthread_mutex_unlock(&mutex_new);
                enviar_a_finalizado(pcb_a_retirar, "EXIT POR CONSOLA");
                break;
            case READY:
                sem_wait(&procesos_en_ready);
                pthread_mutex_lock(&mutex_ready);
                list_remove_element(procesosReady->elements, pcb_a_retirar);
                pthread_mutex_unlock(&mutex_ready);
                enviar_a_finalizado(pcb_a_retirar, "EXIT POR CONSOLA");
                break;
            case BLOCKED:
                sem_wait(&procesos_en_blocked);
                pthread_mutex_lock(&mutex_block);
                list_remove_element(procesosBloqueados->elements, pcb_a_retirar);
                pthread_mutex_unlock(&mutex_block);
                enviar_a_finalizado(pcb_a_retirar, "EXIT POR CONSOLA");
                break;
            case EXEC:
                enviar_codigo(socket_cpu_interrupt, INTERRUPT);
                
                t_buffer* buffer = crear_buffer_nuestro();
                buffer_write_uint32(buffer, pcb_a_retirar->cde->pid);
                enviar_buffer(buffer, socket_cpu_interrupt);
                destruir_buffer_nuestro(buffer);
                break;
            case FINISH:
                // ?? hace falta hacer algo?? dsps ver
                break;
            default:
                log_error(logger_kernel, "Entre al default en estado nro: %d", pcb_a_retirar->estado);
                break;
        
        }
    }
    else
        log_warning(logger_kernel, "No ejecute el switch");
}

void finalizarProceso(char* pid_string){
    // Antes de hacer finalizar proceso, hay que ver que como desde todos los estados podes pasar a exit, quiza conviene 
    // tener el estado en el pcb, para saber en que lista lo sacas? 
    // Aunque sin este parametro se puede hacer quiza es un poco mas rebuscado?

    int resultado = 0;
    uint32_t pid = atoi(pid_string);
    retirar_pcb_de_su_respectivo_estado(pid, &resultado);    
    // detectar deadlock se llama en menu.c
    return;
}

void detenerPlanificacion(){ 
    planificacion_detenida = 1;
}

void iniciarPlanificacion(){
    sem_post(&pausar_new_a_ready);
    if(pausar_new_a_ready.__align == 1)
        sem_wait(&pausar_new_a_ready);
    
    sem_post(&pausar_ready_a_exec);
    if(pausar_ready_a_exec.__align == 1)
        sem_wait(&pausar_ready_a_exec);
    
    sem_post(&pausar_exec_a_finalizado);
    if(pausar_exec_a_finalizado.__align == 1)
        sem_wait(&pausar_exec_a_finalizado);
    
    sem_post(&pausar_exec_a_ready);
    if(pausar_exec_a_ready.__align == 1)
        sem_wait(&pausar_exec_a_ready);
    
    sem_post(&pausar_exec_a_blocked);
    if(pausar_exec_a_blocked.__align == 1)
        sem_wait(&pausar_exec_a_blocked);
    
    sem_post(&pausar_blocked_a_ready);
    if(pausar_blocked_a_ready.__align == 1)
        sem_wait(&pausar_blocked_a_ready);
    
    planificacion_detenida = 0;
}

void procesosPorEstado(){
    log_info(logger_kernel, "---------LISTANDO PROCESOS POR ESTADO---------");

    char* procesos_cargados_en_new = obtener_elementos_cargados_en(procesosNew);
    char* procesos_cargados_en_ready = obtener_elementos_cargados_en(procesosReady);
    char* procesos_cargados_en_blocked = obtener_elementos_cargados_en(procesosBloqueados);
    char* procesos_cargados_en_exit = obtener_elementos_cargados_en(procesosFinalizados);

    log_info(logger_kernel, "Procesos en %s: %s", obtener_nombre_estado(NEW), procesos_cargados_en_new);
    log_info(logger_kernel, "Procesos en %s: %s", obtener_nombre_estado(READY), procesos_cargados_en_ready);
    if(pcb_en_ejecucion != NULL)
        log_info(logger_kernel, "Proceso en %s: [%d]", obtener_nombre_estado(EXEC), pcb_en_ejecucion->cde->pid);
    else
        log_info(logger_kernel, "Proceso en %s: []", obtener_nombre_estado(EXEC));
    log_info(logger_kernel, "Procesos en %s: %s", obtener_nombre_estado(BLOCKED), procesos_cargados_en_blocked);
    log_info(logger_kernel, "Procesos en %s: %s",  obtener_nombre_estado(FINISH), procesos_cargados_en_exit);

    free(procesos_cargados_en_new);
    free(procesos_cargados_en_ready);
    free(procesos_cargados_en_blocked);
    free(procesos_cargados_en_exit);
}

// FIN UTILS MENSAJES

// IDA Y VUELTA CPU

void enviar_cde_a_cpu(){
    mensajeKernelCpu codigo = EJECUTAR_PROCESO;
    enviar_codigo(socket_cpu_dispatch, codigo);

    t_buffer* buffer_dispatch = crear_buffer_nuestro();
    pthread_mutex_lock(&mutex_pcb_en_ejecucion);
    buffer_write_cde(buffer_dispatch, pcb_en_ejecucion->cde);
    pthread_mutex_unlock(&mutex_pcb_en_ejecucion);

    if(strcmp(config_kernel.algoritmo, "RR") == 0){
        pcb_en_ejecucion->flag_clock = false;
    }

    enviar_buffer(buffer_dispatch, socket_cpu_dispatch);
    destruir_buffer_nuestro(buffer_dispatch);
    sem_post(&bin_recibir_cde);
}

void controlar_tiempo_de_ejecucion(){  
    while(1){
        sem_wait(&sem_iniciar_quantum);

        uint32_t pid_pcb_before_start_clock = pcb_en_ejecucion->cde->pid;
        bool flag_clock_pcb_before_start_clock = pcb_en_ejecucion->flag_clock;

        usleep(config_kernel.quantum * 1000);

        if(pcb_en_ejecucion != NULL)
            pcb_en_ejecucion->fin_q = true;

        if(pcb_en_ejecucion != NULL && pid_pcb_before_start_clock == pcb_en_ejecucion->cde->pid && flag_clock_pcb_before_start_clock == pcb_en_ejecucion->flag_clock){
            enviar_codigo(socket_cpu_interrupt, DESALOJO);

            t_buffer* buffer = crear_buffer_nuestro();
            buffer_write_uint32(buffer, pcb_en_ejecucion->cde->pid); // lo enviamos porque interrupt recibe un buffer, pero no hacemos nada con esto
            enviar_buffer(buffer, socket_cpu_interrupt);
            destruir_buffer_nuestro(buffer);
        }
        sem_post(&sem_reloj_destruido);
    }
}

void recibir_cde_de_cpu(){
    while(1){
        sem_wait(&bin_recibir_cde);

        t_buffer* buffer = recibir_buffer(socket_cpu_dispatch);

        pthread_mutex_lock(&mutex_exec);
        destruir_cde(pcb_en_ejecucion->cde);        
        pcb_en_ejecucion->cde = buffer_read_cde(buffer);
        pthread_mutex_unlock(&mutex_exec);

        t_instruccion* instruccion_actual = buffer_read_instruccion(buffer);
        
        if(strcmp(config_kernel.algoritmo, "RR") == 0 && pcb_en_ejecucion->fin_q && (instruccion_actual->codigo == MOV_IN || instruccion_actual->codigo == MOV_OUT)){
            log_info(logger_kernel, "PID: %d - Desalojado por fin de Quantum", pcb_en_ejecucion->cde->pid);
            enviar_de_exec_a_ready();
            destruir_instruccion(instruccion_actual);
        }

        else if(instruccion_actual->codigo == MOV_IN || instruccion_actual->codigo == MOV_OUT){
            uint32_t nro_pagina = buffer_read_uint32(buffer);
            pthread_t hilo_page_fault;
            pthread_create(&hilo_page_fault, NULL, (void *) recibir_page_fault, (void *) &nro_pagina);
            pthread_detach(hilo_page_fault);
            destruir_instruccion(instruccion_actual);
        }
        else if(instruccion_actual->codigo == F_READ || instruccion_actual->codigo == F_WRITE){
            uint8_t motivo = buffer_read_uint8(buffer);
            if(motivo == HAY_PAGE_FAULT){
                uint32_t nro_pagina = buffer_read_uint32(buffer);          
                pthread_t hilo_page_fault;
                pthread_create(&hilo_page_fault, NULL, (void *) recibir_page_fault, (void *) &nro_pagina);
                pthread_detach(hilo_page_fault);
                destruir_instruccion(instruccion_actual);
                }
            else if(motivo == DIRECCION_FISICA_OK){
                uint32_t dir_fisica = buffer_read_uint32(buffer);
                uint32_t nro_pagina = buffer_read_uint32(buffer);
                instruccion_actual->par2 = string_itoa(dir_fisica);
                instruccion_actual->par3 = string_itoa(nro_pagina);
                evaluar_instruccion(instruccion_actual);
            }
        }
        else{
            evaluar_instruccion(instruccion_actual);
        }
        
        destruir_buffer_nuestro(buffer);
    }
}

void recibir_page_fault(void* args){
    uint32_t nro_pagina = *((uint32_t *)args);
    
    pthread_mutex_lock(&mutex_pagefault);
    list_add(procesosBloqueadosPageFault, pcb_en_ejecucion);
    pthread_mutex_unlock(&mutex_pagefault);

    t_pcb* pcb_page_fault = pcb_en_ejecucion;
    enviar_de_exec_a_block();

    log_info(logger_kernel, "Page Fault PID: %d - Pagina: %d", pcb_page_fault->cde->pid, nro_pagina);

    // No comprometer recursos compartidos??
    enviar_codigo(socket_memoria, PAGE_FAULT_SOLICITUD);

    t_buffer* buffer = crear_buffer_nuestro();
    buffer_write_uint32(buffer, nro_pagina);
    buffer_write_uint32(buffer, pcb_page_fault->cde->pid);
    enviar_buffer(buffer, socket_memoria);
    destruir_buffer_nuestro(buffer);
    
    // recibimos el resultado de memoria
    mensajeKernelMem rta_memoria = recibir_codigo(socket_memoria);
    
    switch(rta_memoria){
        case PAGE_FAULT_OK:
            pthread_mutex_lock(&mutex_pagefault);
            list_remove_element(procesosBloqueadosPageFault, pcb_page_fault);
            pthread_mutex_unlock(&mutex_pagefault);
            enviar_pcb_de_block_a_ready(pcb_page_fault);
            break;
        default:
            log_warning(logger_kernel, "Respuesta no esperada por parte de Memoria");
            break;
    }
}

void evaluar_instruccion(t_instruccion* instruccion_actual){
    switch(instruccion_actual->codigo){
        case WAIT:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            } // si se bloquea tengo que reiniciar el clock para el nuevo proceso (solo en RR)
            char* nombre_recurso_wait = instruccion_actual->par1;
            evaluar_wait(nombre_recurso_wait);
            destruir_instruccion(instruccion_actual);
            break;
        case SIGNAL:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            char* nombre_recurso_signal = instruccion_actual->par1;
            evaluar_signal(nombre_recurso_signal);
            destruir_instruccion(instruccion_actual);
            break;
        case SLEEP:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            uint32_t tiempo_sleep = atoi(instruccion_actual->par1);
            t_pcb* pcb_a_dormir = pcb_en_ejecucion;
            enviar_de_exec_a_block();
            destruir_instruccion(instruccion_actual);
            evaluar_sleep(pcb_a_dormir, tiempo_sleep);
            break;
        case F_OPEN:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            // Tenemos que abrir el archivo en el modo indicado
            char* nombreArchivo = string_new();
            char* modoApertura = string_new();
            string_append(&nombreArchivo, instruccion_actual->par1);
            string_append(&modoApertura, instruccion_actual->par2);
            ejecutar_f_open(nombreArchivo, modoApertura, pcb_en_ejecucion);
            destruir_instruccion(instruccion_actual);
            // hacer los frees
            break;
        case F_CLOSE:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            // Tenemos que cerrar el archivo indicado
            ejecutar_f_close(instruccion_actual->par1, pcb_en_ejecucion);
            destruir_instruccion(instruccion_actual);
            break;
        case F_SEEK:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            uint32_t nueva_posicion = atoi(instruccion_actual->par2);
            ejecutar_f_seek(instruccion_actual->par1, nueva_posicion);
            destruir_instruccion(instruccion_actual);
            if(strcmp(config_kernel.algoritmo, "RR") == 0 && !pcb_en_ejecucion->fin_q)
                enviar_cde_a_cpu();
            else if(strcmp(config_kernel.algoritmo, "RR") == 0 && pcb_en_ejecucion->fin_q)
                enviar_de_exec_a_ready();
            else
                enviar_cde_a_cpu();
            break;
        case F_READ:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            uint32_t direccion_fisica_para_read = atoi(instruccion_actual->par2);
            uint32_t numero_pagina_para_read = atoi(instruccion_actual->par3);
            ejecutar_f_read(instruccion_actual->par1, direccion_fisica_para_read, numero_pagina_para_read);
            destruir_instruccion(instruccion_actual);
            // se hace solo el post de bin_enviar_cde porque mandamos el pcb_en_ejecucion al estado bloqueado
            break;
        case F_WRITE:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            uint32_t direccion_fisica_para_write = atoi(instruccion_actual->par2);
            uint32_t numero_pagina_para_write = atoi(instruccion_actual->par3);
            ejecutar_f_write(instruccion_actual->par1, direccion_fisica_para_write, numero_pagina_para_write);
            destruir_instruccion(instruccion_actual);
            // se hace solo el post de bin_enviar_cde porque mandamos el pcb_en_ejecucion al estado bloqueado
            break;
        case F_TRUNCATE:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            pthread_t hilo_f_truncate;
            pthread_create(&hilo_f_truncate, NULL, (void *) ejecutar_f_truncate, (void *) instruccion_actual);
            pthread_detach(hilo_f_truncate);
            //destruir_instruccion(instruccion_actual);
            break;
        case EXIT:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            enviar_de_exec_a_finalizado("SUCCESS");
            destruir_instruccion(instruccion_actual);
            break;
        case EXIT_POR_CONSOLA:
            if(strcmp(config_kernel.algoritmo, "RR") == 0){
                pcb_en_ejecucion->flag_clock = true;
            }
            enviar_de_exec_a_finalizado("EXIT POR CONSOLA");
            destruir_instruccion(instruccion_actual);
            break;
        default: // es por fin de quantum
            if(strcmp(config_kernel.algoritmo, "RR") == 0)
                log_info(logger_kernel, "PID: %d - Desalojado por fin de Quantum", pcb_en_ejecucion->cde->pid);
            else if(strcmp(config_kernel.algoritmo, "PRIORIDADES") == 0)
                log_info(logger_kernel, "PID: %d - Desalojado por un proceso de mayor prioridad", pcb_en_ejecucion->cde->pid);
            enviar_de_exec_a_ready();
            destruir_instruccion(instruccion_actual);
            break;
    }
}

void evaluar_wait(char* nombre_recurso_pedido){
    int coincidencia = 0;
    int posicion_recurso;
    for(int i=0; i < list_size(config_kernel.recursos); i++){
        t_recurso* recurso = list_get(config_kernel.recursos, i);
        char* nombre_recurso = recurso->nombre;
        if(string_equals_ignore_case(nombre_recurso_pedido, nombre_recurso)){
            coincidencia++;
            posicion_recurso = i;
        }
    }
    if(coincidencia>0){ // el recurso existe
        t_recurso* recurso = list_get(config_kernel.recursos, posicion_recurso);
        recurso->instancias--;
        log_info(logger_kernel, "PID: %d - Wait: %s - Instancias: %d", pcb_en_ejecucion->cde->pid, nombre_recurso_pedido, recurso->instancias);
        
        if(recurso->instancias < 0){  // Chequea si debe bloquear al proceso por falta de instancias

            sem_t semaforo_recurso = recurso->sem_recurso;
        	sem_wait(&semaforo_recurso);

        	list_add(recurso->procesos_bloqueados, pcb_en_ejecucion);
        	recurso->instancias = 0;
        	sem_post(&semaforo_recurso);

            log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb_en_ejecucion->cde->pid, nombre_recurso_pedido);

            list_add(pcb_en_ejecucion->recursos_solicitados, recurso);
            
            enviar_de_exec_a_block();
        }
        else{
            list_add(pcb_en_ejecucion->recursos_asignados, recurso);

            if(strcmp(config_kernel.algoritmo, "RR") == 0 && !pcb_en_ejecucion->fin_q)
                enviar_cde_a_cpu();
            else if(strcmp(config_kernel.algoritmo, "RR") == 0 && pcb_en_ejecucion->fin_q)
                enviar_de_exec_a_ready();
            else
                enviar_cde_a_cpu();
        }
    }
    else{ // el recurso no existe
        enviar_de_exec_a_finalizado("INVALID_RESOURCE"); //TODO: codigo de inexistencia de recurso
    }
}

void evaluar_signal(char* nombre_recurso_pedido){
    int coincidencia = 0;
    int posicion_recurso;
    int asignado = 0;
    for(int i=0; i < list_size(config_kernel.recursos); i++){ //obtiene la posicion del recurso si existe
        t_recurso* recurso = list_get(config_kernel.recursos, i);
        char* nombre_recurso = recurso->nombre;
        if(string_equals_ignore_case(nombre_recurso_pedido, nombre_recurso)){
            coincidencia++;
            posicion_recurso = i;
        }
    }
    for(int i=0; i < list_size(pcb_en_ejecucion->recursos_asignados); i++){ //se fija si lo tiene asignado
        t_recurso* recurso = list_get(pcb_en_ejecucion->recursos_asignados, i);
        char* nombre_recurso = recurso->nombre;
        if(string_equals_ignore_case(nombre_recurso_pedido, nombre_recurso)){
            asignado++;
        }
    }
    if(coincidencia>0 && asignado>0){ // el recurso existe y lo tiene asignado
        t_recurso* recurso = list_get(config_kernel.recursos, posicion_recurso);
        recurso->instancias++; //podria considerarse chequear que no se pase de las instancias totales del recurso, pero me parecio innecesario
        log_info(logger_kernel, "PID: %d - Signal: %s - Instancias: %d", pcb_en_ejecucion->cde->pid, nombre_recurso_pedido, recurso->instancias);
        
        list_remove_element(pcb_en_ejecucion->recursos_asignados, recurso); // saco el recurso porque lo libero
        
        if(strcmp(config_kernel.algoritmo, "RR") == 0 && !pcb_en_ejecucion->fin_q)
            enviar_cde_a_cpu();
        else if(strcmp(config_kernel.algoritmo, "RR") == 0 && pcb_en_ejecucion->fin_q)
            enviar_de_exec_a_ready();
        else
            enviar_cde_a_cpu();

        if(list_size(recurso->procesos_bloqueados) > 0){ // Desbloquea al primer proceso de la cola de bloqueados del recurso
			sem_t semaforo_recurso = recurso->sem_recurso;

			sem_wait(&semaforo_recurso);

			t_pcb* pcb = list_remove(recurso->procesos_bloqueados, 0);
			sem_post(&semaforo_recurso);

            list_remove_element(pcb->recursos_solicitados, recurso); // saco el recurso que solicito el pcb y ya se le puede asignar
            // para mi no hay que destruirlo, porque estariamos destruyendo el recurso en todo el programa
            
            list_add(pcb->recursos_asignados, recurso);
                        
            recurso->instancias--;

            enviar_pcb_de_block_a_ready(pcb);
		}
    }
    else if(coincidencia>0 && asignado==0){ // el recurso existe y NO lo tiene asignado
        enviar_de_exec_a_finalizado("INVALID_RESOURCE"); 
    }
    else if(coincidencia==0 && asignado==0){ // el recurso NO existe y NO lo tiene asignado
        enviar_de_exec_a_finalizado("INVALID_RESOURCE"); //TODO: codigo de inexistencia de recurso
    }
}

void sleep_pcb(void* args){
    t_pcb_durmiendo* proceso_durmiendo = (t_pcb_durmiendo*) args;
    usleep(proceso_durmiendo->tiempo_del_sleep * 1000000);
    if(esta_proceso_en_cola_bloqueados(proceso_durmiendo->pcb_a_dormir) == -1){
        sem_wait(&procesos_en_blocked);
        return;
    }
    enviar_pcb_de_block_a_ready(proceso_durmiendo->pcb_a_dormir);
}

void evaluar_sleep(t_pcb* pcb, uint32_t tiempo_sleep){
    log_info(logger_kernel, "PID: %d - Bloqueado por: SLEEP", pcb->cde->pid);

    t_pcb_durmiendo* pcb_a_sleepear = malloc(sizeof(t_pcb_durmiendo));
    pcb_a_sleepear->pcb_a_dormir = pcb;
    pcb_a_sleepear->tiempo_del_sleep = tiempo_sleep;

    pthread_t hilo_sleep;
    pthread_create(&hilo_sleep, NULL, (void*) sleep_pcb, (void* ) pcb_a_sleepear);
    pthread_detach(hilo_sleep);
}

void enviar_a_finalizado(t_pcb* pcb_a_finalizar, char* razon){
    agregar_pcb_a(procesosFinalizados, pcb_a_finalizar, &mutex_finalizados);
    log_info(logger_kernel, "PID: %d - Estado anterior: %s - Estado actual: %s", pcb_a_finalizar->cde->pid, obtener_nombre_estado(pcb_a_finalizar->estado), obtener_nombre_estado(FINISH)); //OBLIGATORIO
    log_info(logger_kernel, "Finaliza el proceso %d - Motivo: %s", pcb_a_finalizar->cde->pid, razon); // OBLIGATORIO
    sem_post(&procesos_en_exit);
    if (pcb_a_finalizar->estado == READY || pcb_a_finalizar->estado == BLOCKED)
        sem_post(&grado_de_multiprogramacion); //Como se envia a EXIT, se "libera" 1 grado de multiprog
}

char* obtener_nombre_estado(t_estados estado){
    switch(estado){
        case NEW:
            return "NEW";
            break;
        case READY:
            return "READY";
            break;
        case EXEC:
            return "EXEC";
            break;
        case BLOCKED:
            return "BLOCKED";
            break;
        case FINISH:
            return "EXIT";
            break;
        default:
            return "NULO";
            break;
    }
}

// FIN IDA Y VUELTA CPU

// ------------------ PLANIFICADORES ----------------------

void iniciarPlanificadores(){
    planificadorLargoPlazo();
    planificadorCortoPlazo();
}

void planificadorLargoPlazo(){
    pthread_t hiloNew;
	pthread_t hiloExit;

	pthread_create(&hiloNew, NULL, (void *) enviar_de_new_a_ready, NULL);
	pthread_create(&hiloExit, NULL, (void *) terminar_proceso, NULL);

	pthread_detach(hiloNew);
	pthread_detach(hiloExit);
}

void planificadorCortoPlazo(){
    // Se crean los hilos necesarios
    pthread_t poner_en_ejecucion;
	pthread_create(&poner_en_ejecucion, NULL, (void *) enviar_de_ready_a_exec, NULL);
	pthread_detach(poner_en_ejecucion);

}

void cambiar_grado_multiprogramacion(char* nuevo_grado){
    //para cambiarlo la planificacion debe estar detendida
    int grado_a_asignar = atoi(nuevo_grado);
    if(planificacion_detenida == 1){
        // se puede cambiar el grado de multiprogramacion
        
        grado_de_multiprogramacion.__align = grado_a_asignar - config_kernel.grado_max_multiprogramacion + grado_de_multiprogramacion.__align - 1;
        sem_post(&grado_de_multiprogramacion);
        config_kernel.grado_max_multiprogramacion = grado_a_asignar; // el grado_max_multiprog se actualiza para siempre contener el grado maximo de multirpg actual
    }
    else{
        // no se puede realizar el cambio
        log_warning(logger_kernel, "La planificacion no se detuvo. No se puede cambiar el grado de multiprogramacion");
    }
}

// ----------------- FIN PLANIFICADORES -------------------

// UTILS COLAS DE ESTADOS
void agregar_pcb_a(t_queue* cola, t_pcb* pcb_a_agregar, pthread_mutex_t* mutex){
    
    pthread_mutex_lock(mutex);
    queue_push(cola, (void*) pcb_a_agregar);
	pthread_mutex_unlock(mutex);

}

t_pcb* retirar_pcb_de(t_queue* cola, pthread_mutex_t* mutex){
    
	pthread_mutex_lock(mutex);
	t_pcb* pcb = queue_pop(cola);
	pthread_mutex_unlock(mutex);
    
	return pcb;
}

// FIN UTILS COLAS DE ESTADOS

