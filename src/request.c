#include "request.h"

void request_init(request_t* request, uint32_t epoch) {
    request->epoch = epoch;
}

void init_mpi_request_type(MPI_Datatype* type) {
    int count = 1;
    int lengths[] = {1};
    MPI_Aint offsets[] = {
        offsetof(request_t, epoch),
    };
    MPI_Datatype types[] = {
        MPI_UINT32_T,
    };

    MPI_Type_create_struct(count, lengths, offsets, types, type);
    MPI_Type_commit(type);
}
