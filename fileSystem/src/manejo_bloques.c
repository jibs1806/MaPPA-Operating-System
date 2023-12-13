#include <manejo_bloques.h>

void escribir_en_bitmap(uint32_t nroBloque, uint8_t valor){
    memcpy(bitmapBloquesSwap + nroBloque, &valor, sizeof(uint8_t));
}

uint8_t leer_de_bitmap(uint32_t nroBloque){
    uint8_t estadoBloque;
    memcpy(&estadoBloque, bitmapBloquesSwap + nroBloque, sizeof(uint8_t));
    return estadoBloque;
}

void levantar_archivo_bloques(){
    int blfd = open(config_file_system.path_bloques, O_RDWR, S_IRUSR | S_IWUSR);  // bloques file descriptor tamArchivoBloques

    int tamArchivoBloques = config_file_system.cantidad_bloques_total * config_file_system.tam_bloque;  

    if(blfd == -1){
        blfd = open(config_file_system.path_bloques, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        tablaFat = mmap(NULL, tamArchivoBloques, PROT_WRITE, MAP_SHARED, blfd, 0);
        ftruncate(blfd, tamArchivoBloques);
    }
    else{
        tablaFat = mmap(NULL, tamArchivoBloques, PROT_WRITE, MAP_SHARED, blfd, 0);
        ftruncate(blfd, tamArchivoBloques);
    }

    ftruncate(blfd, tamArchivoBloques);

    bloquesMapeado = mmap(NULL, tamArchivoBloques, PROT_WRITE, MAP_SHARED, blfd, 0);
}

uint32_t obtener_nro_bloque_libre(){
    for(int nroBloque = 0; nroBloque < config_file_system.cantidad_bloques_swap; nroBloque++){
        uint8_t bloqueOcupado = leer_de_bitmap(nroBloque);
        if(!bloqueOcupado)
            return nroBloque;
    }
}

void escribir_pagina_en_swap(uint32_t posSwap, void* paginaAEscribir){
    memcpy(bloquesMapeado + posSwap, paginaAEscribir, config_file_system.tam_bloque);
    
    uint32_t nroBloque = posSwap / config_file_system.tam_bloque;
    log_info(logger_file_system, "Acceso SWAP: %d", nroBloque);
}

void* leer_pagina_en_swap(uint32_t posSwap){
    void* pagina = malloc(config_file_system.tam_bloque);
    memcpy(pagina, bloquesMapeado + posSwap, config_file_system.tam_bloque);
    
    uint32_t nroBloque = posSwap / config_file_system.tam_bloque;
    log_info(logger_file_system, "Acceso SWAP: %d", nroBloque);

    return pagina;
}

// Ya viene creado el t_config
void escribir_en_archivo_bloques(t_config* fcb, void* bloqueAEscribir, uint32_t puntero){
    uint32_t nroBloqueAEscribirBloquesDat; 
    uint32_t offsetDentroDelBloque; 

    recorrer_tabla_fat(fcb, puntero, &nroBloqueAEscribirBloquesDat, &offsetDentroDelBloque);
    
    uint32_t indiceEnBytes = nroBloqueAEscribirBloquesDat * config_file_system.tam_bloque + offsetDentroDelBloque;

    memcpy(bloquesMapeado + indiceEnBytes, bloqueAEscribir, config_file_system.tam_bloque);
}

// Ya viene creado el t_config
void* leer_en_archivo_bloques(t_config* fcb, uint32_t puntero){
    uint32_t nroBloqueALeerBloquesDat;
    uint32_t offsetDentroDelBloque;
    void* bloqueLeido = malloc(config_file_system.tam_bloque);
    
    recorrer_tabla_fat(fcb, puntero, &nroBloqueALeerBloquesDat, &offsetDentroDelBloque); //devuelve por referencia el bloque y el offeset
    
    uint32_t indiceEnBytes = nroBloqueALeerBloquesDat * config_file_system.tam_bloque + offsetDentroDelBloque;

    memcpy(bloqueLeido, bloquesMapeado + indiceEnBytes, config_file_system.tam_bloque);

    return bloqueLeido;
}

// Devuelve el bloqueFS en el que vamos a escribir y el offsetDentroDelBloque
void recorrer_tabla_fat(t_config* fcb, uint32_t puntero, uint32_t* bloqueFS, uint32_t* offsetDentroDelBloque){
    float nroBloqueAInteractuarDelArchivoFloat = ((float) puntero) / ((float) config_file_system.tam_bloque);
    uint32_t nroBloqueAInteractuarDelArchivo = (uint32_t) nroBloqueAInteractuarDelArchivoFloat;
    *offsetDentroDelBloque = puntero - nroBloqueAInteractuarDelArchivo * config_file_system.tam_bloque;
    
    int bloqueInicial = config_get_int_value(fcb, "BLOQUE_INICIAL");

    uint32_t nroBloqueBuscado = bloqueInicial;
    uint32_t bloqueAnterior;
    for(int i = 0; i < nroBloqueAInteractuarDelArchivo; i++){
        bloqueAnterior = nroBloqueBuscado;
        nroBloqueBuscado = leer_de_tabla_fat(nroBloqueBuscado);
        usleep(config_file_system.retardo_acceso_fat * 1000);
        log_info(logger_file_system, "Acceso FAT - Entrada: %d - Valor: %d", bloqueAnterior, nroBloqueBuscado);
    }
    
    *bloqueFS = nroBloqueBuscado + config_file_system.cantidad_bloques_swap;

    char* nombreArchivo = config_get_string_value(fcb, "NOMBRE_ARCHIVO");
    
    usleep(config_file_system.retardo_acceso_bloque * 1000);
    log_info(logger_file_system, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque FS: %d", nombreArchivo, nroBloqueAInteractuarDelArchivo, *bloqueFS);

    //free(nombreArchivo);
}

uint32_t obtener_primer_bloque_libre_de_fat(){
    int cantBloquesFat = config_file_system.cantidad_bloques_total - config_file_system.cantidad_bloques_swap;
    for(int i = 1; i < cantBloquesFat; i++){
        if (leer_de_tabla_fat(i) == 0)
            return i;
    }
}

void levantar_tabla_fat(){
    // tabla fat file descriptor
    int tffd = open(config_file_system.path_fat, O_RDWR, S_IRUSR | S_IWUSR);
    
    int cantBloquesFat = config_file_system.cantidad_bloques_total - config_file_system.cantidad_bloques_swap;
    int tamArchivoBloques = cantBloquesFat * sizeof(uint32_t);

    if(tffd == -1){
        tffd = open(config_file_system.path_fat, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        tablaFat = mmap(NULL, tamArchivoBloques, PROT_WRITE, MAP_SHARED, tffd, 0);
        ftruncate(tffd, tamArchivoBloques);

        for(int i = 0; i < cantBloquesFat; i++)
            escribir_en_tabla_fat(0, i);
    }
    else{
        tablaFat = mmap(NULL, tamArchivoBloques, PROT_WRITE, MAP_SHARED, tffd, 0);
        ftruncate(tffd, tamArchivoBloques);
    }
}

void escribir_en_tabla_fat(uint32_t nroAEscribir, uint32_t nroDeBloque){
    memcpy(tablaFat + (nroDeBloque * sizeof(uint32_t)), &nroAEscribir, sizeof(uint32_t));
}

uint32_t leer_de_tabla_fat(uint32_t nroDeBloque){
    uint32_t nroLeido;
    memcpy(&nroLeido, tablaFat + (nroDeBloque * sizeof(uint32_t)), sizeof(uint32_t));
    return nroLeido;
}

// El config ya viene creado
void agregar_bloque_archivo_fat(uint32_t bloqueInicial){
    uint32_t nroBloqueLeido;
    uint32_t primerBloqueLibre = obtener_primer_bloque_libre_de_fat();
    uint32_t ultimoBloqueArchivo = bloqueInicial;
    
    while(1){
        nroBloqueLeido = leer_de_tabla_fat(ultimoBloqueArchivo);
        if(nroBloqueLeido != UINT32_MAX)
            ultimoBloqueArchivo = nroBloqueLeido;
        else
            break;
    }

    escribir_en_tabla_fat(primerBloqueLibre, ultimoBloqueArchivo);

    escribir_en_tabla_fat(UINT32_MAX, primerBloqueLibre);
}

// El config ya viene creado
void sacar_bloque_archivo_fat(uint32_t bloqueInicial){
    uint32_t ultimoBloqueArchivo = bloqueInicial;
    uint32_t anteUltimoBloqueArchivo;
    
    while(ultimoBloqueArchivo != UINT32_MAX){
        anteUltimoBloqueArchivo = ultimoBloqueArchivo;
        ultimoBloqueArchivo = leer_de_tabla_fat(ultimoBloqueArchivo);
    }

    escribir_en_tabla_fat(0, ultimoBloqueArchivo);
    escribir_en_tabla_fat(UINT32_MAX,  anteUltimoBloqueArchivo);
}