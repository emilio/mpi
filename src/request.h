#ifndef REQUEST_H_
#define REQUEST_H_

#include <stdint.h>
#include <mpi.h>

// A request, at least for now, is just a dummy object.
// Later we might want to tweak the count and all that stuff.
typedef struct request {
    uint32_t __dummy;
} request_t;

void request_init(request_t* request);
void init_mpi_request_type(MPI_Datatype*);

#endif
