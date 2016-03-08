#include "dispatcher.h"

void* dispatcher_thread(void* arg) {
    char** passwords = (char**) arg;
    int process_count;

    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    printf("Dispatcher started: %s\n", passwords[0]);

    while (passwords) {
        char* current = *passwords;
        if (!current)
            break;

        if (strlen(current) != CRYPT_PASSWORD_LEN) {
            printf("Discarding invalid password %s\n", current);
            continue;
        }

        printf("Dispatching password %s\n", current);
        MPI_Bcast(current, CRYPT_PASSWORD_LEN, MPI_CHAR, 0, MPI_COMM_WORLD);

        passwords++;
    }

    return NULL;
}
