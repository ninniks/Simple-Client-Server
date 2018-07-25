#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

// macros for handling errors
#define handle_error_en(en, msg)    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg)           do { perror(msg); exit(EXIT_FAILURE); } while (0)


// parametri di configurazione
#define SERVER_ADDRESS  "127.0.0.1"
#define SERVER_COMMAND  "QUIT"
#define SERVER_PORT     2015
#define MAX_CONN_SUPPORTED 3
#define QUIT_COMMAND "QUIT"

#endif
