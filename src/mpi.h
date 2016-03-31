#include <mpi.h>

// This header exists because older openmpi has no MPI_UINT32_T,
// and no MPI_UINT8_T

#ifndef MPI_UINT32_T
#define MPI_UINT32_T MPI_UNSIGNED
#endif

#ifndef MPI_UINT8_T
#define MPI_UINT8_T MPI_UNSIGNED_CHAR
#endif
