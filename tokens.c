#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "tokens.h"
#include "benchmark.h"

static void JSON_Tokens_resize(JSON_Tokens *const tokens, const double multiplier) {
    assert(tokens != NULL);
    assert(multiplier > 1.0); //multiplier must actually increase the size
    assert(multiplier <= UINT_MAX / tokens->capacity); //check overflow

    BENCHMARK_START();

    const unsigned int capacity = (unsigned int)((double)tokens->capacity * multiplier);
    JSON_Token *data = CJSON_REALLOC(tokens->data, (size_t)capacity * sizeof(JSON_Token), tokens->capacity * sizeof(JSON_Token));
    assert(data != NULL);

    tokens->data = data;
    tokens->capacity = capacity;

    BENCHMARK_END();
}

void JSON_Tokens_init(JSON_Tokens *const tokens) {
    assert(tokens != NULL);

    JSON_Token *data = CJSON_MALLOC(INITIAL_TOKENS_CAPACITY * sizeof(JSON_Token));
    assert(data != NULL);

    *tokens = (JSON_Tokens) {
        .data = data,
        .capacity = INITIAL_TOKENS_CAPACITY,
    };
}

inline void JSON_Tokens_free(JSON_Tokens *const tokens) {
    assert(tokens != NULL);

    CJSON_FREE(tokens->data);
    *tokens = (JSON_Tokens){0};
}

JSON_Token *JSON_Tokens_next(JSON_Tokens *const tokens) {
    assert(tokens != NULL);

    BENCHMARK_START();

    if(tokens->length == tokens->capacity) {
        JSON_Tokens_resize(tokens, 2.0);
    }

    JSON_Token *const token = tokens->data + tokens->length;
    tokens->length++;

    BENCHMARK_END();

    return token;
}

