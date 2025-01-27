#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "tokens.h"
#include "benchmark.h"

static void JSON_Tokens_resize(CJSON_Tokens *const tokens, const unsigned int capacity) {
    assert(tokens != NULL);
    assert(capacity > tokens->capacity); //new size must be larger than current size

    BENCHMARK_START();

    CJSON_Token *data = CJSON_REALLOC(tokens->data, (size_t)capacity * sizeof(CJSON_Token), (size_t)tokens->capacity * sizeof(CJSON_Token));
    
    assert(data != NULL);

    tokens->data = data;
    tokens->capacity = capacity;

    BENCHMARK_END();
}

void CJSON_Tokens_init(CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    CJSON_Token *data = CJSON_MALLOC(INITIAL_TOKENS_CAPACITY * sizeof(CJSON_Token));
    assert(data != NULL);

    *tokens = (CJSON_Tokens) {
        .data = data,
        .capacity = INITIAL_TOKENS_CAPACITY,
    };
}

inline void CJSON_Tokens_free(CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    CJSON_FREE(tokens->data);
    *tokens = (CJSON_Tokens){0};
}

CJSON_Token *CJSON_Tokens_next(CJSON_Tokens *const tokens) {
    assert(tokens != NULL);

    BENCHMARK_START();

    if(tokens->length == tokens->capacity) {
        JSON_Tokens_resize(tokens, tokens->capacity * 2);
    }

    CJSON_Token *const token = tokens->data + tokens->length;
    tokens->length++;

    BENCHMARK_END();

    return token;
}

