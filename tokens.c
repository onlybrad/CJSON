#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "tokens.h"
#include "allocator.h"
#include "util.h"

EXTERN_C void CJSON_Tokens_init(struct CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    tokens->data                      = NULL;
    tokens->current_token             = NULL;
    tokens->counter.object            = 0U;
    tokens->counter.array             = 0U;
    tokens->counter.number            = 0U;
    tokens->counter.string            = 0U;
    tokens->counter.keyword           = 0U;
    tokens->counter.chars             = 0U;
    tokens->counter.comma             = 0U;
    tokens->counter.object_elements   = 0U;
    tokens->counter.array_elements    = 0U;
    tokens->capacity                  = 0U;
    tokens->count                     = 0U;
}

EXTERN_C bool CJSON_Tokens_reserve(struct CJSON_Tokens *const tokens, unsigned capacity) {
    assert(tokens != NULL);

    if(capacity < CJSON_TOKENS_MINIMUM_CAPACITY) {
        capacity = CJSON_TOKENS_MINIMUM_CAPACITY;
    }

    if(capacity <= tokens->capacity) {
        return true;
    }

    struct CJSON_Token *data = (struct CJSON_Token*)CJSON_REALLOC(tokens->data, (size_t)capacity * sizeof(*data));
    if(data == NULL) {
        return false;
    }

    tokens->capacity      = capacity;
    tokens->data          = data;
    tokens->current_token = data;

    return true;
}

EXTERN_C void CJSON_Tokens_free(struct CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    CJSON_FREE(tokens->data);
    CJSON_Tokens_init(tokens);
}

EXTERN_C struct CJSON_Token *CJSON_Tokens_next(struct CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    if(tokens->count == tokens->capacity) {
        if(!CJSON_Tokens_reserve(tokens, tokens->capacity * 2U)) {
            return NULL;
        }
    }

    struct CJSON_Token *const token = tokens->data + tokens->count;
    tokens->count++;

    return token;
}
