#include <assert.h>
#include "stack.h"
#include "util.h"
#include "allocator.h"

EXTERN_C bool CJSON_Stack_init(struct CJSON_Stack *const stack, const unsigned capacity) {
    assert(stack != NULL);

    stack->count    = 0U;
    stack->capacity = capacity;
    stack->data     = NULL;

    if(capacity == 0U) {
        stack->data = NULL;
        return true;
    }

    if(!CJSON_Stack_reserve(stack, capacity)) {
        stack->capacity = 0U;
        return false;
    }
    
    return true;
}

EXTERN_C void CJSON_Stack_free(struct CJSON_Stack *const stack) {
    assert(stack != NULL);

    CJSON_FREE(stack->data);
    stack->data     = NULL;
    stack->capacity = 0U;
    stack->count    = 0U;
}

EXTERN_C bool CJSON_Stack_reserve(struct CJSON_Stack *const stack, unsigned capacity) {
    assert(stack != NULL);

    if(capacity <= stack->capacity) {
        return true;
    }

    void **const data = (void**)CJSON_REALLOC(stack->data, (size_t)capacity * sizeof(*data));
    if(data == NULL) {
        return false;
    }
    stack->data     = data;
    stack->capacity = capacity;

    return true;
}

EXTERN_C void *CJSON_Stack_peek(const struct CJSON_Stack *const stack, bool *const success) {
    assert(stack != NULL);
    assert(success != NULL);

    if(stack->count == 0U) {
        *success = false;
        return NULL;
    }

    *success = true;
    return stack->data[stack->count - 1U];
}

EXTERN_C bool CJSON_Stack_push(struct CJSON_Stack *const stack, void *const datum) {
    assert(stack != NULL);
    
    if(stack->count == stack->capacity) {
        if(!CJSON_Stack_reserve(stack, MAX(CJSON_STACK_MINIMUM_CAPACITY, stack->capacity * 2U))) {
            return false;
        }
    }

    stack->data[stack->count++] = datum;

    return true;
}

EXTERN_C void *CJSON_Stack_unsafe_peek(const struct CJSON_Stack *const stack) {
    assert(stack != NULL);

    return stack->data[stack->count - 1U];
}

EXTERN_C void CJSON_Stack_unsafe_push(struct CJSON_Stack *const stack, void *const datum) {
    assert(stack != NULL);
    
    stack->data[stack->count++] = datum;
}

EXTERN_C void *CJSON_Stack_pop(struct CJSON_Stack *const stack, bool *const success) {
    assert(stack != NULL);
    assert(success != NULL);

    if(stack->count == 0U) {
        *success = false;
        return NULL;
    }
    
    *success = true;
    return stack->data[--stack->count];
}

EXTERN_C void *CJSON_Stack_unsafe_pop(struct CJSON_Stack *const stack) {
    assert(stack != NULL);

    return stack->data[--stack->count];
}
