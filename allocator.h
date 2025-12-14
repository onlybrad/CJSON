#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_ALLOCATOR
#define CJSON_ALLOCATOR

#include <stdbool.h>
#include <stdlib.h>

#define CJSON_ARENA_INFINITE_NODES 0U

#if defined(__GNUC__) || defined(__clang__)
#   define CJSON_ALIGNOF(T) __alignof__(T)
#elif defined(_MSC_VER)
#   define CJSON_ALIGNOF(T) __alignof(T)
#else
    #error "Missing macro definition CJSON_ALIGNOF for this platform"
#endif

struct CJSON_ArenaNode {
    struct CJSON_ArenaNode *next;
    unsigned                size,
                            offset;
    //unsigned char         data[]; //use the CJSON_GET_DATA() macro to a get a pointer to this
};

struct CJSON_Arena {
    struct CJSON_ArenaNode *head,
                           *current;
    unsigned                node_count,
                            max_nodes;
#ifndef NDEBUG
    const char             *name;
#endif
};

#define CJSON_ARENA_ALLOC(ARENA, COUNT, TYPE) (TYPE*)CJSON_Arena_alloc(ARENA, (COUNT) * sizeof(TYPE), CJSON_ALIGNOF(TYPE))

void  CJSON_Arena_zero(struct CJSON_Arena*);
bool  CJSON_Arena_init  (struct CJSON_Arena*, unsigned size, unsigned max_nodes, const char *name);
void  CJSON_Arena_free  (struct CJSON_Arena*);
void  CJSON_Arena_reset (struct CJSON_Arena*);
void *CJSON_Arena_alloc (struct CJSON_Arena*, unsigned size, unsigned alignment);
char *CJSON_Arena_strdup(struct CJSON_Arena*, const char *str, unsigned *length);

#ifndef NDEBUG

struct CJSON_AllocationStats {
    unsigned allocated;
    unsigned deallocated;
};

void *CJSON_debug_malloc (size_t);
void *CJSON_debug_calloc (size_t, size_t);
void *CJSON_debug_realloc(void*, size_t);
char *CJSON_debug_strdup (const char*);
void  CJSON_debug_free   (void*);
const struct CJSON_AllocationStats *CJSON_get_allocation_stats(void);

#define CJSON_MALLOC  CJSON_debug_malloc
#define CJSON_CALLOC  CJSON_debug_calloc
#define CJSON_REALLOC CJSON_debug_realloc
#define CJSON_STRDUP  CJSON_debug_strdup
#define CJSON_FREE    CJSON_debug_free

#else

#define CJSON_MALLOC  malloc
#define CJSON_CALLOC  calloc
#define CJSON_REALLOC realloc
#define CJSON_FREE    free
#define CJSON_STRDUP  strdup

#endif

#endif

#ifdef __cplusplus
}
#endif

