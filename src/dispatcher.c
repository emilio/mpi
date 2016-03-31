#include "dispatcher.h"
#include "csv.h"
#include "epoch_info.h"
#include "job.h"
#include "log.h"
#include "reply.h"
#include "request.h"
#include "tags.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef ASYNC_ROUND
#define ASSERT_IF_SYNC(cond, msg)
#define MODE_STRING "async"
#else
#define ASSERT_IF_SYNC(cond, msg) assert((cond) && msg)
#define MODE_STRING "sync"
#endif

#define CRASH_IF_SYNC(msg) ASSERT_IF_SYNC(false, msg)

void dispatch_job_requests(int workers, const char* password,
                           uint32_t current_epoch, double* total_time,
                           size_t* total_iterations, bool* finish_status,
                           FILE* csv_output) {
    assert(total_time);
    assert(total_iterations);

    // TODO: We could reuse this allocation in fact, but...
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

#ifdef ASYNC_ROUND
    // In async mode we can quit quickly if we find the password, but we should
    // wait if we don't for every possible response
    while (found_by == -1 && invalid_jobs_sent != workers) {
#else
    while (invalid_jobs_sent != workers) {
#endif
        MPI_Iprobe(MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &completed,
                   MPI_STATUS_IGNORE);
        if (completed) {
            MPI_Recv(&request, 1, REQUEST_DATA_TYPE, MPI_ANY_SOURCE,
                     REQUEST_TAG, MPI_COMM_WORLD, &status);

            if (request.epoch != current_epoch) {
                LOG("request epoch mismatch: %u != %u\n", request.epoch,
                    current_epoch);
                CRASH_IF_SYNC("request epoch mismatch in sync round model");
            }

            job_t job;
            job.epoch = request.epoch;
            if (request.epoch == current_epoch && found_by == -1 &&
                sent_works_until < MAX_PASSWORD + 1) {
                job.is_valid = 1;
                job.start = sent_works_until;
                job.length = MIN(JOB_SIZE, MAX_PASSWORD + 1 - sent_works_until);
                sent_works_until += job.length;
            } else {
                // Mark this one as finished
                if (request.epoch == current_epoch) {
                    finish_status[status.MPI_SOURCE - 1] = true;
                    invalid_jobs_sent++;
                }
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
                LOG("reply epoch mismatch: %u != %u\n", reply.epoch,
                    current_epoch);
                CRASH_IF_SYNC("reply epoch mismatch in sync mode");
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

    size_t iterations = 0;
    for (int i = 0; i < workers; ++i) {
        iterations += job_done_so_far[i];
        printf("   * Proccess %d tried: %d\n", i + 1, job_done_so_far[i]);
    }
    printf("  Total(%s): %zu iterations in %g seconds\n", password, iterations,
           time_delta);
    *total_iterations += iterations;

    if (found_by == -1) {
        printf("  No process was able to find the password\n");
        csv_write_row(csv_output, workers, password, "<not found>", time_delta,
                      job_done_so_far);
    } else {
        printf("  Process %d found the password: %s -> %s\n", found_by,
               password, successful_reply.decrypted);
        csv_write_row(csv_output, workers, password, successful_reply.decrypted,
                      time_delta, job_done_so_far);
    }

    free(job_done_so_far);
}

void* dispatcher_thread(void* arg) {
    const char** passwords = (const char**)arg;
    int process_count;

    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    int workers = process_count - 1;
    double total_time = 0.0;
    size_t total_iterations = 0;
    uint32_t current_epoch = 0;
    bool* finish_status = malloc(sizeof(bool) * workers);
    FILE* csv_output = NULL;

    // 1024 is more than enough for mode-workers-passwords-hash
    char csv_name[1024] = {0};
    csv_construct_name(MODE_STRING, workers, passwords, csv_name,
                       sizeof(csv_name));

    // Mark all as finished because if all the passwords we have are invalid
    // we'll read uninitialized memory.
    for (size_t i = 0; i < workers; ++i)
        finish_status[i] = true;

    printf("Dispatcher started, trying to save data to: %s\n", csv_name);
    printf("MPI version: %d.%d\n", MPI_VERSION, MPI_SUBVERSION);

    csv_output = fopen(csv_name, "w");
    if (!csv_output)
        WARN("Couldn't open file: %s (os error %d: %s)\n", csv_name, errno,
             strerror(errno));

    csv_write_header(csv_output, workers);

    const char* current;
    while ((current = *passwords++)) {
	LOG("Testing password: %s", current);

        if (strlen(current) != CRYPT_PASSWORD_LEN) {
            WARN("Discarding invalid password %s\n", current);
            continue;
        }

        for (int i = 0; i < workers; ++i) {
            // In synchonous mode all should have been finished, always.
            ASSERT_IF_SYNC(finish_status[i],
                           "someone didn't finish in synchronous mode");
            finish_status[i] = false;
        }

        double dispatch_begin = MPI_Wtime();
        epoch_info_t epoch_info;
        epoch_info_init(&epoch_info, current_epoch, current);
        for (int i = 0; i < workers; ++i) {
            MPI_Request req;
            MPI_Isend(&epoch_info, 1, EPOCH_INFO_DATA_TYPE, i + 1, PASSWORD_TAG,
                      MPI_COMM_WORLD, &req);
            MPI_Request_free(&req);
        }
        double dispatch_delta = MPI_Wtime() - dispatch_begin;
        total_time += dispatch_delta;

        printf("Dispatched password %s in %g seconds\n", current,
               dispatch_delta);

        dispatch_job_requests(workers, current, current_epoch, &total_time,
                              &total_iterations, finish_status, csv_output);

        current_epoch++;
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
    final_job.epoch = (uint32_t)-1;
    for (int i = 0; i < workers; ++i) {
        if (!finish_status[i]) {
            LOG("Sending end job to %d\n", i + 1);
            MPI_Request req;
            MPI_Isend(&final_job, 1, JOB_DATA_TYPE, i + 1, JOB_TAG,
                      MPI_COMM_WORLD, &req);
            MPI_Request_free(&req);
        }
    }
#endif

    free(finish_status);

    // We send an empty string to indicate the end
    char over = '\0';
    epoch_info_t epoch_info;
    epoch_info_init(&epoch_info, current_epoch + 1, &over);
    for (int i = 0; i < workers; ++i) {
        MPI_Send(&epoch_info, 1, EPOCH_INFO_DATA_TYPE, i + 1, PASSWORD_TAG,
                 MPI_COMM_WORLD);
    }

    printf("Total: %zu iterations in %g seconds\n", total_iterations,
           total_time);

    if (csv_output)
        fclose(csv_output);

    return NULL;
}
