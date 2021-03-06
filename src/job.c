#include "job.h"

void job_init(job_t* job, uint8_t is_valid, uint32_t start, uint32_t len,
              uint32_t epoch) {
    job->is_valid = is_valid;
    job->epoch = epoch;
    if (is_valid) {
        job->start = start;
        job->length = len;
    }
}

void init_mpi_job_type(MPI_Datatype* type) {
    int count = 4;
    int lengths[] = {1, 1, 1, 1};

    MPI_Aint offsets[] = {offsetof(job_t, is_valid), offsetof(job_t, start),
                          offsetof(job_t, length), offsetof(job_t, epoch)};

    MPI_Datatype types[] = {MPI_UINT8_T, MPI_UINT32_T, MPI_UINT32_T,
                            MPI_UINT32_T};

    MPI_Type_create_struct(count, lengths, offsets, types, type);
    MPI_Type_commit(type);
}
