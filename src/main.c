#include <mpi.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <crypt.h>
#include <pthread.h>
#include "job.h"
#include "reply.h"
#include "dispatcher.h"

typedef char crypt_password_t[CRYPT_PASSWORD_LEN + 1];

MPI_Datatype JOB_DATA_TYPE;
MPI_Datatype REPLY_DATA_TYPE;

pthread_t DISPATCHER_THREAD;

int main(int argc, char** argv) {
    int process_count;
    int process_id;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);

    init_mpi_job_type(&JOB_DATA_TYPE);
    init_mpi_reply_type(&REPLY_DATA_TYPE);

    // We don't care about the program name
    argv++;
    argc--;

    if (!argc) {
        fprintf(stderr, "At least one argument is required\n");
        MPI_Finalize();
        exit(1);
    }

    if (process_id == 0)
        pthread_create(&DISPATCHER_THREAD, NULL, dispatcher_thread, argv);

    printf("Process %d started, %s\n", process_id, argv[0]);

    // Expect the work message
    char password[14] = { 0 };
    size_t password_index = 0;
    while (true) {
        if (process_id == 0) { // Already knows it
            if (strlen(argv[password_index]) != CRYPT_PASSWORD_LEN)
                break;
            memcpy(password, argv[password_index], CRYPT_PASSWORD_LEN);
        } else {
            MPI_Recv(&password, CRYPT_PASSWORD_LEN, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        password_index++;

        // TODO

        printf("Process %d got password: %s\n", process_id, password);
        break;
    }

    if (process_id == 0)
        pthread_join(DISPATCHER_THREAD, NULL);

    MPI_Finalize();
}
