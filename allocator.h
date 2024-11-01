#ifndef JSON_ALLOCATOR

#define JSON_MALLOC(SIZE) malloc(SIZE)
#define JSON_CALLOC(NUM, SIZE) calloc(NUM, SIZE)
#define JSON_REALLOC(PTR, NEW_SIZE, OLD_SIZE) realloc(PTR, NEW_SIZE)
#define JSON_FREE(PTR) free(PTR)

#endif