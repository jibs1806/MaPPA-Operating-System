#include <transiciones.h>

// TANSICIONES LARGO PLAZO
void enviar_de_new_a_ready(){  
    while(1){
        sem_wait(&procesos_en_new);
        
        sem_wait(&grado_de_multiprogramacion); // si esto esta arriba del if planif detendia => anda cambiar grado de multiprog
        // habria que chequear que ande planificacion detenida para todooo x este cambio!!
        if(planificacion_detenida == 1){ 
            sem_wait(&pausar_new_a_ready);
        }

        t_pcb* pcb_a_ready = retirar_pcb_de(procesosNew, &mutex_new);
        
        agregar_pcb_a(procesosReady, pcb_a_ready, &mutex_ready);
        pcb_a_ready->estado = READY;

        // Pedir a memoria incializar estructuras

        log_info(logger_kernel, "PID: %d - Estado anterior: %s - Estado actual: %s", pcb_a_ready->cde->pid, obtener_nombre_estado(NEW), obtener_nombre_estado(READY)); //OBLIGATORIO
        
        if(strcmp(config_kernel.algoritmo, "PRIORIDADES") == 0 && pcb_en_ejecucion != NULL){
            t_pcb* posible_a_ejecutar = pcb_con_mayor_prioridad_en_ready();

            if(pcb_en_ejecucion->prioridad > posible_a_ejecutar->prioridad){
                enviar_codigo(socket_cpu_interrupt, DESALOJO);

                t_buffer* buffer = crear_buffer_nuestro();
                buffer_write_uint32(buffer, posible_a_ejecutar->cde->pid); // lo enviamos porque interrupt recibe un buffer, pero no hacemos nada con esto
                enviar_buffer(buffer, socket_cpu_interrupt);
                destruir_buffer_nuestro(buffer);
            }
        }
        
        sem_post(&procesos_en_ready);
	}
}

char* obtener_elementos_cargados_en(t_queue* cola){
    char* aux = string_new();
    string_append(&aux,"[");
    int pid_aux;
    char* aux_2;
    for(int i = 0 ; i < list_size(cola->elements); i++){
        t_pcb* pcb = list_get(cola->elements,i);
        pid_aux = pcb->cde->pid;
        aux_2 = string_itoa(pid_aux);
        string_append(&aux, aux_2);
        free(aux_2);
        if(i != list_size(cola->elements)-1)
            string_append(&aux,", ");
    }
    string_append(&aux,"]");
    return aux;
}

void enviar_de_exec_a_finalizado(char* razon){
    sem_wait(&cont_exec);
    
    if(planificacion_detenida == 1){
        sem_wait(&pausar_exec_a_finalizado);
    }

	agregar_pcb_a(procesosFinalizados, pcb_en_ejecucion, &mutex_finalizados);
    pcb_en_ejecucion->estado = EXIT;

	log_info(logger_kernel, "PID: %d - Estado anterior: %s - Estado actual: %s", pcb_en_ejecucion->cde->pid, obtener_nombre_estado(EXEC), obtener_nombre_estado(FINISH)); //OBLIGATORIO
	log_info(logger_kernel, "Finaliza el proceso %d - Motivo: %s", pcb_en_ejecucion->cde->pid, razon); // OBLIGATORIO
	
    pthread_mutex_lock(&mutex_pcb_en_ejecucion);
    pcb_en_ejecucion = NULL;
    pthread_mutex_unlock(&mutex_pcb_en_ejecucion);

    sem_post(&cpu_libre); // se libera el procesador
	sem_post(&procesos_en_exit); // se agrega uno a procesosExit
    sem_post(&grado_de_multiprogramacion); // Se libera 1 grado de multiprog
}
// FIN TANSICIONES LARGO PLAZO

// TRANSICIONES CORTO PLAZO

t_pcb* elegir_segun_fifo(){
    return retirar_pcb_de(procesosReady, &mutex_ready);
}

