#include <manejo_fs.h>

// FUNCIONES DE ARCHIVOS FS

t_archivo_abierto* crear_archivo_abierto(t_archivo* archivo){
    t_archivo_abierto* archivo_abierto = malloc(sizeof(t_archivo_abierto));
    archivo_abierto->archivo = archivo;
    archivo_abierto->puntero_archivo = 0;

    return archivo_abierto;
}

t_archivo* crear_archivo_local(char* nombre_archivo){
    t_archivo* archivo = malloc(sizeof(t_archivo));
    archivo->nombre = nombre_archivo;
    archivo->tamanio = 0;
    sem_init(&archivo->lock_lectura, 0, 0);
    sem_init(&archivo->lock_escritura, 0, 0);
    sem_init(&archivo->sem_archivo, 0, 1);
    archivo->procesos_bloqueados_para_lectura = queue_create();
    archivo->procesos_bloqueados_para_escritura = queue_create();
    archivo->participantes_del_lock = queue_create();
    archivo->procesos_bloqueados = queue_create();

    return archivo;
}

void crear_archivo(char* nombre_archivo){
    enviar_codigo(socket_file_system, CREAR_ARCHIVO_SOLICITUD);
    t_buffer* buffer = crear_buffer_nuestro();
    buffer_write_string(buffer, nombre_archivo);
    enviar_buffer(buffer, socket_file_system);
    destruir_buffer_nuestro(buffer);

    mensajeKernelFile rta_crear_archivo = recibir_codigo(socket_file_system);
            
    if(rta_crear_archivo == CREAR_ARCHIVO_OK){
        t_archivo* archivo = crear_archivo_local(nombre_archivo);
        pthread_mutex_lock(&mutex_archivos_globales);
        list_add(listaArchivosGlobales, archivo);
        list_add(tablaArchivosAbiertos, archivo);
        pthread_mutex_unlock(&mutex_archivos_globales);
    }
    else
        log_warning(logger_kernel, "No se creo correctamente el archivo en File System"); 
}

bool esta_abierto(char* nombre_archivo){
    for(int i = 0; i < list_size(tablaArchivosAbiertos); i++){
        t_archivo* archivo = list_get(tablaArchivosAbiertos, i);
        if(strcmp(archivo->nombre, nombre_archivo) == 0)
            return true;
    }
    return false;
}

t_archivo* encontrar_archivo(char* nombre_archivo, t_list* lista_archivo){ // siempre lo va a encontrar debido a que ya se verifico antes de que exista
    for(int i = 0; i < list_size(lista_archivo); i++){
        t_archivo* archivo = list_get(lista_archivo, i);
        if(strcmp(archivo->nombre, nombre_archivo) == 0)
            return archivo;
    }
    return NULL;
}

t_archivo_abierto* encontrar_archivo_abierto(char* nombre_archivo, t_list* lista_archivo){ // siempre lo va a encontrar debido a que ya se verifico antes de que exista
    for(int i = 0; i < list_size(lista_archivo); i++){
        t_archivo_abierto* archivo_abierto = list_get(lista_archivo, i);
        if(strcmp(archivo_abierto->archivo->nombre, nombre_archivo) == 0)
            return archivo_abierto;
    }
    return NULL;
}

bool esta_pcb_en(uint32_t pid, t_queue* cola_archivo){ // siempre lo va a encontrar debido a que ya se verifico antes de que exista
    for(int i = 0; i < list_size(cola_archivo->elements); i++){
        uint32_t pid_bloqueado = list_get(cola_archivo->elements, i);
        if(pid_bloqueado == pid)
            return true;
    }
    return false;
}

t_modo_apertura motivo_bloqueo(t_archivo* archivo, t_pcb* pcb){
    // si el pcb no esta bloqueado por escritura, tiene que estar bloqueado por lectura si o si
    if(esta_pcb_en(pcb->cde->pid, archivo->procesos_bloqueados_para_escritura))
        return ESCRITURA;
    else
        return LECTURA;
}

