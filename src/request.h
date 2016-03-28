#ifndef REQUEST_H_
#define REQUEST_H_

#include <mpi.h>
#include <stdint.h>

// A request, at least for now, is just a dummy object.
// Later we might want to tweak the count and all that stuff.
typedef struct request { uint32_t epoch; } request_t;

void request_init(request_t* request, uint32_t epoch);
void init_mpi_request_type(MPI_Datatype*);

#endif
