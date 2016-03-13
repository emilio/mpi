#include "dispatcher.h"
#include "request.h"
#include "reply.h"
#include "job.h"
#include "tags.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void dispatch_job_requests(int workers, const char* password) {
    const uint32_t WORK_SIZE = 5000;
    const uint32_t MAX_PASSWORD = 99999999;

    uint32_t* job_done_so_far = calloc(sizeof(uint32_t), workers);

    uint32_t sent_works_until = 0;
    int found_by = -1;

    MPI_Status status;
    int completed = 0;

    int invalid_jobs_sent = 0;
    reply_t reply;
    reply_t successful_reply;
    request_t request;
    while (invalid_jobs_sent != workers) {
        MPI_Iprobe(MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &completed, MPI_STATUS_IGNORE);
        if (completed) {
            MPI_Recv(&request, 1, REQUEST_DATA_TYPE, MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &status);

            job_t job;
            if (found_by == -1 && sent_works_until < MAX_PASSWORD) {
                job.is_valid = 1;
                job.start = sent_works_until;
                job.length = MIN(WORK_SIZE, MAX_PASSWORD - sent_works_until);
                sent_works_until += job.length;
            } else {
                invalid_jobs_sent++;
                job.is_valid = 0;
            }

            MPI_Request req;
            MPI_Isend(&job, 1, JOB_DATA_TYPE, status.MPI_SOURCE, JOB_TAG, MPI_COMM_WORLD, &req);
            // NB: This doesn't cancel the request, just tells mpi to remove it as
            // soond as it finish
            MPI_Request_free(&req);
        }

        MPI_Iprobe(MPI_ANY_SOURCE, REPLY_TAG, MPI_COMM_WORLD, &completed, MPI_STATUS_IGNORE);
        if (completed) {
            MPI_Recv(&reply, 1, REPLY_DATA_TYPE, MPI_ANY_SOURCE, REPLY_TAG, MPI_COMM_WORLD, &status);
            job_done_so_far[status.MPI_SOURCE - 1] += reply.try_count;
            if (reply.is_success) {
                successful_reply = reply;
                found_by = status.MPI_SOURCE;
            }
        }
    }

    if (found_by == -1) {
        printf("No process was able to find the password\n");
        free(job_done_so_far);
        return;
    }

    printf("Process %d found the password: %s -> %s\n", found_by, password, successful_reply.decrypted);
    for (int i = 0; i < workers; ++i) {
        printf("Proccess %d tried: %d\n", i + 1, job_done_so_far[i]);
    }
    free(job_done_so_far);
}


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
            passwords++;
            continue;
        }

        printf("Dispatching password %s\n", current);
        for (int i = 1; i < process_count; ++i) {
            MPI_Request req;
            MPI_Isend(current, CRYPT_PASSWORD_LEN, MPI_CHAR, i, PASSWORD_TAG, MPI_COMM_WORLD, &req);
            MPI_Request_free(&req);
        }

        // Dispatch job requests
        dispatch_job_requests(process_count - 1, current);

        passwords++;
    }


    // We send an empty string to indicate the end
    char over = '\0';
    for (int i = 1; i < process_count; ++i) {
        MPI_Request req;
        MPI_Isend(&over, 1, MPI_CHAR, i, PASSWORD_TAG, MPI_COMM_WORLD, &req);
        MPI_Request_free(&req);
    }

    return NULL;
}
