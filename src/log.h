#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG
#  define LOG(...) fprintf(stderr, "log: " __VA_ARGS__)
#else
#  define LOG(...)
#endif

#endif
