#ifndef ERROR_GUARD
#define ERROR_GUARD

#include <stdlib.h>

#ifdef DEBUG
#define DBG(fmt, ...) fprintf(stderr, "debug: " fmt "\n", ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define ERROR(fmt, ...) fprintf(stderr, "error: " fmt "\n", ##__VA_ARGS__)

#endif
