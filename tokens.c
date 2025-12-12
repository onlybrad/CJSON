#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "tokens.h"
#include "allocator.h"
#include "util.h"

static bool JSON_Tokens_resize(struct CJSON_Tokens *const tokens, const unsigned capacity) {
    assert(tokens != NULL);
    
    if(capacity <= tokens->capacity) {
        return true;
    }

    struct CJSON_Token *data = (struct CJSON_Token*)CJSON_REALLOC(tokens->data, (size_t)capacity * sizeof(*data));
    if(data == NULL) {
        return false;
    }

    tokens->data     = data;
    tokens->capacity = capacity;

    return true;
}

EXTERN_C bool CJSON_Tokens_init(struct CJSON_Tokens *const tokens, const unsigned capacity) {
    assert(tokens != NULL);

    tokens->counter.object  = 0U;
    tokens->counter.array   = 0U;
    tokens->counter.number  = 0U;
    tokens->counter.string  = 0U;
    tokens->counter.keyword = 0U;
    tokens->counter.chars   = 0U;
    tokens->counter.comma   = 0U;
    tokens->data            = NULL;
    tokens->capacity        = 0U;
    tokens->count           = 0U;
    tokens->index           = 0U;
    
    return JSON_Tokens_resize(tokens, capacity);
}

EXTERN_C void CJSON_Tokens_free(struct CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    CJSON_FREE(tokens->data);
    CJSON_Tokens_init(tokens, 0U);
}

EXTERN_C struct CJSON_Token *CJSON_Tokens_next(struct CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    if(tokens->count == tokens->capacity) {
        if(!JSON_Tokens_resize(tokens, tokens->capacity * 2U)) {
            return NULL;
        }
    }

    struct CJSON_Token *const token = tokens->data + tokens->count;
    tokens->count++;

    return token;
}
