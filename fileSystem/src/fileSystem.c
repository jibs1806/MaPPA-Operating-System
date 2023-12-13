#include <fileSystem.h>

int main(int argc, char* argv[]) {
    char* config_path = argv[1]; // TO DO
    inicializarModulo("./fileSystem.config");

    sem_wait(&no_end_file_system);

    terminar_conexiones(2, socket_kernel, socket_memoria);
    terminar_programa(logger_file_system, config);
    
    return 0;
}

void inicializarModulo(char* config_path){
    levantarLogger();
    levantarConfig(config_path);

    inicializarVariables();
    inicializarSemaforos();
    inicializarConexiones();

    return;
}

void inicializarVariables(){
    bitmapBloquesSwap = malloc(config_file_system.cantidad_bloques_swap * sizeof(uint8_t));
    
    for(int i = 0; i < config_file_system.cantidad_bloques_swap; i++){
        uint8_t k = 0;
        memcpy(bitmapBloquesSwap + i, &k, sizeof(uint8_t));
    }

    levantar_archivo_bloques();
    levantar_tabla_fat();
}

void inicializarSemaforos(){
    sem_init(&no_end_file_system, 0, 0);
}

void inicializarConexiones(){
   
    int socket_servidor = crear_servidor(config_file_system.puerto_escucha);

    conectarMemoria(socket_servidor); //crear servidor y esperar cliente (memoria)

    conectarKernel(socket_servidor);  //crear servidor y esperar cliente (kernel)
    
}

void conectarMemoria(int socket_servidor){

    log_info(logger_file_system, "Esperando Memoria....");
    socket_memoria = esperar_cliente(socket_servidor); //En la librería faltaba utils/conexiones.h, ya no hace falta agreagarlo porque se encuentra en instruccionescpu.h
    log_info(logger_file_system, "Se conecto Memoria");

    pthread_t hilo_memoria;
	pthread_create(&hilo_memoria, NULL, (void *) atender_memoria, NULL);
	pthread_detach(hilo_memoria);
}

void conectarKernel(int socket_servidor){

    log_info(logger_file_system, "Esperando Kernel....");
    socket_kernel = esperar_cliente(socket_servidor); //En la librería faltaba utils/conexiones.h, ya no hace falta agreagarlo porque se encuentra en instruccionescpu.h
    log_info(logger_file_system, "Se conecto el Kernel");

    pthread_t hilo_kernel;
	pthread_create(&hilo_kernel, NULL, (void *) atender_kernel, NULL);
	pthread_detach(hilo_kernel);
}

void levantarLogger(){
    logger_file_system = iniciar_logger("./file_system.log", "FILE_SYSTEM");
}

void levantarConfig(char* config_path){
    config = iniciar_config(config_path);

    if (config == NULL){
        log_info(logger_file_system, "Error al levantarConfig");
        exit(1);
    }
    config_file_system.ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    config_file_system.puerto_memoria = config_get_int_value(config,"PUERTO_MEMORIA");
    config_file_system.puerto_escucha = config_get_int_value(config,"PUERTO_ESCUCHA");
    config_file_system.path_fat = config_get_string_value(config,"PATH_FAT");
    config_file_system.path_bloques = config_get_string_value(config,"PATH_BLOQUES");
    config_file_system.path_fcb = config_get_string_value(config,"PATH_FCB");
    config_file_system.cantidad_bloques_total= config_get_int_value(config,"CANT_BLOQUES_TOTAL");
    config_file_system.cantidad_bloques_swap= config_get_int_value(config,"CANT_BLOQUES_SWAP");
    config_file_system.tam_bloque= config_get_int_value(config,"TAM_BLOQUE");
    config_file_system.retardo_acceso_bloque= config_get_int_value(config,"RETARDO_ACCESO_BLOQUE");
    config_file_system.retardo_acceso_fat= config_get_int_value(config,"RETARDO_ACCESO_FAT");
}