// FIN FUNCIONES DE ARCHIVOS FS

// FUNCIONES DE FILE-SYSTEM
void ejecutar_f_open(char* nombre_archivo, char* modo_apertura, t_pcb* pcb){ // cuando ejecuta por 2da vez falla, no encuentra archivo en tablaARchivosAbiertos

    // intentamos abrir el archivo directamente
    enviar_codigo(socket_file_system, ABRIR_ARCHIVO_SOLICITUD);
    t_buffer* buffer = crear_buffer_nuestro();
    buffer_write_string(buffer, nombre_archivo);
    enviar_buffer(buffer, socket_file_system);
    destruir_buffer_nuestro(buffer);

    mensajeKernelFile codigo_recibido = recibir_codigo(socket_file_system);

    switch((codigo_recibido)){
    case ABRIR_ARCHIVO_OK:
        abrir_archivo(nombre_archivo, modo_apertura, pcb);
        break;
    case NO_EXISTE_ARCHIVO:
        crear_archivo(nombre_archivo);
        ejecutar_f_open(nombre_archivo, modo_apertura, pcb);
        break;
    default:
        log_warning(logger_kernel, "Codigo recibido por default de F_OPEN: %d", codigo_recibido);
        break;
    }
}

void abrir_archivo(char* nombre_archivo, char* modo_apertura, t_pcb* pcb){
    t_archivo* archivo;
    t_buffer* buffer = recibir_buffer(socket_file_system);
    uint32_t tamanio_archivo = buffer_read_uint32(buffer);
    destruir_buffer_nuestro(buffer);
    
    if(esta_abierto(nombre_archivo)){ // el archivo ya esta abierto
        archivo = encontrar_archivo(nombre_archivo, tablaArchivosAbiertos);
        // asignamos el tamanio del archivo recibido por file system
        archivo->tamanio = tamanio_archivo;
    }
    else if(!esta_abierto(nombre_archivo) && encontrar_archivo(nombre_archivo, listaArchivosGlobales) != NULL){        
        archivo = encontrar_archivo(nombre_archivo, listaArchivosGlobales);
        // asignamos el tamanio del archivo recibido por file system
        archivo->tamanio = tamanio_archivo;

        pthread_mutex_lock(&mutex_archivos_abiertos);
        list_add(tablaArchivosAbiertos, archivo);
        pthread_mutex_unlock(&mutex_archivos_abiertos);
    }
    else if(encontrar_archivo(nombre_archivo, listaArchivosGlobales) == NULL){ // el archivo no esta abierto, hay que agregarlo a la lista de archivos abiertos
        archivo = crear_archivo_local(nombre_archivo);

        pthread_mutex_lock(&mutex_archivos_globales);
        list_add(listaArchivosGlobales, archivo);
        list_add(tablaArchivosAbiertos, archivo);
        pthread_mutex_unlock(&mutex_archivos_globales);
    }

    t_archivo_abierto* archivo_abierto = crear_archivo_abierto(archivo);

    if(strcmp(modo_apertura, "R") == 0){ // el proceso quiere leer el archivo
        if(archivo->lock_escritura.__align == 0){ // no hay un lock de escritura activo
            if(archivo->lock_lectura.__align == 0){ // no hay nadie leyendo
                // En este caso, se debe crear dicho lock como único participante.
                sem_wait(&archivo->sem_archivo);
                queue_push(archivo->participantes_del_lock, pcb);
                sem_post(&archivo->sem_archivo);

                sem_post(&archivo->lock_lectura);
                log_info(logger_kernel, "PID: %d - Abrir Archivo: %s", pcb->cde->pid, archivo->nombre);

                list_add(pcb->archivos_abiertos, archivo_abierto); // como lo pudo abrir se lo agrega a los archivos_abiertos_del_pcb
                
                if(encontrar_archivo(archivo->nombre, pcb->archivos_solicitados) != NULL) // significa que el archivo esta dentro de los archivos_solicitados por el pcb
                    list_remove_element(pcb->archivos_solicitados, archivo); // lo pudo abrir, asi que lo sca

                if(strcmp(config_kernel.algoritmo, "RR") == 0 && !pcb_en_ejecucion->fin_q)
                    enviar_cde_a_cpu();
                else if(strcmp(config_kernel.algoritmo, "RR") == 0 && pcb_en_ejecucion->fin_q)
                    enviar_de_exec_a_ready();
                else
                    enviar_cde_a_cpu();
            }
            else{ // hay alguien leyendo, agrego al proceso como participante
                // De ser así se debe agregar al proceso como "participante" 
                // de dicho lock y el mismo finalizará cuando se ejecuten todos los F_CLOSE de los participantes.
                sem_wait(&archivo->sem_archivo);
                queue_push(archivo->participantes_del_lock, pcb);
                sem_post(&archivo->sem_archivo);

                sem_post(&archivo->lock_lectura);
                log_info(logger_kernel, "PID: %d - Abrir Archivo: %s", pcb->cde->pid, archivo->nombre);
                
                list_add(pcb->archivos_abiertos, archivo_abierto); // como lo pudo abrir se lo agrega a los archivos_abiertos_del_pcb

                if(encontrar_archivo(archivo->nombre, pcb->archivos_solicitados) != NULL) // significa que el archivo esta dentro de los archivos_solicitados por el pcb
                    list_remove_element(pcb->archivos_solicitados, archivo); // no lo pudo abrir, asi que lo solicita
                
                // si el pcb_en_ejecucion es el mismo que el que ejecuta el f_open, entonces que haga el post de enviar_cde
                if(pcb_en_ejecucion->cde->pid == pcb->cde->pid){
                    if(strcmp(config_kernel.algoritmo, "RR") == 0 && !pcb_en_ejecucion->fin_q)
                        enviar_cde_a_cpu();
                    else if(strcmp(config_kernel.algoritmo, "RR") == 0 && pcb_en_ejecucion->fin_q)
                        enviar_de_exec_a_ready();
                    else
                        enviar_cde_a_cpu();
                }
                else // sino que mande el proceso a ready
                    enviar_pcb_de_block_a_ready(pcb);
            }
        }
        else{ // bloquear proceso y esperar a que finalice la operacion
            sem_wait(&archivo->sem_archivo);
            queue_push(archivo->procesos_bloqueados_para_lectura, pcb->cde->pid);
            queue_push(archivo->procesos_bloqueados, pcb);
            sem_post(&archivo->sem_archivo);

            list_add(pcb->archivos_solicitados, archivo); // no lo pudo abrir, asi que lo solicita
            log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb->cde->pid, archivo->nombre);
            enviar_de_exec_a_block();
        }
    }
    else if(strcmp(modo_apertura, "W") == 0){ // el proceso quiere escribir el archivo
        //puede escribir solo si otro proceso no esta escribiendo
        //o si no hay ningun proceso que este leyendo, sino tiene que esperar a que terminen todos los que estan leyendo
        if(archivo->lock_escritura.__align == 0 && archivo->lock_lectura.__align == 0){
            sem_post(&archivo->lock_escritura);
            log_info(logger_kernel, "PID: %d - Abrir Archivo: %s", pcb->cde->pid, archivo->nombre);
            
            list_add(pcb->archivos_abiertos, archivo_abierto); // como lo pudo abrir se lo agrega a los archivos_abiertos_del_pcb

            if(encontrar_archivo(archivo->nombre, pcb->archivos_solicitados) != NULL) // significa que el archivo esta dentro de los archivos_solicitados por el pcb
                list_remove_element(pcb->archivos_solicitados, archivo); // no lo pudo abrir, asi que lo solicita

            if(strcmp(config_kernel.algoritmo, "RR") == 0 && !pcb_en_ejecucion->fin_q)
                enviar_cde_a_cpu();
            else if(strcmp(config_kernel.algoritmo, "RR") == 0 && pcb_en_ejecucion->fin_q)
                enviar_de_exec_a_ready();
            else
                enviar_cde_a_cpu();
        }
        else{ // sino se bloquea
            sem_wait(&archivo->sem_archivo);
            queue_push(archivo->procesos_bloqueados_para_escritura, pcb->cde->pid);
            queue_push(archivo->procesos_bloqueados, pcb);
            sem_post(&archivo->sem_archivo);

            list_add(pcb->archivos_solicitados, archivo); // no lo pudo abrir, asi que lo solicita
            log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb->cde->pid, archivo->nombre);
            enviar_de_exec_a_block();
        }
    }
    else
        log_error(logger_kernel, "No se reconoce el modo de apertura");
}

