#ifndef DISPATCHER_H
#define DISPATCHER_H

#define CRYPT_PASSWORD_LEN 13

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* dispatcher_thread(void* arg);

#endif