void atender_memoria(){
    while(1){
        mensajeMemoriaFile codigo = recibir_codigo(socket_memoria);

        switch(codigo){
        case BUSCAR_PAGINA_SOLICITUD:
            devolver_pagina();
            break;
        case ACTUALIZAR_PAGINA_SOLICITUD:
            actualizar_pagina();
            break;
        case LECTURA_MEMORIA_OK:
            recibir_bloque_de_memoria();
            break;
        case ESCRITURA_MEMORIA_OK:
            enviar_confirmacion_a_kernel_de_escritura();
            break;
        case CREAR_PAGINA_SOLICITUD:
            reservar_bloque_swap();
            break;
        case LIBERAR_BLOQUES_SOLICITUD:
            liberar_bloque_swap();
            break;
        default:
            break;
        }
    }
}

void atender_kernel(){
    while(1){
        mensajeKernelFile codigo = recibir_codigo(socket_kernel);

        switch(codigo){
            case ABRIR_ARCHIVO_SOLICITUD:
                abrir_archivo();
                break;
            case CREAR_ARCHIVO_SOLICITUD:
                crear_archivo();
                break;
            case REALIZAR_LECTURA_SOLICITUD: // F_READ
                leer_archivo();
                break;
            case REALIZAR_ESCRITURA_SOLICITUD: // F_WRITE
                escribir_archivo();
                break;
            case CAMBIAR_TAMANIO_SOLICITUD:
                truncar_archivo();
                break;
            default:
                log_warning(logger_file_system, "default");
                break;
        }
    }
}

// UTILS KERNEL

// Kernel nos pasa una direccion -> le pido a memoria el dato de esa direccion 
// -> lo escribo en el archivo pasado por parametro (en el bloques.dat)
void escribir_archivo(){
    t_buffer* buffer = recibir_buffer(socket_kernel);
    uint32_t tam_escritura;
    
    char* nombre_archivo = buffer_read_string(buffer, &tam_escritura);
    uint32_t dirFisica = buffer_read_uint32(buffer);
    uint32_t puntero = buffer_read_uint32(buffer);
    uint32_t nroPagina = buffer_read_uint32(buffer);
    uint32_t pid = buffer_read_uint32(buffer);
    destruir_buffer_nuestro(buffer);
    
    enviar_codigo(socket_memoria, LECTURA_MEMORIA_SOLICITUD);
    
    t_buffer* b = crear_buffer_nuestro();
    buffer_write_uint32(b, dirFisica);
    buffer_write_string(b, nombre_archivo);
    buffer_write_uint32(b, puntero);
    buffer_write_uint32(b, nroPagina);
    buffer_write_uint32(b, pid);
    enviar_buffer(b, socket_memoria);
    destruir_buffer_nuestro(b);

    free(nombre_archivo);
}

void recibir_bloque_de_memoria(){
    uint32_t tam;
    t_buffer* b = recibir_buffer(socket_memoria);
    void* bloqueLeido = buffer_read_pagina(b, config_file_system.tam_bloque);

    uint32_t dirFisica = buffer_read_uint32(b);
    char* nombre_archivo = buffer_read_string(b, &tam);
    uint32_t puntero = buffer_read_uint32(b);

    destruir_buffer_nuestro(b);

    char* ruta_fcb = obtener_ruta_archivo(nombre_archivo);
    t_config* fcb = config_create(ruta_fcb);
    escribir_en_archivo_bloques(fcb, bloqueLeido, puntero);

    enviar_codigo(socket_kernel, REALIZAR_ESCRITURA_OK);
    
    log_info(logger_file_system, "Escribir Archivo: %s - Puntero: %d - Memoria: %d", nombre_archivo, puntero, dirFisica);    
    
    free(ruta_fcb);
    config_destroy(fcb);
    free(bloqueLeido);
    free(nombre_archivo);
}

