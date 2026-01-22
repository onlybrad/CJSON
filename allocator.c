#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "allocator.h"
#include "util.h"

#define CJSON_GET_DATA(NODE) ((unsigned char *)((NODE) + 1))

#ifndef NDEBUG

static struct CJSON_AllocationStats allocation_stats;

void *CJSON_debug_malloc(const size_t size) {
    void *const ret = malloc(size);
    if(ret != NULL) {
        allocation_stats.allocated++;
    }

    return ret;
}

void *CJSON_debug_calloc(const size_t count, const size_t size) {
    void *const ret = calloc(count, size);
    if(ret != NULL) {
        allocation_stats.allocated++;
    }

    return ret;
}

void *CJSON_debug_realloc(void *const ptr, const size_t size) {
    void *const ret = realloc(ptr, size);

    if(ret != NULL) {
        if(ptr != NULL) {
            allocation_stats.deallocated++;
        }
        allocation_stats.allocated++;
    }

    return ret;
}

char *CJSON_debug_strdup(const char *const str) {
#if defined(__MINGW32__) || !defined(_WIN32)
    #define CJSON_STRDUP_FUNC strdup
#else
    #define CJSON_STRDUP_FUNC _strdup
#endif

    char *const ret = CJSON_STRDUP_FUNC(str);
    if(ret != NULL) {
        allocation_stats.allocated++;
    }

    return ret;

#undef CJSON_STRDUP_FUNC
}

void CJSON_debug_free(void *ptr) {
    if(ptr == NULL) {
        return;
    }

    free(ptr);
    allocation_stats.deallocated++;
}

const struct CJSON_AllocationStats *CJSON_get_allocation_stats(void) {
    return &allocation_stats;
}

#endif

static struct CJSON_ArenaNode *CJSON_ArenaNode_new(const unsigned size) {
    assert(size > 0);

    struct CJSON_ArenaNode *const node = (struct CJSON_ArenaNode *)CJSON_CALLOC(sizeof(*node) + (size_t)size, sizeof(unsigned char));
    if(node == NULL) {
        return NULL;
    }

    node->size   = size;
    node->offset = 0U;
    node->next   = NULL;
    
    return node;
}

static bool CJSON_Arena_create_next_node(struct CJSON_Arena *const arena, const unsigned size) {
    assert(arena != NULL);
    assert(size > 0);

    unsigned node_size = arena->current->size;
    if(node_size < size) {
        if(node_size > UINT_MAX / 2U) {
            node_size = size;
        } else do {
            node_size *= 2U;
        } while(node_size < size);
    }

    struct CJSON_ArenaNode *const current = arena->current;
    struct CJSON_ArenaNode *const next    = current->next;
    if(next == NULL) {
        if(arena->node_count == arena->node_max) {
            return false;
        }

        if((current->next = CJSON_ArenaNode_new(node_size)) != NULL) {
            arena->current = current->next;
            arena->node_count++;
            return true;
        }

        return false;
    }

    if(next->size < size) {
        struct CJSON_ArenaNode *const next_next = next->next;
        CJSON_FREE(next);

        if((current->next = CJSON_ArenaNode_new(node_size)) != NULL) {
            current->next->next = next_next;
            arena->current      = current->next;
            return true;
        }

        current->next = next_next;
        arena->node_count--;
        return false;
    }

    return true;
}

EXTERN_C void CJSON_Arena_init(struct CJSON_Arena *const arena, const unsigned node_max, const char *const name) {
    assert(arena != NULL);

    arena->node_count = 0U;
    arena->node_max   = node_max;
    arena->head       = NULL;
    arena->current    = NULL;

#ifndef NDEBUG
    arena->name = name;
#else
    (void)name;
#endif

}

EXTERN_C bool CJSON_Arena_create_node(struct CJSON_Arena *const arena, unsigned size) {
    assert(arena != NULL);
    assert(size > 0U);

    if(arena->head != NULL) {
        return true;
    }

    if(size < CJSON_ARENA_MINIMUM_SIZE) {
        size = CJSON_ARENA_MINIMUM_SIZE;
    }

    arena->current = arena->head = CJSON_ArenaNode_new(size);

    if(arena->head != NULL) {
        arena->node_count = 1U;
    }

    return arena->head != NULL;
}

