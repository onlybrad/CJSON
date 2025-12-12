#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "allocator.h"
#include "util.h"

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
    char *const ret = strdup(str);
    if(ret != NULL) {
        allocation_stats.allocated++;
    }

    return ret;
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
    struct CJSON_ArenaNode *const node = (struct CJSON_ArenaNode *)CJSON_CALLOC(sizeof(*node) + (size_t)size, sizeof(unsigned char));
    if(node == NULL) {
        return NULL;
    }

    node->size   = size;
    node->offset = 0U;
    node->next   = NULL;
    node->data   = (unsigned char*)node + sizeof(*node);
    
    return node;
}

EXTERN_C bool CJSON_Arena_init(struct CJSON_Arena *const arena, const unsigned size, const unsigned max_nodes, const char *const name) {
    assert(arena != NULL);
    assert(size > 0);

#ifndef NDEBUG
    arena->name = name;
#else
    (void)name;
#endif

    arena->current    = &arena->head;
    arena->max_nodes  = max_nodes;
    arena->node_count = 1U;
    
    arena->head.size   = size;
    arena->head.offset = 0U;
    arena->head.next   = NULL;
    arena->head.data   = (unsigned char*)CJSON_CALLOC(size, sizeof(unsigned char));
    if(arena->head.data == NULL) {
        arena->head.size = 0U;
        return false;
    }

    return true;
}

EXTERN_C void CJSON_Arena_zero(struct CJSON_Arena *const arena) {
    assert(arena != NULL);

    arena->head.data     = NULL;
    arena->head.next     = NULL;
    arena->head.size     = 0U;
    arena->head.offset   = 0U;
    arena->current       = NULL;
    arena->node_count    = 0U;
#ifndef NDEBUG
    arena->name          = NULL;
#endif

}

EXTERN_C void CJSON_Arena_free(struct CJSON_Arena *const arena) {
    assert(arena != NULL);

    arena->current    = NULL;
    arena->max_nodes  = CJSON_ARENA_INFINITE_NODES;
    arena->node_count = 1U;

    CJSON_FREE(arena->head.data);
    arena->head.data   = NULL;
    arena->head.size   = 0U;
    arena->head.offset = 0U;
    
    struct CJSON_ArenaNode *current = arena->head.next;
    arena->head.next = NULL;

    while(current != NULL) {
        struct CJSON_ArenaNode *const next = current->next;
        CJSON_FREE(current);
        current = next;
    }
}

EXTERN_C void CJSON_Arena_reset(struct CJSON_Arena *const arena) {
    assert(arena != NULL);
    
    arena->current       = &arena->head;
    arena->head.offset   = 0U;
}

EXTERN_C void *CJSON_Arena_alloc(struct CJSON_Arena *const arena, const unsigned size, unsigned alignment) {
    assert(arena != NULL);
    assert(size > 0U);
    assert((alignment & (alignment - 1U)) == 0U);

    if(alignment == 0) {
        alignment = CJSON_ALIGNOF(uintmax_t);
    }

    const uintptr_t start_address = (uintptr_t)(arena->current->data + arena->current->offset);
    uintptr_t aligned_address     = (start_address + ((uintptr_t)alignment - 1U)) & ~((uintptr_t)alignment - 1U);
    unsigned padding              = (unsigned)(aligned_address - start_address);

    if(arena->current->offset + padding + size > arena->current->size) {
        struct CJSON_ArenaNode *next = arena->current->next;

        if(next == NULL) {
            if(arena->node_count == arena->max_nodes) {
                return NULL;
            }

            unsigned node_size = arena->current->size;
            if(node_size < size) {
                if(node_size > UINT_MAX / 2U) {
                    node_size = size;
                } else do {
                    node_size *= 2U;
                } while(node_size < size);
            } 

            if((next = CJSON_ArenaNode_new(node_size)) == NULL) {
                return NULL;
            }

            arena->node_count++;
            arena->current->next = next;
        }

        aligned_address = (uintptr_t)next->data;
        padding         = 0U;
        next->offset    = 0U;
        arena->current  = next;
    }

    arena->current->offset += padding + size;
    return (void*)aligned_address;
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