// Kernel nos pasa una direccion -> lo leo del archivo pasado por parametro (en el bloques.dat)
// -> memoria escribe el dato leido en la direccion que paso kernel
void leer_archivo(){
    t_buffer* buffer = recibir_buffer(socket_kernel);
    uint32_t tam_lectura;
    
    char* nombre_archivo = buffer_read_string(buffer, &tam_lectura);
    uint32_t dirFisica = buffer_read_uint32(buffer);
    uint32_t puntero = buffer_read_uint32(buffer);
    uint32_t numPagina = buffer_read_uint32(buffer);
    uint32_t pid = buffer_read_uint32(buffer);
    destruir_buffer_nuestro(buffer);
    
    char* rutaArchivo = obtener_ruta_archivo(nombre_archivo);
    t_config* fcb = config_create(rutaArchivo);

    void* bloqueLeido = leer_en_archivo_bloques(fcb, puntero);
    
    enviar_codigo(socket_memoria, ESCRITURA_MEMORIA_SOLICITUD);
    buffer = crear_buffer_nuestro();
    buffer_write_pagina(buffer, bloqueLeido, config_file_system.tam_bloque);
    buffer_write_uint32(buffer, dirFisica);
    buffer_write_uint32(buffer, puntero);
    buffer_write_string(buffer, nombre_archivo);
    buffer_write_uint32(buffer, numPagina);
    buffer_write_uint32(buffer, pid);
    enviar_buffer(buffer, socket_memoria);
    destruir_buffer_nuestro(buffer);
    
    free(bloqueLeido);
    free(nombre_archivo);
    free(rutaArchivo);
    config_destroy(fcb);
}

void enviar_confirmacion_a_kernel_de_escritura(){
    t_buffer* buffer = recibir_buffer(socket_memoria);
    uint32_t tam;
    uint32_t dirFisica = buffer_read_uint32(buffer);
    uint32_t puntero = buffer_read_uint32(buffer);
    char* nombre_archivo = buffer_read_string(buffer, &tam);
    destruir_buffer_nuestro(buffer);
    
    log_info(logger_file_system, "Leer Archivo: %s - Puntero: %d - Memoria: %d", nombre_archivo, puntero, dirFisica);
    
    enviar_codigo(socket_kernel, REALIZAR_LECTURA_OK);
    free(nombre_archivo);
}

void abrir_archivo(){
    t_buffer* buffer = recibir_buffer(socket_kernel);
    uint32_t tam;
    char* nombreArchivo = buffer_read_string(buffer, &tam);
    destruir_buffer_nuestro(buffer);

    char* ruta_fcb_buscado = obtener_ruta_archivo(nombreArchivo);

    t_config* fcb_buscado = config_create(ruta_fcb_buscado);

    if(fcb_buscado == NULL)
        enviar_codigo(socket_kernel, NO_EXISTE_ARCHIVO);
    else{
        uint32_t tamArchivo = config_get_int_value(fcb_buscado, "TAMANIO_ARCHIVO");
        enviar_codigo(socket_kernel, ABRIR_ARCHIVO_OK);

        log_info(logger_file_system, "Abrir Archivo: %s", nombreArchivo);
        
        t_buffer* buffer_tamanio = crear_buffer_nuestro();
        buffer_write_uint32(buffer_tamanio, tamArchivo);
        enviar_buffer(buffer_tamanio, socket_kernel);
        destruir_buffer_nuestro(buffer_tamanio);
        
        config_destroy(fcb_buscado);
    }

    free(nombreArchivo);
    free(ruta_fcb_buscado);
}

void crear_archivo(){
    t_buffer* buffer = recibir_buffer(socket_kernel);
    uint32_t tam;
    char* nombreArchivo = buffer_read_string(buffer, &tam);
    destruir_buffer_nuestro(buffer);

    t_config* fcb_buscado = crear_fcb_archivo(nombreArchivo);
    config_destroy(fcb_buscado);
    
    enviar_codigo(socket_kernel, CREAR_ARCHIVO_OK);
    log_info(logger_file_system, "Crear Archivo: %s", nombreArchivo);
    free(nombreArchivo);
}

