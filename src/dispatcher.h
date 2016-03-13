#ifndef DISPATCHER_H
#define DISPATCHER_H

#define CRYPT_PASSWORD_LEN 13

#include <mpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern MPI_Datatype REPLY_DATA_TYPE;
extern MPI_Datatype JOB_DATA_TYPE;
extern MPI_Datatype REQUEST_DATA_TYPE;

void* dispatcher_thread(void* arg);

#endif
