#include <deadlock.h>

void detectarDeadlock(){ // se llama cuando: entra 1 a block, o cuando finaliza un proceso por consola!!!!!
    log_info(logger_kernel, "---------------ANÁLISIS DE DETECCIÓN DE DEADLOCK---------------");

    // Deteccion de Deadlock para los recursos
    int cant_de_rec = list_size(config_kernel.recursos);
    t_list* recursos_con_procesos_en_espera;
    t_list* procesos_con_deadlock_por_recursos;
    char* lista_recursos_asignados_pcb;
    char* lista_recursos_solicitados_pcb;
    
    // Ver que recursos tiene procesos en espera
    recursos_con_procesos_en_espera = obtener_recursos_con_proc_bloqueados(cant_de_rec); // recursos posible deadlock
    
    // Ver que procesos tienen asignados estos recursos
    procesos_con_deadlock_por_recursos = obtener_procesos_con_deadlock_de_recursos(recursos_con_procesos_en_espera); // procesos con deadlock

    if(list_size(procesos_con_deadlock_por_recursos) >= 2){
        for(int i = 0; i < list_size(procesos_con_deadlock_por_recursos); i++){
            t_pcb* pcb_en_deadlock_por_rec = list_get(procesos_con_deadlock_por_recursos, i);
            lista_recursos_asignados_pcb = obtener_lista_de_recursos(pcb_en_deadlock_por_rec->recursos_asignados);
            lista_recursos_solicitados_pcb = obtener_lista_de_recursos(pcb_en_deadlock_por_rec->recursos_solicitados);
            log_info(logger_kernel, "Deadlock detectado: PID %d - Recursos en posesión: %s- Recurso requerido: %s", pcb_en_deadlock_por_rec->cde->pid, lista_recursos_asignados_pcb, lista_recursos_solicitados_pcb);
            free(lista_recursos_asignados_pcb);
            free(lista_recursos_solicitados_pcb);
        }
    }
    
    destruir_lista(recursos_con_procesos_en_espera);
    destruir_lista(procesos_con_deadlock_por_recursos);

    // Deteccion de Deadlock para los archivos
    int cant_de_archivos = list_size(tablaArchivosAbiertos); // creo que se tiene que usar esta lista
    t_list* archivos_con_procesos_en_espera;
    t_list* procesos_con_deadlock_por_archivos;
    char* lista_archivos_asignados_pcb;
    char* lista_archivos_solicitados_pcb;

    // Ver que recursos tiene procesos en espera
    archivos_con_procesos_en_espera = obtener_archivos_con_proc_bloqueados(cant_de_archivos); // archivos posible deadlock

    // Ver que procesos tienen asignados estos recursos
    procesos_con_deadlock_por_archivos = obtener_procesos_con_deadlock_de_archivos(archivos_con_procesos_en_espera); // procesos con deadlock

    if(list_size(procesos_con_deadlock_por_archivos) >= 2){
        for(int i = 0; i < list_size(procesos_con_deadlock_por_archivos); i++){
            t_pcb* pcb_en_deadlock_por_archivo = list_get(procesos_con_deadlock_por_archivos, i);
            lista_archivos_asignados_pcb = obtener_lista_de_archivos_abiertos(pcb_en_deadlock_por_archivo->archivos_abiertos);
            lista_archivos_solicitados_pcb = obtener_lista_de_archivos(pcb_en_deadlock_por_archivo->archivos_solicitados);
            log_info(logger_kernel, "Deadlock detectado: PID %d - Archivos en posesión: %s- Archivo requerido: %s", pcb_en_deadlock_por_archivo->cde->pid, lista_archivos_asignados_pcb, lista_archivos_solicitados_pcb);
            free(lista_archivos_asignados_pcb);
            free(lista_archivos_solicitados_pcb);
        }
    }
    
    destruir_lista(archivos_con_procesos_en_espera);
    destruir_lista(procesos_con_deadlock_por_archivos);
}

