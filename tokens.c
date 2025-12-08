#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "tokens.h"
#include "allocator.h"

static bool JSON_Tokens_resize(struct CJSON_Tokens *const tokens, const unsigned capacity) {
    assert(tokens != NULL);
    assert(capacity > tokens->capacity);

    struct CJSON_Token *data = (struct CJSON_Token*)CJSON_REALLOC(tokens->data, (size_t)capacity * sizeof(*data));
    if(data == NULL) {
        return false;
    }

    tokens->data     = data;
    tokens->capacity = capacity;

    return true;
}

bool CJSON_Tokens_init(struct CJSON_Tokens *const tokens, const unsigned capacity) {
    assert(tokens != NULL);

    tokens->data     = NULL;
    tokens->index    = 0U;
    tokens->count    = 0U;
    tokens->capacity = 0U;

    return JSON_Tokens_resize(tokens, capacity);
}

inline void CJSON_Tokens_free(struct CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    CJSON_FREE(tokens->data);
    memset(tokens, 0, sizeof(*tokens));
}

struct CJSON_Token *CJSON_Tokens_next(struct CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    if(tokens->count == tokens->capacity) {
        if(!JSON_Tokens_resize(tokens, tokens->capacity * 2)) {
            return NULL;
        }
    }

    struct CJSON_Token *const token = tokens->data + tokens->count;
    tokens->count++;

    return token;
}

