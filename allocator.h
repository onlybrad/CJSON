#ifndef JSON_ALLOCATOR

#define CJSON_MALLOC(SIZE) malloc(SIZE)
#define CJSON_CALLOC(NUM, SIZE) calloc(NUM, SIZE)
#define CJSON_REALLOC(PTR, NEW_SIZE, OLD_SIZE) realloc(PTR, NEW_SIZE)
#define CJSON_FREE(PTR) free(PTR)
#define CJSON_STRDUP(STR) strdup(STR)

#endif