void ejecutar_f_close(char* nombre_archivo, t_pcb* pcb_f_close){
    t_archivo_abierto* archivo_abierto = encontrar_archivo_abierto(nombre_archivo, pcb_f_close->archivos_abiertos);
    t_archivo* archivo = archivo_abierto->archivo;

    if(archivo->lock_escritura.__align == 1){ // significa que habia sido abierto para escritura
        sem_wait(&archivo->lock_escritura);

        pthread_mutex_lock(&mutex_archivos_abiertos);
        list_remove_element(tablaArchivosAbiertos, archivo);
        pthread_mutex_unlock(&mutex_archivos_abiertos);

        log_info(logger_kernel, "PID: %d - Cerrar Archivo: %s", pcb_f_close->cde->pid, archivo->nombre);
        list_remove_element(pcb_f_close->archivos_abiertos, archivo_abierto); // como lo pudo cerrar se lo saca de archivos_abiertos_del_pcb
        free(archivo_abierto);
        // no liberamos el t_archivo de adentro porque sigue existiendo en el programa, y sino perdemos la referencia
    }
    else{ // significa que habia sido abierto para lectura
        sem_wait(&archivo->lock_lectura);
        list_remove_element(pcb_f_close->archivos_abiertos, archivo_abierto); // como lo pudo cerrar se lo saca de archivos_abiertos_del_pcb
        list_remove_element(archivo->participantes_del_lock->elements, pcb_f_close);
        free(archivo_abierto);
        // no liberamos el t_archivo de adentro porque sigue existiendo en el programa, y sino perdemos la referencia
        if(archivo->lock_lectura.__align == 0){
            pthread_mutex_lock(&mutex_archivos_abiertos);
            list_remove_element(tablaArchivosAbiertos, archivo);
            pthread_mutex_unlock(&mutex_archivos_abiertos);

            log_info(logger_kernel, "PID: %d - Cerrar Archivo: %s", pcb_f_close->cde->pid, archivo->nombre);
        }
    }

    sem_wait(&archivo->sem_archivo);
    if(queue_size(archivo->procesos_bloqueados) > 0 && archivo->lock_lectura.__align == 0){
        // entra si hay pcbs bloqueados esperando a acceder al archivo y no hay nadie leyendo
        t_pcb* pcb = queue_pop(archivo->procesos_bloqueados);
        sem_post(&archivo->sem_archivo); // aca posteo el semaforo que se waiteo antes del if
        t_modo_apertura apertura = motivo_bloqueo(archivo, pcb);

        if(apertura == ESCRITURA){
            sem_wait(&archivo->sem_archivo);
            list_remove_element(archivo->procesos_bloqueados_para_escritura->elements, pcb->cde->pid);
            sem_post(&archivo->sem_archivo);
        
            ejecutar_f_open(nombre_archivo, "W", pcb);

            enviar_pcb_de_block_a_ready(pcb);
        }
        else if(apertura == LECTURA){
            sem_wait(&archivo->sem_archivo);
            list_remove_element(archivo->procesos_bloqueados_para_lectura->elements, pcb->cde->pid);
            sem_post(&archivo->sem_archivo);

            ejecutar_f_open(nombre_archivo, "R", pcb);
            enviar_pcb_de_block_a_ready(pcb);

            uint32_t flag_lectura = 0;

            while(flag_lectura != 1 && queue_size(archivo->procesos_bloqueados) > 0){
                t_pcb* pcb_posible_participante = list_get(archivo->procesos_bloqueados->elements, 0);
                if(motivo_bloqueo(archivo, pcb_posible_participante) == LECTURA){ // significa que tambien esta para lectura el que le seguia en la cola de bloqueados
                    sem_wait(&archivo->sem_archivo);
                    list_remove_element(archivo->procesos_bloqueados->elements, pcb_posible_participante);
                    list_remove_element(archivo->procesos_bloqueados_para_lectura->elements, pcb_posible_participante->cde->pid);
                    sem_post(&archivo->sem_archivo);
                
                    ejecutar_f_open(nombre_archivo, "R", pcb_posible_participante);
                }
                else{
                    flag_lectura = 1;
                }
            }
        }
    }
    else{
        sem_post(&archivo->sem_archivo);
        if(!tengo_que_liberar_archivos){ // variable que controla si el pcb_en_ejecucion tiene que cerrar los archivos forzadamente
            if(strcmp(config_kernel.algoritmo, "RR") == 0 && !pcb_en_ejecucion->fin_q)
                enviar_cde_a_cpu();
            else if(strcmp(config_kernel.algoritmo, "RR") == 0 && pcb_en_ejecucion->fin_q)
                enviar_de_exec_a_ready();
            else
                enviar_cde_a_cpu();
        }
    }
}

