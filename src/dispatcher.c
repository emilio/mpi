#include "dispatcher.h"
#include "job.h"
#include "reply.h"
#include "request.h"
#include "tags.h"
#include "epoch_info.h"
#include "log.h"
#include <assert.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void dispatch_job_requests(int workers,
                           const char* password,
                           uint32_t current_epoch,
                           double* total_time,
                           size_t* total_iterations,
                           bool*  finish_status) {
    assert(total_time);
    assert(total_iterations);

    const uint32_t WORK_SIZE = 5000;

    uint32_t* job_done_so_far = calloc(sizeof(uint32_t), workers);

    uint32_t sent_works_until = 0;
    int found_by = -1;

    MPI_Status status;
    int completed = 0;

    int invalid_jobs_sent = 0;
    reply_t reply;
    reply_t successful_reply;
    request_t request;

    double time_start = MPI_Wtime();

#ifndef ASYNC_ROUND
    while (invalid_jobs_sent != workers) {
#else
    while (found_by == -1) {
#endif
        MPI_Iprobe(MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &completed,
                   MPI_STATUS_IGNORE);
        if (completed) {
            MPI_Recv(&request, 1, REQUEST_DATA_TYPE, MPI_ANY_SOURCE,
                     REQUEST_TAG, MPI_COMM_WORLD, &status);

            if (request.epoch != current_epoch) {
                LOG("request epoch mismatch: %u != %u\n", request.epoch, current_epoch);
#ifndef ASYNC_ROUND
                assert(false && "this should never happen in synchronous mode\n");
#endif
            }

            job_t job;
            job.epoch = request.epoch;
            if (request.epoch == current_epoch && found_by == -1 && sent_works_until < MAX_PASSWORD) {
                job.is_valid = 1;
                job.start = sent_works_until;
                job.length = MIN(WORK_SIZE, MAX_PASSWORD - sent_works_until);
                sent_works_until += job.length;
            } else {
                // Mark this one as finished
                if (request.epoch == current_epoch)
                    finish_status[status.MPI_SOURCE - 1] = true;
                invalid_jobs_sent++;
                job.is_valid = 0;
            }

            MPI_Request req;
            MPI_Isend(&job, 1, JOB_DATA_TYPE, status.MPI_SOURCE, JOB_TAG,
                      MPI_COMM_WORLD, &req);
            // NB: This doesn't cancel the request, just tells mpi to remove it
            // as soond as it finishes
            MPI_Request_free(&req);
        }

        MPI_Iprobe(MPI_ANY_SOURCE, REPLY_TAG, MPI_COMM_WORLD, &completed,
                   MPI_STATUS_IGNORE);
        if (completed) {
            MPI_Recv(&reply, 1, REPLY_DATA_TYPE, MPI_ANY_SOURCE, REPLY_TAG,
                     MPI_COMM_WORLD, &status);

            if (reply.epoch != current_epoch) {
                LOG("reply epoch mismatch: %u != %u\n", reply.epoch, current_epoch);
                continue;
            }

            if (found_by != -1)
                LOG("received reply when password was already found\n");

            job_done_so_far[status.MPI_SOURCE - 1] += reply.try_count;
            if (reply.is_success) {
                successful_reply = reply;
                found_by = status.MPI_SOURCE;
            }
        }
    }

    double time_delta = MPI_Wtime() - time_start;

    // Just in case, but what the heck could
    // happen for this not to be true?
    assert(time_delta > 0);
    *total_time += time_delta;

    if (found_by == -1) {
        printf("  No process was able to find the password\n");
        free(job_done_so_far);
        return;
    }

    printf("  Process %d found the password: %s -> %s\n", found_by, password,
           successful_reply.decrypted);

    size_t iterations = 0;
    for (int i = 0; i < workers; ++i) {
        iterations += job_done_so_far[i];
        printf("   * Proccess %d tried: %d\n", i + 1, job_done_so_far[i]);
    }
    printf("  Total(%s): %zu iterations in %g seconds\n", password, iterations, time_delta);
    *total_iterations += iterations;

    free(job_done_so_far);
}

void* dispatcher_thread(void* arg) {
    char** passwords = (char**)arg;
    int process_count;

    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    double total_time = 0.0;
    size_t total_iterations = 0;
    uint32_t current_epoch = 0;
    bool* finish_status = malloc(sizeof(bool) * process_count);

    while (passwords) {
        char* current = *passwords;
        if (!current)
            break;

        if (strlen(current) != CRYPT_PASSWORD_LEN) {
            LOG("warning: Discarding invalid password %s\n", current);
            passwords++;
            continue;
        }

        for (int i = 0; i < process_count - 1; ++i) {
#ifndef ASYNC_ROUND
            // In synchonous mode all should have been finished, always.
            assert(current_epoch == 0 || finish_status[i]);
#endif
            finish_status[i] = false;
        }

        double dispatch_begin = MPI_Wtime();
        epoch_info_t epoch_info;
        epoch_info_init(&epoch_info, current_epoch, current);
        for (int i = 1; i < process_count; ++i) {
            MPI_Request req;
            MPI_Isend(&epoch_info, 1, EPOCH_INFO_DATA_TYPE, i, PASSWORD_TAG,
                      MPI_COMM_WORLD, &req);
            MPI_Request_free(&req);
        }
        double dispatch_delta = MPI_Wtime() - dispatch_begin;
        total_time += dispatch_delta;

        printf("Dispatched password %s in %g seconds\n", current, dispatch_delta);

        dispatch_job_requests(process_count - 1, current, current_epoch, &total_time, &total_iterations, finish_status);

        current_epoch++;
        passwords++;
    }

#ifdef ASYNC_ROUND
    // If the round is asynchronous we should clean up all those who didn't
    // receive it's reply back.
    //
    // There exists the chance if I'm not wrong that for two rounds they
    // haven't received the invalid job, but it's **hugely** unlikely.
    //
    // Also, the invalid epoch should serve as a hint to stop working an thus
    // this stops being a problem.
    LOG("Cleaning up remaining workers who maybe haven't finished\n");

    job_t final_job;
    final_job.is_valid = false;
    final_job.epoch = (uint32_t) -1;
    for (int i = 0; i < process_count - 1; ++i) {
        if (!finish_status[i]) {
            LOG("Sending end job to %d\n", i + 1);
            MPI_Request req;
            MPI_Isend(&final_job, 1, JOB_DATA_TYPE, i + 1, JOB_TAG,
                      MPI_COMM_WORLD, &req);
            MPI_Request_free(&req);
        }
    }
#endif

    // We send an empty string to indicate the end
    char over = '\0';
    epoch_info_t epoch_info;
    epoch_info_init(&epoch_info, current_epoch + 1, &over);
    for (int i = 1; i < process_count; ++i) {
        MPI_Request req;
        MPI_Isend(&epoch_info, 1, EPOCH_INFO_DATA_TYPE, i, PASSWORD_TAG,
                  MPI_COMM_WORLD, &req);
        MPI_Request_free(&req);
    }

    printf("Total: %zu iterations in %g seconds\n", total_iterations, total_time);

    return NULL;
}
