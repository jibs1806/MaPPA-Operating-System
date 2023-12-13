#ifndef SRC_MENU_H_
#define SRC_MENU_H_

#include <commons/log.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <utils/conexiones.h>
#include <kernel.h>

// ENUMS
typedef enum{
    EXIT_CONSOLA,
    INICIAR_PROCESO,
    FINALIZAR_PROCESO,
    INICIAR_PLANIFICACION,
    DETENER_PLANIFICACION,
    MULTIPROGRAMACION,
    PROCESO_ESTADO,
    E_PARAMETROS,
    MENSAJE_VACIO,
    BASURA
}codigo_operacion;
// FIN ENUMS

// UTILS CONSOLA
void menu(t_log* logger_kernel);
void mostrar_menu();
void atender_consola(char* valor_leido, t_log* logger_kernel);
codigo_operacion get_codigo_operacion(t_log* logger_kernel, char* comando, int cant_par);
// FIN UTILS CONSOLA

#endif /* SRC_MENU_H_ */
