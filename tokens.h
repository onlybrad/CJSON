#ifndef CJSON_Tokens_H
#define CJSON_Tokens_H

#define INITIAL_TOKENS_CAPACITY (1 << 10)

#include <stdbool.h>
#include "token.h"
#include "allocator.h"

typedef struct CJSON_Tokens {
    CJSON_Token *data;
    unsigned int index;
    unsigned int length;
    unsigned int capacity;
} CJSON_Tokens;

void CJSON_Tokens_init(CJSON_Tokens *const tokens);
void CJSON_Tokens_free(CJSON_Tokens *const tokens);
CJSON_Token *CJSON_Tokens_next(CJSON_Tokens *const tokens);

#endif
