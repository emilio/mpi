#ifndef JOB_H
#define JOB_H

#include <mpi.h>
#include <stdint.h>

static const uint32_t MAX_PASSWORD = 99999999;

typedef struct job {
    // A job can only be processed if it's valid.
    // An invalid job is returned if there is no more
    // remaining work to do.
    uint8_t is_valid;
    // The first password you'd want to check.
    uint32_t start;
    // How many passwords you have to check.
    uint32_t length;
    // A monothonic counter used to identify a batch of
    // job/request pairs.
    uint32_t epoch;
} job_t;

void job_init(job_t* job, uint8_t is_valid, uint32_t start, uint32_t len,
              uint32_t epoch);

void init_mpi_job_type(MPI_Datatype* type);

#endif
