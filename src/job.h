#ifndef JOB_H
#define JOB_H

#include <stdint.h>
#include <mpi.h>

typedef struct job {
    // A job can only be processed if it's valid.
    // An invalid job is returned if there is no more
    // remaining work to do.
    uint8_t is_valid;
    // The first password you'd want to check.
    uint32_t start;
    // How many passwords you have to check.
    uint32_t length;
} job_t;

void job_init(job_t* job,
              uint8_t is_valid,
              uint32_t start,
              uint32_t len);

void init_mpi_job_type(MPI_Datatype* type);

#endif
