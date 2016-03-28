#ifndef LOG_H_
#define LOG_H_

#define WARN(...) fprintf(stderr, "warning: " __VA_ARGS__)

#ifdef DEBUG
#define LOG(...) fprintf(stderr, "log: " __VA_ARGS__)
#else
#define LOG(...)
#endif

#endif
