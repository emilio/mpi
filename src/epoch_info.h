#ifndef EPOCH_INFO_H_
#define EPOCH_INFO_H_

#include <mpi.h>
#include <stdint.h>
#define CRYPT_PASSWORD_LEN 13

// This is the struct sent to signify the start of a new epoch.
// It contains the password to decrypt and the epoch itself.
typedef struct epoch_info {
    uint32_t epoch;
    char password[CRYPT_PASSWORD_LEN + 1];
} epoch_info_t;

void epoch_info_init(epoch_info_t* info, uint32_t epoch, const char* password);

void init_mpi_epoch_info_type(MPI_Datatype* type);
#endif