//void ejecutar_f_truncate(char* nombre_archivo, uint32_t nuevo_tamanio){
void ejecutar_f_truncate(void* args){
    t_instruccion* instruccion = (t_instruccion*) args;
    char* nombre_archivo = instruccion->par1;
    uint32_t nuevo_tamanio = atoi(instruccion->par2);
    //destruir_instruccion(instruccion);
    t_archivo* archivo = encontrar_archivo(nombre_archivo, tablaArchivosAbiertos);
    enviar_codigo(socket_file_system, CAMBIAR_TAMANIO_SOLICITUD);
    
    t_buffer* buffer = crear_buffer_nuestro();
    buffer_write_string(buffer, nombre_archivo);
    buffer_write_uint32(buffer, nuevo_tamanio);
    enviar_buffer(buffer, socket_file_system);
    destruir_buffer_nuestro(buffer);

    t_pcb* pcb = pcb_en_ejecucion;
    
    // se bloquea el proceso hasta que se realice el cambio de tamanio del archivo solicitado
    enviar_de_exec_a_block();

    mensajeKernelFile codigo_recibido = recibir_codigo(socket_file_system);

    if(codigo_recibido == CAMBIAR_TAMANIO_OK){
        archivo->tamanio = nuevo_tamanio;
        log_info(logger_kernel, "PID: %d - Archivo: %s - Tamaño: %d", pcb->cde->pid, archivo->nombre, archivo->tamanio);
        enviar_pcb_de_block_a_ready(pcb);
    }
    else
        log_warning(logger_kernel, "No se realizo la operacion F_TRUNCATE correctamente");
}