EXTERN_C void CJSON_Arena_free(struct CJSON_Arena *const arena) {
    assert(arena != NULL);
    
    struct CJSON_ArenaNode *current = arena->head;
    arena->head = NULL;
    while(current != NULL) {
        struct CJSON_ArenaNode *const next = current->next;
        CJSON_FREE(current);
        current = next;
    }

#ifndef NDEBUG
    CJSON_Arena_init(arena, arena->node_max, arena->name);
#else
    CJSON_Arena_init(arena, arena->node_max, NULL);
#endif
}

EXTERN_C void CJSON_Arena_reset(struct CJSON_Arena *const arena) {
    assert(arena != NULL);
    
    arena->current      = arena->head;
    arena->head->offset = 0U;
}

EXTERN_C void *CJSON_Arena_alloc_objects(struct CJSON_Arena *const arena, const unsigned count, const unsigned size, const unsigned alignment) {
    assert(arena != NULL);
    assert(count > 0U);
    assert(size > 0U);
    assert((alignment & (alignment - 1U)) == 0U);

    bool success;
    const unsigned total_size = CJSON_safe_unsigned_mult(count, size, &success);
    
    return success ? CJSON_Arena_alloc(arena, total_size, alignment) : NULL;
}

EXTERN_C void *CJSON_Arena_alloc(struct CJSON_Arena *const arena, const unsigned size, unsigned alignment) {
    assert(arena != NULL);
    assert(size > 0U);
    assert((alignment & (alignment - 1U)) == 0U);

    if(alignment == 0) {
        alignment = CJSON_ALIGNOF(uintmax_t);
    }

    if(!CJSON_Arena_create_node(arena, size)) {
        return NULL;
    }

    const uintptr_t start_address = (uintptr_t)(CJSON_GET_DATA(arena->current) + arena->current->offset);
    uintptr_t aligned_address     = (start_address + ((uintptr_t)alignment - 1U)) & ~((uintptr_t)alignment - 1U);
    unsigned padding              = (unsigned)(aligned_address - start_address);

    if(arena->current->offset + padding + size > arena->current->size) {
        if(!CJSON_Arena_create_next_node(arena, size)) {
            return NULL;
        }

        aligned_address = (uintptr_t)(CJSON_GET_DATA(arena->current));
        padding         = 0U;
    }

    arena->current->offset += padding + size;
    return (void*)aligned_address;
}

bool CJSON_Arena_reserve(struct CJSON_Arena *const arena, const unsigned size, unsigned alignment) {
    assert(arena != NULL);
    assert(size > 0U);
    assert((alignment & (alignment - 1U)) == 0U);

    if(alignment == 0) {
        alignment = CJSON_ALIGNOF(uintmax_t);
    }

    if(!CJSON_Arena_create_node(arena, size)) {
        return NULL;
    }

    const uintptr_t start_address = (uintptr_t)(CJSON_GET_DATA(arena->current) + arena->current->offset);
    uintptr_t aligned_address     = (start_address + ((uintptr_t)alignment - 1U)) & ~((uintptr_t)alignment - 1U);
    unsigned padding              = (unsigned)(aligned_address - start_address);

    if(arena->current->offset + padding + size <= arena->current->size) {
        return true;
    }

    return CJSON_Arena_create_next_node(arena, size);
}


EXTERN_C char *CJSON_Arena_strdup(struct CJSON_Arena *const arena, const char *const str, unsigned *const length) {
    assert(arena != NULL);
    assert(str != NULL);

    const size_t len = strlen(str);
    if(len >= (size_t)UINT_MAX) {
        return NULL;
    }

    char *const copy = CJSON_ARENA_ALLOC(arena, (unsigned)len + 1U, char);
    if(copy == NULL) {
        return NULL;
    }

    if(length != NULL) {
        *length = (unsigned)len;
    }
    
    return strcpy(copy, str);
}