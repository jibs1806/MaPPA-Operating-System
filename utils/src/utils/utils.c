#include <utils/utils.h>

t_log* iniciar_logger(char* path, char* nombre) {
	t_log* nuevo_logger;

	nuevo_logger = log_create(path, nombre, 1, LOG_LEVEL_INFO);

	if (!nuevo_logger) {
		printf("error en el logger create\n");
		exit(1);
	}

	return nuevo_logger;
}

t_config* iniciar_config(char* config_path)
{
	t_config* nuevo_config;

	nuevo_config = config_create(config_path);

	if (nuevo_config == NULL){
		printf("Error al crear el nuevo config\n");
		exit(2);
	}

	return nuevo_config;
}