t_list* obtener_recursos_con_proc_bloqueados(int cant_de_rec){
    t_list* recursos_con_procesos_en_espera = list_create();
    for(int i = 0; i < cant_de_rec; i++){
        t_recurso* recurso = list_get(config_kernel.recursos, i);
        if(list_size(recurso->procesos_bloqueados) > 0){ // guardar el recurso que cumpla esto!
            list_add(recursos_con_procesos_en_espera, recurso);
        }
        // si no entra en el if, el recurso no tiene procesos_bloqueado => no puede generar deadlock
    }
    return recursos_con_procesos_en_espera;
}

t_list* obtener_procesos_con_deadlock_de_recursos(t_list* recursos_con_procesos_en_espera){
    t_list* procesos_con_deadlock = list_create();
    for(int j = 0; j < queue_size(procesosBloqueados); j++){ // recorro los procesos bloqueados
        t_pcb* pcb = list_get(procesosBloqueados->elements, j);
        // chequeamos si el pcb esta dentro de alguna de la lista de procesos_bloqueados de alguno de los recursos
        for(int k = 0; k < list_size(pcb->recursos_solicitados); k++){
            t_recurso* rec_proceso = list_get(pcb->recursos_solicitados, k);
            for(int h = 0; h < list_size(recursos_con_procesos_en_espera); h++){
                t_recurso* rec_a_comparar = list_get(recursos_con_procesos_en_espera, h);
                if(rec_a_comparar->nombre == rec_proceso->nombre && tiene_asignado_algun_recurso_en_deadlock(pcb, recursos_con_procesos_en_espera)){
                    // si son iguales entonces ese proceso es posible deadlock
                    // ademas, chequeamos de que el proceso tenga al menos un recurso asignado, porque sino esta en starvation
                    // un proceso no puede producir deadlock si no tiene ningun recurso asignado
                    // tampoco puede producir deadlock si el proceso no tiene asignado una instancia de recurso que produce deadlock
                    list_add(procesos_con_deadlock, pcb);
                }
            }
        }
    }
    return procesos_con_deadlock;
}

char* obtener_lista_de_recursos(t_list* lista_recursos){
    char* recursos_listados = string_new();
    string_append(&recursos_listados,"[");
    for(int i = 0; i < list_size(lista_recursos); i++){
        t_recurso* recurso = list_get(lista_recursos, i);
        string_append(&recursos_listados, recurso->nombre);
        if(i != list_size(lista_recursos)-1)
            string_append(&recursos_listados,", ");
    }
    string_append(&recursos_listados,"]");
    return recursos_listados;
}

bool tiene_asignado_algun_recurso_en_deadlock(t_pcb* pcb, t_list* recursos_con_procesos_en_espera){
    for(int i = 0; i < list_size(pcb->recursos_asignados); i++){
        t_recurso* recurso_asignado_al_pcb = list_get(pcb->recursos_asignados, i);
        for(int j = 0; j < list_size(recursos_con_procesos_en_espera); j++){
            t_recurso* recurso_proc_en_espera = list_get(recursos_con_procesos_en_espera, j);
            if(recurso_proc_en_espera->nombre == recurso_asignado_al_pcb->nombre)
                return true;
        }
    }
    return false;
}

void destruir_lista(t_list* lista){
    for(int i = 0; i < list_size(lista); i++){
        void* elemento = list_get(lista, i);
        list_remove_element(lista, elemento);
    }
    list_destroy(lista);
}

// FIN FUNCIONES PARA RECURSOS

// FUNCIONES PARA ARCHIVOS
t_list* obtener_archivos_con_proc_bloqueados(int cant_de_archivo){
    t_list* archivos_con_procesos_en_espera = list_create();
    for(int i = 0; i < cant_de_archivo; i++){
        t_archivo* archivo = list_get(tablaArchivosAbiertos, i);
        if(queue_size(archivo->procesos_bloqueados) > 0){ // guardar el archivo que cumpla esto!
            list_add(archivos_con_procesos_en_espera, archivo);
        }
        // si no entra en el if, el archivo no tiene procesos_bloqueado => no puede generar deadlock
    }
    return archivos_con_procesos_en_espera;
}

