#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_UNSAFESTACK_H
#define CJSON_UNSAFESTACK_H

#include <stdbool.h>

extern void *const CJSON_STACK_NULL;

struct CJSON_Stack {
    void    **data;
    unsigned  count;
    unsigned  capacity;
};

bool  CJSON_Stack_init(struct CJSON_Stack*, unsigned capacity);
void  CJSON_Stack_free(struct CJSON_Stack*);
bool  CJSON_Stack_reserve(struct CJSON_Stack*, unsigned capacity);
void *CJSON_Stack_peek(const struct CJSON_Stack*);
bool  CJSON_Stack_push(struct CJSON_Stack*, void*);
void *CJSON_Stack_pop(struct CJSON_Stack*);

#endif

#ifdef __cplusplus
}
#endif