#ifndef STATIC_HELLO_H_
#define STATIC_HELLO_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>


t_log* iniciar_logger(char* path, char* nombre);
t_config* iniciar_config(char* config_path);

#endif