t_pcb* elegir_segun_rr(){
    return retirar_pcb_de(procesosReady, &mutex_ready);
}

t_pcb* elegir_segun_prioridades(){
    t_pcb* pcb_elegido = pcb_con_mayor_prioridad_en_ready();

    pthread_mutex_lock(&mutex_ready);
    list_remove(procesosReady->elements, indice);
    pthread_mutex_unlock(&mutex_ready);

    return pcb_elegido;
}

t_pcb* pcb_con_mayor_prioridad_en_ready(){
    pthread_mutex_lock(&mutex_ready);
	t_pcb* pcb_elegido = list_get(procesosReady->elements, 0);
	pthread_mutex_unlock(&mutex_ready);    
    t_pcb* pcb_aux;
    indice = 0;
    
    for(int i=1; i < list_size(procesosReady->elements); i++){
        pthread_mutex_lock(&mutex_ready);
        pcb_aux = list_get(procesosReady->elements, i);
        pthread_mutex_unlock(&mutex_ready);

        if(pcb_elegido->prioridad > pcb_aux->prioridad){
            pcb_elegido = pcb_aux;
            pthread_mutex_lock(&mutex_indice);
            indice = i; 
            pthread_mutex_unlock(&mutex_indice);    
        }
    }

    return pcb_elegido;
}

t_pcb* retirar_pcb_de_ready_segun_algoritmo(){
    if(strcmp(config_kernel.algoritmo, "FIFO") == 0){
        t_pcb* pcb = elegir_segun_fifo();
        return pcb;
    }
    else if(strcmp(config_kernel.algoritmo, "RR") == 0){
        t_pcb* pcb = elegir_segun_rr();
        return pcb;
    }
    else if(strcmp(config_kernel.algoritmo, "PRIORIDADES") == 0){
        return elegir_segun_prioridades();
    }
    else{
        log_info(logger_kernel, "No se reconoce el algoritmo definido en el config_kernel");
    }

    exit(1); 
}

void enviar_de_ready_a_exec(){
    while(1){
        sem_wait(&cpu_libre);

		sem_wait(&procesos_en_ready);

        // Log obligatorio
        char* lista_pcbs_en_ready = obtener_elementos_cargados_en(procesosReady);
        log_info(logger_kernel, "Cola Ready %s: %s", config_kernel.algoritmo, lista_pcbs_en_ready);
        free(lista_pcbs_en_ready);

        if(planificacion_detenida == 1){
            sem_wait(&pausar_ready_a_exec);
        }

		pthread_mutex_lock(&mutex_exec);
		pcb_en_ejecucion = retirar_pcb_de_ready_segun_algoritmo();
		pthread_mutex_unlock(&mutex_exec);

		log_info(logger_kernel, "PID: %d - Estado anterior: %s - Estado actual: %s", pcb_en_ejecucion->cde->pid, obtener_nombre_estado(READY), obtener_nombre_estado(EXEC)); //OBLIGATORIO
        pcb_en_ejecucion->estado = EXEC;

		// Primer post de ese semaforo ya que se envia por primera vez
		sem_post(&cont_exec); // dsps se hace el wait cuando quiero sacar de exec (x block, etc)
        
        if(strcmp(config_kernel.algoritmo, "RR") == 0){
            pcb_en_ejecucion->fin_q = false;
            sem_wait(&sem_reloj_destruido);
            sem_post(&sem_iniciar_quantum);
        }
        enviar_cde_a_cpu(); //avisarle al kernel que empiece a correr el proceso en el cpu y le mande las cosas necesarias
    }
}

