#include "request.h"

// Do nothing for now
void request_init(request_t* request) {}

void init_mpi_request_type(MPI_Datatype* type) {
    int count = 1;
    int lengths[] = {1};
    MPI_Aint offsets[] = {
        offsetof(request_t, __dummy),
    };
    MPI_Datatype types[] = {
        MPI_UINT32_T,
    };

    MPI_Type_create_struct(count, lengths, offsets, types, type);
    MPI_Type_commit(type);
}
