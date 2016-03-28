#include "csv.h"
#include "dispatcher.h"
#include "epoch_info.h"
#include <crypt.h>
#include <stdio.h>

#ifndef CSV_OUT_PATH
#define CSV_OUT_PATH "bench/data/"
#endif

#define STR_(x) #x
#define STR(x) STR_(x)

size_t csv_construct_name(const char* mode, int workers, const char** passwords,
                          char* buffer, size_t buffer_size) {
    const char** passwords_iter;
    int password_count = 0;
    char salt[3] = {0};

    buffer[0] = 0;
    passwords_iter = passwords;
    const char* current;
    while ((current = *passwords_iter++))
        if (strlen(current) == CRYPT_PASSWORD_LEN)
            password_count++;

    salt[0] = workers % 10 + '0';
    salt[1] = password_count % 10 + '0';

    char hash[100] = {0};

    char* current_hash = crypt(passwords[0], salt);
    strncpy(hash, current_hash, sizeof(hash));

    passwords_iter = passwords + 1;
    while ((current = *passwords_iter++)) {
        if (strlen(current) == CRYPT_PASSWORD_LEN) {
            strncat(hash, current, sizeof(hash) - CRYPT_PASSWORD_LEN - 1);
            current_hash = crypt(hash, salt);
            strncpy(hash, current_hash, sizeof(hash));
        }
    }

    snprintf(buffer, buffer_size,
             CSV_OUT_PATH "%s-%s-%d-workers-%d-passwords-job-size-%d-%s.csv",
             STR(BUILD_TYPE), mode, workers, password_count, JOB_SIZE, hash);
    return strlen(buffer);
}

// Our csv will be: hash, password, time, worker_x_iterations
void csv_write_header(FILE* output, size_t workers) {
    if (!output)
        return;

    fprintf(output, "hash, password, time");

    for (size_t i = 0; i < workers; ++i)
        fprintf(output, ", worker %zu iterations", i + 1);

    fprintf(output, "\n");
}

void csv_write_row(FILE* output, size_t workers, const char* hash,
                   const char* password, double time_in_seconds,
                   uint32_t* iterations_per_worker) {
    if (!output)
        return;

    fprintf(output, "\"%s\", \"%s\", %f", hash, password, time_in_seconds);

    for (size_t i = 0; i < workers; ++i)
        fprintf(output, ", %u", iterations_per_worker[i]);

    fprintf(output, "\n");
}