void truncar_archivo(){
    t_buffer* buffer = recibir_buffer(socket_kernel);
    uint32_t tam;
    char* nombreArchivo = buffer_read_string(buffer, &tam);
    uint32_t nuevoTam = buffer_read_uint32(buffer);
    destruir_buffer_nuestro(buffer);

    cambiar_tamanio_archivo(nombreArchivo, nuevoTam);

    log_info(logger_file_system, "Truncar Archivo: %s - Tamaño: %d", nombreArchivo, nuevoTam);

    enviar_codigo(socket_kernel, CAMBIAR_TAMANIO_OK);
    free(nombreArchivo);
}

char* obtener_ruta_archivo(char* nombre_archivo){
    char* ruta_fcb_buscado = string_new();
    string_append(&ruta_fcb_buscado, config_file_system.path_fcb);
    string_append(&ruta_fcb_buscado, nombre_archivo);
    string_append(&ruta_fcb_buscado, ".fcb");

    return ruta_fcb_buscado;
}

t_config* crear_fcb_archivo(char* nombreArchivo){
    char* rutaArchivo = obtener_ruta_archivo(nombreArchivo);

    FILE* fcbArchivo = fopen(rutaArchivo, "w");

    txt_write_in_file(fcbArchivo, "NOMBRE_ARCHIVO=");
    txt_write_in_file(fcbArchivo, nombreArchivo);
    txt_write_in_file(fcbArchivo, "\n");

    txt_write_in_file(fcbArchivo, "TAMANIO_ARCHIVO=0\n");

    uint32_t bloqueInicial = obtener_primer_bloque_libre_de_fat();
    
    char* numBloqueInicialString = string_itoa(bloqueInicial);

    txt_write_in_file(fcbArchivo, "BLOQUE_INICIAL=");
    txt_write_in_file(fcbArchivo, numBloqueInicialString);
    escribir_en_tabla_fat(UINT32_MAX, bloqueInicial);
    
    free(numBloqueInicialString);

    fclose(fcbArchivo);

    t_config* fcbBuscado = config_create(rutaArchivo);

    free(rutaArchivo);

    return fcbBuscado;
}

void cambiar_tamanio_archivo(char* nombreArchivo, uint32_t nuevo_tamanio){
    char* ruta_fcb_buscado = obtener_ruta_archivo(nombreArchivo);
    t_config* fcb_buscado = config_create(ruta_fcb_buscado);
    char* tamanio_a_aplicar;
    tamanio_a_aplicar = string_itoa(nuevo_tamanio);

    uint32_t tamanio_anterior = config_get_int_value(fcb_buscado, "TAMANIO_ARCHIVO");
    uint32_t bloque_inicial = config_get_int_value(fcb_buscado, "BLOQUE_INICIAL"); 

    config_set_value(fcb_buscado, "TAMANIO_ARCHIVO", tamanio_a_aplicar);
    config_save_in_file(fcb_buscado, ruta_fcb_buscado);

    if(nuevo_tamanio > tamanio_anterior){
        for(int i = 0; i < (nuevo_tamanio - tamanio_anterior) / config_file_system.tam_bloque; i++){
            agregar_bloque_archivo_fat(bloque_inicial);
        }
    }
    
    else{
        for(int i = 0; (tamanio_anterior - nuevo_tamanio) / config_file_system.tam_bloque; i++)
            sacar_bloque_archivo_fat(bloque_inicial);
    }

    config_destroy(fcb_buscado);
    free(ruta_fcb_buscado);
    free(tamanio_a_aplicar);
}
// FIN UTILS KERNEL

