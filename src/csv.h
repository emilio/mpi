#ifndef CSV_H_
#define CSV_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Gets the csv base name given passwords, worker count, and mode
size_t csv_construct_name(const char* mode, int workers, const char** passwords,
                          char* buffer, size_t buffer_size);

void csv_write_header(FILE* output, size_t workers);

void csv_write_row(FILE* output, size_t workers, const char* hash,
                   const char* password, double time_in_seconds,
                   uint32_t* iterations_per_worker);

#endif
