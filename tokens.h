#ifndef JSON_Tokens_H
#define JSON_Tokens_H

#define INITIAL_TOKENS_CAPACITY (1 << 10)

#include <stdbool.h>
#include "token.h"
#include "allocator.h"

typedef struct JSON_Tokens {
    JSON_Token *data;
    unsigned int index;
    unsigned int length;
    unsigned int capacity;
} JSON_Tokens;

void JSON_Tokens_init(JSON_Tokens *const tokens);
void JSON_Tokens_free(JSON_Tokens *const tokens);
JSON_Token *JSON_Tokens_next(JSON_Tokens *const tokens);

#endif
