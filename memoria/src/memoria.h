#ifndef SRC_MEMORIA_H_
#define SRC_MEMORIA_H_

#include <memInstrucciones.h>
#include <utils/utils.h>
#include <utils/conexiones.h>
#include <utils/instrucciones.h>
#include <utils/serializacion.h>
#include <commons/string.h>
#include <pthread.h>
#include <diccionarioMem.h>



// FIRMAS *****************************************************************************************
// INICIALIZAR MODULO -------------------------------------------------------------------
void inicializarModulo(char* config_path);
void levantarLogger();
void levantarConfig(char* config_path);
void inicializarVariables();

void enviar_tamanio_pagina();
// FIN INICIALIZAR MODULO ---------------------------------------------------------------

// CONEXIONES ---------------------------------------------------------------------------
void inicializarConexiones();
void conectarFileSystem();
void conectarKernel(int socket_servidor);
void conectarCpu(int socket_servidor);
// FIN CONEXIONES -----------------------------------------------------------------------

// UTILS KERNEL -------------------------------------------------------------------------
void atender_kernel();
void iniciar_proceso();
void finalizarProceso();
void vaciar_marco(uint32_t nroMarco);
t_pagina* liberar_marco(); // elige pagina victima, si esta se habia modificado => la mando a FS
void atender_page_fault();
bool hay_marcos_libres();
uint32_t obtener_marco_libre();
void instruccion_destroy(t_instruccion* intruccion_a_destruir);
void proceso_destroy(t_proceso* proceso_a_destruir);
void liberarPaginasDeUnProceso(t_proceso* procesoAEliminar);
void liberar_pos_swap(uint32_t posicion_swap);
t_proceso* crearProceso(t_list* listaInstrucciones, uint32_t pid, uint32_t tamanio);
// FIN UTILS KERNEL ---------------------------------------------------------------------

// UTILS FILE SYSTEM
void atender_file_system();
void asignar_bloque_swap_a_pag();
void enviar_bloque_a_fs();
void escribir_bloque_para_fs();
// FIN UTILS FILE SYSTEM


// UTILS MANEJO DE PAGINAS --------------------------------------------------------------
t_pagina* elegir_pagina_victima();
t_pagina* retirarPaginaPorLRU();
int obtenerTiempoEnMiliSegundos(char* tiempo);
t_pagina* retirarPaginaPorFIFO();
void escribir_pagina(uint32_t posEnMemoria, void* pagina);
void* leer_pagina(uint32_t posEnMemoria);
t_pagina* crear_pagina(uint32_t nroPag, uint32_t nroMarco, void* dirreccionInicio, uint32_t pid);
void colocar_pagina_en_marco();
// FIN UTILS MANEJO DE PAGINAS ----------------------------------------------------------

// FIN FIRMAS *************************************************************************************

#endif /* SRC_MEMORIA_H_ */