void enviar_de_exec_a_ready(){
    sem_wait(&cont_exec);
    if(planificacion_detenida == 1){
        sem_wait(&pausar_exec_a_ready);
    }
    agregar_pcb_a(procesosReady, pcb_en_ejecucion, &mutex_ready);
    pcb_en_ejecucion->estado = READY;
    
    log_info(logger_kernel, "PID: %d - Estado anterior: %s - Estado actual: %s", pcb_en_ejecucion->cde->pid, obtener_nombre_estado(EXEC), obtener_nombre_estado(READY));

    pthread_mutex_lock(&mutex_pcb_en_ejecucion);
    pcb_en_ejecucion = NULL;
    pthread_mutex_unlock(&mutex_pcb_en_ejecucion);
    
    sem_post(&cpu_libre); // se libera el procesador
    sem_post(&procesos_en_ready);
}

void enviar_de_exec_a_block(){
    sem_wait(&cont_exec);
    if(planificacion_detenida == 1){
        sem_wait(&pausar_exec_a_blocked);
    }
    agregar_pcb_a(procesosBloqueados, pcb_en_ejecucion, &mutex_block);
    pcb_en_ejecucion->estado = BLOCKED;

    log_info(logger_kernel, "PID: %d - Estado anterior: %s - Estado actual: %s", pcb_en_ejecucion->cde->pid, obtener_nombre_estado(EXEC), obtener_nombre_estado(BLOCKED));
    
    pthread_mutex_lock(&mutex_pcb_en_ejecucion);
    pcb_en_ejecucion = NULL;
    pthread_mutex_unlock(&mutex_pcb_en_ejecucion);
    

    sem_post(&cpu_libre); // se libera el procesador
    sem_post(&procesos_en_blocked);
    detectarDeadlock();
}

// esta funcion se usa para mandar un pcb a ready cuando se liberan las instancias del recurso/s que necesitaba
// yendo a ready porque esta listo para ser elegido para ejecutar
void enviar_pcb_de_block_a_ready(t_pcb* pcb){
    
    sem_wait(&procesos_en_blocked);
    
    if(planificacion_detenida == 1){
        sem_wait(&pausar_blocked_a_ready);
    }

    int posicion_pcb = esta_proceso_en_cola_bloqueados(pcb);

    t_pcb* pcb_a_ready = list_get(procesosBloqueados->elements,posicion_pcb);
    
    pthread_mutex_lock(&mutex_block);
    list_remove_element(procesosBloqueados->elements, pcb);
    pthread_mutex_unlock(&mutex_block);

    agregar_pcb_a(procesosReady, pcb_a_ready, &mutex_ready);
    pcb_a_ready->estado = READY;

    log_info(logger_kernel, "PID: %d - Estado anterior: %s - Estado actual: %s", pcb_a_ready->cde->pid, obtener_nombre_estado(BLOCKED), obtener_nombre_estado(READY));

    sem_post(&procesos_en_ready);

    if(strcmp(config_kernel.algoritmo, "PRIORIDADES") == 0 && pcb_en_ejecucion != NULL){
            t_pcb* posible_a_ejecutar = pcb_con_mayor_prioridad_en_ready();

            if(pcb_en_ejecucion->prioridad > posible_a_ejecutar->prioridad){
                enviar_codigo(socket_cpu_interrupt, DESALOJO);

                t_buffer* buffer = crear_buffer_nuestro();
                buffer_write_uint32(buffer, posible_a_ejecutar->cde->pid); // lo enviamos porque interrupt recibe un buffer, pero no hacemos nada con esto
                enviar_buffer(buffer, socket_cpu_interrupt);
                destruir_buffer_nuestro(buffer);
            }
        }
}

int esta_proceso_en_cola_bloqueados(t_pcb* pcb){
    int posicion_pcb = -1;
    for(int i=0; i < list_size(procesosBloqueados->elements); i++){
        t_pcb* pcb_get = list_get(procesosBloqueados->elements, i);
        int pid_pcb = pcb_get->cde->pid;
        if(pcb->cde->pid == pid_pcb){
            posicion_pcb = i;
            break;
        }
    }
    return posicion_pcb;
}