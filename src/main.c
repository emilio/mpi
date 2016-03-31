#include "mpi.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "dispatcher.h"
#include "epoch_info.h"
#include "job.h"
#include "log.h"
#include "reply.h"
#include "request.h"
#include "tags.h"
#include <assert.h>
#include <crypt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef char crypt_password_t[CRYPT_PASSWORD_LEN + 1];
typedef char password_answer_t[9];

MPI_Datatype JOB_DATA_TYPE;
MPI_Datatype REPLY_DATA_TYPE;
MPI_Datatype REQUEST_DATA_TYPE;
MPI_Datatype EPOCH_INFO_DATA_TYPE;

pthread_t DISPATCHER_THREAD;

// This function processes a valid job, using the `answer` buffer as output.
//
// Returns the number of iterations done.
uint32_t process_valid_job(job_t* job, const char* pass,
                           password_answer_t answer, bool* found) {
    assert(job);
    assert(job->is_valid);
    assert(found);

    char salt[3] = {0};
    salt[0] = pass[0];
    salt[1] = pass[1];

    // Ensure it's null-terminated
    answer[sizeof(password_answer_t) - 1] = 0;
    *found = false;
    uint32_t i = 0;
    for (; i < job->length; ++i) {
        snprintf(answer, sizeof(password_answer_t), "%08u", job->start);
        if (strcmp(crypt(answer, salt), pass) == 0) {
            *found = true;
            return i;
        }

        job->start++;
    }

    return i;
}

// This function takes care of sending job requests to the parent, and
// returning once there's no more job to do for the current password.
//
// It returns false if the process should do no more job.
bool process_password(int me, const char* pass, uint32_t current_epoch) {
    bool found = false;
    MPI_Request req;
    while (true) {
        // Send the job request
        request_t request;
        request_init(&request, current_epoch);
        MPI_Isend(&request, 1, REQUEST_DATA_TYPE, 0, REQUEST_TAG,
                  MPI_COMM_WORLD, &req);
        MPI_Request_free(&req);

        job_t job;
        MPI_Recv(&job, 1, JOB_DATA_TYPE, 0, JOB_TAG, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        // If the job epoch doesn't match, we should totally stop working, not
        // just go on to the next round
        if (!job.is_valid) {
            LOG("Process %d received invalid job, epoch: %d, current: %d\n", me,
                job.epoch, current_epoch);
            return job.epoch == current_epoch;
        }

        assert(job.epoch == current_epoch);

        LOG("Process %d: %u..%u\n", me, job.start, job.start + job.length);

        password_answer_t answer;
        uint32_t iterations = process_valid_job(&job, pass, answer, &found);

        reply_t reply;
        reply_init(&reply, found, iterations, current_epoch, answer);

        MPI_Isend(&reply, 1, REPLY_DATA_TYPE, 0, REPLY_TAG, MPI_COMM_WORLD,
                  &req);
        MPI_Request_free(&req);
    }
}

// This is the fallback for when just one process is available
void sequential_fallback(char** passwords) {
    double total_time = 0;
    size_t total_iterations = 0;

    while (passwords) {
        char* current = *passwords;
        if (!current)
            break;

        if (strlen(current) != CRYPT_PASSWORD_LEN) {
            LOG("warning: Discarding invalid password %s\n", current);
            passwords++;
            continue;
        }

        double begin = MPI_Wtime();

        job_t job;
        // Create a job for every possible combination of the
        // password
        job_init(&job, true, 0, MAX_PASSWORD + 1, 0);

        bool found;
        password_answer_t maybe_answer;
        size_t iterations =
            process_valid_job(&job, current, maybe_answer, &found);

        double delta = MPI_Wtime() - begin;
        assert(delta > 0);

        total_time += delta;
        total_iterations += iterations;

        if (found) {
            printf("Process 0 found password %s -> %s\n", current,
                   maybe_answer);
        } else {
            printf("Process 0 unable to find password\n");
        }

        printf("  Total(%s): %zu iterations in %g seconds\n", current,
               iterations, delta);

        passwords++;
    }

    printf("Total: %zu iterations in %g seconds\n", total_iterations,
           total_time);
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
    init_mpi_epoch_info_type(&EPOCH_INFO_DATA_TYPE);

    // We don't care about the program name
    argv++;
    argc--;

    if (!argc) {
        fprintf(stderr, "At least one argument is required\n");
        MPI_Finalize();
        exit(1);
    }

    if (process_count == 1) {
        LOG("warning: Just one worker found, using sequential fallback\n");
        sequential_fallback(argv);
        MPI_Finalize();
        exit(0);
    }

    // TODO: Make process 0 work too instead of just dispatch new jobs.
    // It's difficult to not fall into data races or interlock.
    if (process_id == 0) {
        dispatcher_thread(argv);
    } else {
        epoch_info_t info;
        bool should_continue_doing_work = true;
        while (true) {
            MPI_Recv(&info, 1, EPOCH_INFO_DATA_TYPE, 0, PASSWORD_TAG,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (!info.password[0])
                break; // Empty string -> it's over

            if (should_continue_doing_work) {
                // NB: We don't just break here, since the final epoch message
                // should arrive regardless.
                should_continue_doing_work =
                    process_password(process_id, info.password, info.epoch);
            }
        }
    }

    if (process_id == 0)
        LOG("Everything is over!\n");

    MPI_Finalize();
    return 0;
}
