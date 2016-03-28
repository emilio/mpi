#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <mpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef JOB_SIZE
#define JOB_SIZE 5000
#endif

extern MPI_Datatype REPLY_DATA_TYPE;
extern MPI_Datatype JOB_DATA_TYPE;
extern MPI_Datatype REQUEST_DATA_TYPE;
extern MPI_Datatype EPOCH_INFO_DATA_TYPE;

void* dispatcher_thread(void* arg);

#endif
