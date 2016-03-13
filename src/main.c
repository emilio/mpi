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
#include "tags.h"
#include "job.h"
#include "reply.h"
#include "dispatcher.h"
#include "request.h"

typedef char crypt_password_t[CRYPT_PASSWORD_LEN + 1];

MPI_Datatype JOB_DATA_TYPE;
MPI_Datatype REPLY_DATA_TYPE;
MPI_Datatype REQUEST_DATA_TYPE;

pthread_t DISPATCHER_THREAD;

// This function takes care of sending job requests to the parent, and
// returning once there's no more job to do for the current password.
void process_password(int me, const char* pass) {
    while (true) {
        // Send the job request
        request_t request;
        request_init(&request);
        MPI_Send(&request, 1, REQUEST_DATA_TYPE, 0, REQUEST_TAG, MPI_COMM_WORLD);

        job_t job;
        MPI_Recv(&job, 1, JOB_DATA_TYPE, 0, JOB_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (!job.is_valid)
            break; // No more work to do for this password

        // printf("Got work to do: %d, %d\n", job.start, job.length);
        char salt[3] = { 0 };
        salt[0] = pass[0];
        salt[1] = pass[1];
        char to_try[9] = { 0 };
        bool found = false;
        uint32_t i = 0;
        for (; i < job.length; ++i) {
            snprintf(to_try, sizeof(to_try), "%08u", job.start);
            if (strcmp(crypt(to_try, salt), pass) == 0) {
                // printf("Found: %d\n", job.start);
                found = true;
                break;
            }

            job.start++;
        }

        reply_t reply;
        reply_init(&reply, found, i, to_try);

        MPI_Request req;
        MPI_Isend(&reply, 1, REPLY_DATA_TYPE, 0, REPLY_TAG, MPI_COMM_WORLD, &req);
        MPI_Request_free(&req);
    }
}

int main(int argc, char** argv) {
    int process_count;
    int process_id;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);

    init_mpi_job_type(&JOB_DATA_TYPE);
    init_mpi_reply_type(&REPLY_DATA_TYPE);
    init_mpi_request_type(&REQUEST_DATA_TYPE);

    // We don't care about the program name
    argv++;
    argc--;

    if (!argc) {
        fprintf(stderr, "At least one argument is required\n");
        MPI_Finalize();
        exit(1);
    }

    // TODO: Make process 0 work too instead of just dispatch new jobs.
    // It's difficult to not fall into data races or interlock.
    if (process_id == 0) {
        dispatcher_thread(argv);
        // pthread_create(&DISPATCHER_THREAD, NULL, dispatcher_thread, argv);
    } else {

        char password[14] = { 0 };
        while (true) {
            MPI_Recv(&password, CRYPT_PASSWORD_LEN, MPI_CHAR, 0, PASSWORD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (!*password)
                break; // Empty string -> it's over

            // printf("Process %d got password: %s\n", process_id, password);
            process_password(process_id, password);
        }
    }

    // if (process_id == 0)
    //    pthread_join(DISPATCHER_THREAD, NULL);
    if (process_id == 0)
        printf("Everything is over!\n");

    MPI_Finalize();
}
