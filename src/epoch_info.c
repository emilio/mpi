#include "epoch_info.h"
#include <string.h>

void epoch_info_init(epoch_info_t* info, uint32_t epoch, const char* password) {
    info->epoch = epoch;
    strncpy(info->password, password, CRYPT_PASSWORD_LEN);
    info->password[CRYPT_PASSWORD_LEN] = 0;
}

void init_mpi_epoch_info_type(MPI_Datatype* type) {
    int count = 2;
    int lengths[] = {1, CRYPT_PASSWORD_LEN + 1};

    MPI_Aint offsets[] = {
        offsetof(epoch_info_t, epoch), offsetof(epoch_info_t, password),
    };

    MPI_Datatype types[] = {
        MPI_UINT32_T, MPI_CHAR,
    };

    MPI_Type_create_struct(count, lengths, offsets, types, type);
    MPI_Type_commit(type);
}
