#ifndef JSON_ALLOCATOR

#define MIN(A,B) ((A)>(B)?(B):(A))

#include <string.h>

#define JSON_MALLOC(SIZE) malloc(SIZE)
#define JSON_CALLOC(NUM, SIZE) calloc(NUM, SIZE)
#define JSON_REALLOC(PTR, NEW_SIZE, OLD_SIZE) realloc(PTR, NEW_SIZE)
#define JSON_FREE(PTR) free(PTR)

#endif