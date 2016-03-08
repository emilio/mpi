#ifndef JOB_H
#define JOB_H

#include <stdint.h>
#include <mpi.h>

typedef struct job {
    uint32_t start;
    uint32_t length;
} job_t;

void job_init(job_t* job, uint32_t start, uint32_t len);

void init_mpi_job_type(MPI_Datatype* type);

#endif