// UTILS MEMORIA
void reservar_bloque_swap(){
    t_buffer* b = recibir_buffer(socket_memoria);
    uint32_t nroPag = buffer_read_uint32(b);
    uint32_t pid = buffer_read_uint32(b);
    destruir_buffer_nuestro(b);

    uint32_t nroBloqueLibre = obtener_nro_bloque_libre();
    uint32_t posswapBloqueLibre = nroBloqueLibre * config_file_system.tam_bloque;
    
    escribir_en_bitmap(nroBloqueLibre, true);

    void* pagina = malloc(config_file_system.tam_bloque);
    
    char* byteRelleno = malloc(sizeof(char));
    string_append(&byteRelleno, "\0");
    for(int i = 0; i < config_file_system.tam_bloque; i++)
        memcpy(pagina + i, byteRelleno, sizeof(char));
    free(byteRelleno);

    escribir_pagina_en_swap(posswapBloqueLibre, pagina);
    free(pagina);

    enviar_codigo(socket_memoria, CREAR_PAGINA_RTA);

    b = crear_buffer_nuestro();
    buffer_write_uint32(b, posswapBloqueLibre);
    buffer_write_uint32(b, nroPag);
    buffer_write_uint32(b, pid);
    enviar_buffer(b, socket_memoria);
    destruir_buffer_nuestro(b);
}

void liberar_bloque_swap(){
    t_buffer* buffer = recibir_buffer(socket_memoria);
    uint32_t posicion_en_swap = buffer_read_uint32(buffer);
    destruir_buffer_nuestro(buffer);

    uint32_t nro_bloque_a_liberar = posicion_en_swap / config_file_system.tam_bloque;
    
    escribir_en_bitmap(nro_bloque_a_liberar, false);
}

void devolver_pagina(){
    t_buffer* buffer = recibir_buffer(socket_memoria);
    uint32_t posSwap = buffer_read_uint32(buffer);

    uint32_t pid = buffer_read_uint32(buffer);
	uint32_t pag_a_matar_pid_uso = buffer_read_uint32(buffer);
	uint32_t pag_a_matar_nro_pag = buffer_read_uint32(buffer);
	uint32_t nro_pagina_a_buscar = buffer_read_uint32(buffer);

    //log_warning(logger_file_system, "Nro pag a buscar: %d", nro_pagina_a_buscar);

	destruir_buffer_nuestro(buffer);

    void* pagina = leer_pagina_en_swap(posSwap);
    
    // devolver pagina a memoria
    enviar_codigo(socket_memoria, BUSCAR_PAGINA_OK);

    buffer = crear_buffer_nuestro();
    buffer_write_pagina(buffer, pagina, config_file_system.tam_bloque);
    buffer_write_uint32(buffer, pid);
    buffer_write_uint32(buffer, pag_a_matar_pid_uso);
    buffer_write_uint32(buffer, pag_a_matar_nro_pag);
    buffer_write_uint32(buffer, nro_pagina_a_buscar);
    enviar_buffer(buffer, socket_memoria);
    destruir_buffer_nuestro(buffer);
}

void actualizar_pagina(){
    t_buffer* buffer = recibir_buffer(socket_memoria);
    uint32_t posSwap = buffer_read_uint32(buffer);
    void* paginaAActualizar = buffer_read_pagina(buffer, config_file_system.tam_bloque);
    destruir_buffer_nuestro(buffer);

    // de aca vuela hasta proxima señal
    uint32_t num;
    uint32_t cantEnteros = config_file_system.tam_bloque / sizeof(uint32_t);
    uint32_t offsetEnPag;
    for(int i = 0; i < cantEnteros; i++){
        offsetEnPag = i * sizeof(uint32_t);
        memcpy(&num, paginaAActualizar + offsetEnPag, sizeof(uint32_t));
    }
    // proxima señal jeje

    escribir_pagina_en_swap(posSwap, paginaAActualizar);
    free(paginaAActualizar);
}
// FIN UTILS MEMORIA 