t_list* obtener_procesos_con_deadlock_de_archivos(t_list* archivos_con_procesos_en_espera){
    t_list* procesos_con_deadlock_por_archivos = list_create();
    for(int j = 0; j < queue_size(procesosBloqueados); j++){ // recorro los procesos bloqueados
        t_pcb* pcb = list_get(procesosBloqueados->elements, j);
        // chequeamos si el pcb esta dentro de alguna de la lista de procesos_bloqueados de alguno de los archivos
        for(int k = 0; k < list_size(pcb->archivos_abiertos); k++){
            t_archivo_abierto* archivo_proceso_abierto = list_get(pcb->archivos_abiertos, k);
            t_archivo* archivo_proceso = archivo_proceso_abierto->archivo;
            for(int h = 0; h < list_size(archivos_con_procesos_en_espera); h++){
                t_archivo* archivo_a_comparar = list_get(archivos_con_procesos_en_espera, h);
                if(archivo_a_comparar->nombre == archivo_proceso->nombre && tiene_asignado_algun_archivo_en_deadlock(pcb, archivos_con_procesos_en_espera)){
                    // si son iguales entonces ese proceso es posible deadlock
                    // ademas, chequeamos de que el proceso tenga al menos un archivo asignado, porque sino esta en starvation
                    // un proceso no puede producir deadlock si no tiene ningun archivo asignado
                    // tampoco puede producir deadlock si el proceso no tiene asignado una instancia de archivo que produce deadlock
                    list_add(procesos_con_deadlock_por_archivos, pcb);
                }
            }
        }
    }
    return procesos_con_deadlock_por_archivos;
}

char* obtener_lista_de_archivos_abiertos(t_list* lista_archivos_abiertos){
    char* archivos_listados = string_new();
    string_append(&archivos_listados,"[");
    for(int i = 0; i < list_size(lista_archivos_abiertos); i++){
        t_archivo_abierto* archivo_abierto = list_get(lista_archivos_abiertos, i);
        t_archivo* archivo = archivo_abierto->archivo;
        string_append(&archivos_listados, archivo->nombre);
        if(i != list_size(lista_archivos_abiertos)-1)
            string_append(&archivos_listados,", ");
    }
    string_append(&archivos_listados,"]");
    return archivos_listados;
}

char* obtener_lista_de_archivos(t_list* lista_archivos){
    char* archivos_listados = string_new();
    string_append(&archivos_listados,"[");
    for(int i = 0; i < list_size(lista_archivos); i++){
        t_archivo* archivo = list_get(lista_archivos, i);
        string_append(&archivos_listados, archivo->nombre);
        if(i != list_size(lista_archivos)-1)
            string_append(&archivos_listados,", ");
    }
    string_append(&archivos_listados,"]");
    return archivos_listados;
}

bool tiene_asignado_algun_archivo_en_deadlock(t_pcb* pcb, t_list* archivos_con_procesos_en_espera){
    for(int i = 0; i < list_size(pcb->archivos_abiertos); i++){
        t_archivo_abierto* archivo_asignado_al_pcb_abierto = list_get(pcb->archivos_abiertos, i);
        t_archivo* archivo_asignado_al_pcb = archivo_asignado_al_pcb_abierto->archivo;
        for(int j = 0; j < list_size(archivos_con_procesos_en_espera); j++){
            t_archivo* archivo_proc_en_espera = list_get(archivos_con_procesos_en_espera, j);
            if(archivo_proc_en_espera->nombre == archivo_asignado_al_pcb->nombre)
                return true;
        }
    }
    return false;
}

// FIN FUNCIONES PARA ARCHIVOS