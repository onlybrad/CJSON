#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_STACK_H
#define CJSON_STACK_H

#define CJSON_STACK_MINIMUM_CAPACITY 8U

#include <stdbool.h>

struct CJSON_Stack {
    void    **data;
    unsigned  count,
              capacity;
};

void  CJSON_Stack_init(struct CJSON_Stack*);
void  CJSON_Stack_free(struct CJSON_Stack*);
bool  CJSON_Stack_reserve(struct CJSON_Stack*, unsigned capacity);
void *CJSON_Stack_peek(const struct CJSON_Stack*, bool *success);
bool  CJSON_Stack_push(struct CJSON_Stack*, void*);
void *CJSON_Stack_pop(struct CJSON_Stack*, bool *success);
void *CJSON_Stack_unsafe_peek(const struct CJSON_Stack*);
void  CJSON_Stack_unsafe_push(struct CJSON_Stack*, void *value);
void *CJSON_Stack_unsafe_pop(struct CJSON_Stack*);

#endif

#ifdef __cplusplus
}
#endif