void ejecutar_f_seek(char* nombre_archivo, uint32_t nueva_posicion){
    // el archivo ya fue abierto por el pcb_en_ejecucion
    t_archivo_abierto* archivo_abierto = encontrar_archivo_abierto(nombre_archivo, pcb_en_ejecucion->archivos_abiertos);
    archivo_abierto->puntero_archivo = nueva_posicion;
    log_info(logger_kernel, "PID: %d - Actualizar puntero Archivo: %s - Puntero %d", pcb_en_ejecucion->cde->pid, archivo_abierto->archivo->nombre, archivo_abierto->puntero_archivo);
}

void ejecutar_f_write(char* nombre_archivo, uint32_t direccion_fisica, uint32_t numero_pagina_para_write){
    // hay que buscar el archivo dentro de la lista de pcb_en_ejecucion porque ya le fue otorgado el archivo
    t_archivo_abierto* archivo_abierto = encontrar_archivo_abierto(nombre_archivo, pcb_en_ejecucion->archivos_abiertos);
    t_archivo* archivo = archivo_abierto->archivo;

    if(archivo->lock_lectura.__align > 0){ // significa que el proceso lo abrio para lectura, porque para escritura no se pudo abrir
        enviar_de_exec_a_finalizado("INVALID_WRITE");
        return;
    }
    
    enviar_codigo(socket_file_system, REALIZAR_ESCRITURA_SOLICITUD);
    t_buffer* buffer = crear_buffer_nuestro();
    buffer_write_string(buffer, archivo->nombre);
    buffer_write_uint32(buffer, direccion_fisica);
    buffer_write_uint32(buffer, archivo_abierto->puntero_archivo);
    buffer_write_uint32(buffer, numero_pagina_para_write);
    buffer_write_uint32(buffer, pcb_en_ejecucion->cde->pid);
    enviar_buffer(buffer, socket_file_system);
    destruir_buffer_nuestro(buffer);

    t_pcb* pcb = pcb_en_ejecucion;
    
    enviar_de_exec_a_block();

    mensajeKernelFile codigo_recibido = recibir_codigo(socket_file_system);

    if(codigo_recibido == REALIZAR_ESCRITURA_OK){
        log_info(logger_kernel, "PID: %d - Escribir Archivo: %s - Puntero: %d - Dirección Memoria: %d - Tamaño %d", pcb->cde->pid, archivo->nombre, archivo_abierto->puntero_archivo, direccion_fisica, archivo->tamanio);
        enviar_pcb_de_block_a_ready(pcb);
    }
    else
        log_warning(logger_kernel, "La escritura del archivo %s no se realizo con exito", archivo->nombre);
}

