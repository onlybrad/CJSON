#ifndef JSON_ALLOCATOR

#ifdef COUNT_ALLOCATIONS
extern int malloced;
extern int freed;

#define CJSON_MALLOC(SIZE) (malloced++,malloc(SIZE))
#define CJSON_CALLOC(NUM, SIZE) (malloced++,calloc(NUM, SIZE))
#define CJSON_REALLOC(PTR, NEW_SIZE, OLD_SIZE) realloc(PTR, NEW_SIZE)
#define CJSON_FREE(PTR) (freed++,free(PTR))
#define CJSON_STRDUP(STR) (malloced++,strdup(STR))

#else

#define CJSON_MALLOC(SIZE) malloc(SIZE)
#define CJSON_CALLOC(NUM, SIZE) calloc(NUM, SIZE)
#define CJSON_REALLOC(PTR, NEW_SIZE, OLD_SIZE) realloc(PTR, NEW_SIZE)
#define CJSON_FREE(PTR) free(PTR)
#define CJSON_STRDUP(STR) strdup(STR)

#endif

#endif

