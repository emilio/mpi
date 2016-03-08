#ifndef REPLY_H_
#define REPLY_H_

#include <stdint.h>
#include <string.h>
#include <mpi.h>

#ifndef MPI_DECRYPTED_PASSWORD_LEN
#define MPI_DECRYPTED_PASSWORD_LEN 8
#endif

typedef struct reply {
    uint8_t is_success;
    char decrypted[MPI_DECRYPTED_PASSWORD_LEN + 1];
} reply_t;


void reply_init(reply_t* reply, uint8_t is_success, const char* decrypted);
void init_mpi_reply_type(MPI_Datatype* type);

#endif
