#include "reply.h"

void reply_init(reply_t* reply, uint8_t is_success, uint32_t try_count, uint32_t epoch,
                const char* decrypted) {
    reply->is_success = is_success;
    reply->try_count = try_count;
    reply->epoch = epoch;

    if (is_success)
        memcpy(reply->decrypted, decrypted, MPI_DECRYPTED_PASSWORD_LEN);

    reply->decrypted[MPI_DECRYPTED_PASSWORD_LEN] = 0;
}

void init_mpi_reply_type(MPI_Datatype* type) {
    int count = 4;
    int lengths[] = {1, 1, 1, MPI_DECRYPTED_PASSWORD_LEN + 1};
    MPI_Aint offsets[] = {offsetof(reply_t, is_success),
                          offsetof(reply_t, try_count),
                          offsetof(reply_t, epoch),
                          offsetof(reply_t, decrypted)};
    MPI_Datatype types[] = {MPI_UINT8_T, MPI_UINT32_T, MPI_UINT32_T, MPI_CHAR};

    MPI_Type_create_struct(count, lengths, offsets, types, type);
    MPI_Type_commit(type);
}
