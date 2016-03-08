#include "reply.h"

void reply_init(reply_t* reply, uint8_t is_success, const char* decrypted) {
    reply->is_success = is_success;

    if (is_success)
        memcpy(reply->decrypted, decrypted, MPI_DECRYPTED_PASSWORD_LEN);

    reply->decrypted[MPI_DECRYPTED_PASSWORD_LEN] = 0;
}

void init_mpi_reply_type(MPI_Datatype* type) {
    int count = 2;
    int lengths[] = {1, MPI_DECRYPTED_PASSWORD_LEN};
    MPI_Aint offsets[] = { offsetof(reply_t, is_success), offsetof(reply_t, decrypted) };
    MPI_Datatype types[] = {MPI_UINT32_T, MPI_UINT32_T};

    MPI_Type_struct(count, lengths, offsets, types, type);
    MPI_Type_commit(type);
}