void ejecutar_f_read(char* nombre_archivo, uint32_t direccion_fisica, uint32_t numero_pagina_para_read){
    // hay que buscar el archivo dentro de la lista de pcb_en_ejecucion porque ya le fue otorgado el archivo
    t_archivo_abierto* archivo_abierto = encontrar_archivo_abierto(nombre_archivo, pcb_en_ejecucion->archivos_abiertos);
    t_archivo* archivo = archivo_abierto->archivo;

    enviar_codigo(socket_file_system, REALIZAR_LECTURA_SOLICITUD);
    t_buffer* buffer = crear_buffer_nuestro();
    buffer_write_string(buffer, archivo->nombre);
    buffer_write_uint32(buffer, direccion_fisica);
    buffer_write_uint32(buffer, archivo_abierto->puntero_archivo);
    buffer_write_uint32(buffer, numero_pagina_para_read);
    buffer_write_uint32(buffer, pcb_en_ejecucion->cde->pid);
    enviar_buffer(buffer, socket_file_system);
    destruir_buffer_nuestro(buffer);

    t_pcb* pcb = pcb_en_ejecucion;
    
    enviar_de_exec_a_block();

    mensajeKernelFile codigo_recibido = recibir_codigo(socket_file_system);

    if(codigo_recibido == REALIZAR_LECTURA_OK){
        log_info(logger_kernel, "PID: %d - Leer Archivo: %s - Puntero: %d - Dirección Memoria: %d - Tamaño %d", pcb->cde->pid, archivo->nombre, archivo_abierto->puntero_archivo, direccion_fisica, archivo->tamanio);
        enviar_pcb_de_block_a_ready(pcb);
    }
    else
        log_warning(logger_kernel, "La lectura del archivo %s no se realizo con exito", archivo->nombre);
}

// FIN FUNCIONES DE FILE-SYSTEM