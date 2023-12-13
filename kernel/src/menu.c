#include <menu.h>

void menu(t_log* logger_kernel){
	
	char* leido;

	while(1){
        mostrar_menu();
		leido = readline("Ingrese mensaje: ");
		atender_consola(leido, logger_kernel);
		free(leido);
    }
}

void mostrar_menu(){
	printf("---MENU---\n");
	printf("1. INICIAR_PROCESO\n");
	printf("2. FINALIZAR_PROCESO\n");
	printf("3. INICIAR_PLANIFICACION\n");
	printf("4. DETENER_PLANIFICACION\n");
	printf("5. MULTIPROGRAMACION\n");
	printf("6. PROCESO_ESTADO\n");
	printf("7. EXIT\n");
}

void atender_consola(char* valor_leido, t_log* logger_kernel){

	char** lista_mensaje = string_split(valor_leido, " ");
	int cant_par = string_array_size(lista_mensaje) - 1; // Calcula la cantidad de parametros que recibio, se le resta 1 q es el msje en si
	codigo_operacion cod_op = get_codigo_operacion(logger_kernel, lista_mensaje[0], cant_par);

	switch(cod_op){
    	case INICIAR_PROCESO:
			iniciarProceso(lista_mensaje[1], lista_mensaje[2], lista_mensaje[3]);
			string_array_destroy(lista_mensaje); 
            break;
    	case FINALIZAR_PROCESO:
			finalizarProceso(lista_mensaje[1]);
			detectarDeadlock();
			string_array_destroy(lista_mensaje); 
    		break;
    	case INICIAR_PLANIFICACION:
    		log_info(logger_kernel, "INICIO DE PLANIFICACIÓN");
			iniciarPlanificacion();
			string_array_destroy(lista_mensaje);
    		break;
    	case DETENER_PLANIFICACION:
    		log_info(logger_kernel, "PAUSA DE PLANIFICACIÓN");
			detenerPlanificacion();
			string_array_destroy(lista_mensaje);
    		break;
    	case MULTIPROGRAMACION:
			cambiar_grado_multiprogramacion(lista_mensaje[1]);
			string_array_destroy(lista_mensaje);
    		break;
    	case PROCESO_ESTADO:
			procesosPorEstado();
			string_array_destroy(lista_mensaje); 
    		break;
		case EXIT_CONSOLA:
			string_array_destroy(lista_mensaje);
			break;
		case E_PARAMETROS:
			log_info(logger_kernel, "Error en los parametros pasados");
			string_array_destroy(lista_mensaje);
			break;
		case MENSAJE_VACIO:
			log_info(logger_kernel, "Mensaje recibido vacio");
			string_array_destroy(lista_mensaje);
			break;
		case BASURA:
			log_info(logger_kernel, "Operacion desconocida");
			string_array_destroy(lista_mensaje);
			break;
		default:
			log_warning(logger_kernel,"Operacion desconocida (default)");
			string_array_destroy(lista_mensaje);
			break;
    }
}

codigo_operacion get_codigo_operacion(t_log* logger_kernel, char* comando, int cant_par){
	if(strcmp(comando, "INICIAR_PROCESO") == 0){
		if(cant_par != 3){
			return E_PARAMETROS;
		}
		log_info(logger_kernel, "La opcion elegida es: INICIAR_PROCESO");
		return INICIAR_PROCESO;
	}
	else if(strcmp(comando, "FINALIZAR_PROCESO") == 0){
		if(cant_par != 1){
			return E_PARAMETROS;
		}
		log_info(logger_kernel, "La opcion elegida es: FINALIZAR_PROCESO");
		return FINALIZAR_PROCESO;
	}
	else if(strcmp(comando, "INICIAR_PLANIFICACION") == 0){
		if(cant_par != 0){
			return E_PARAMETROS;
		}
		log_info(logger_kernel, "La opcion elegida es: INICIAR_PLANIFICACION");
		return INICIAR_PLANIFICACION;
	}
	else if(strcmp(comando, "DETENER_PLANIFICACION") == 0){
		if(cant_par != 0){
			return E_PARAMETROS;
		}
		log_info(logger_kernel, "La opcion elegida es: DETENER_PLANIFICACION");
		return DETENER_PLANIFICACION;
	}
	else if(strcmp(comando, "MULTIPROGRAMACION") == 0){
		if(cant_par != 1){
			return E_PARAMETROS;
		}
		log_info(logger_kernel, "La opcion elegida es: MULTIPROGRAMACION");
		return MULTIPROGRAMACION;
	}
	else if(strcmp(comando, "PROCESO_ESTADO") == 0){
		if(cant_par != 0){
			return E_PARAMETROS;
		}
		log_info(logger_kernel, "La opcion elegida es: PROCESO_ESTADO");
		return PROCESO_ESTADO;
	}
	else if(strcmp(comando, "EXIT") == 0){
		if(cant_par != 0){
			return E_PARAMETROS;
		}
		log_info(logger_kernel, "La opcion elegida es: EXIT. Finalizando sistema...");
		return EXIT_CONSOLA;
	}
	else if(strcmp(comando, "") == 0){
		log_info(logger_kernel, "No se recibio ningun valor.");
		return MENSAJE_VACIO;
	}
	else{
		log_info(logger_kernel, "El valor ingresado no es correcto.");
		return BASURA;
	}
}