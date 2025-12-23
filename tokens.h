#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_Tokens_H
#define CJSON_Tokens_H

#define CJSON_TOKENS_MINIMUM_CAPACITY 8U

#include <stdbool.h>
#include "token.h"

struct CJSON_Tokens {
    struct CJSON_Token *data,
                       *current_token;
    struct CJSON_Token_Stats {
        unsigned string,
                 number,
                 array,
                 object,
                 keyword,
                 comma,
                 chars,
                 array_elements,
                 object_elements;
    } counter;
    unsigned count,
             capacity;
};

void CJSON_Tokens_init(struct CJSON_Tokens *tokens);
bool CJSON_Tokens_reserve(struct CJSON_Tokens *tokens, unsigned capacity);
void CJSON_Tokens_free(struct CJSON_Tokens *tokens);
struct CJSON_Token *CJSON_Tokens_next(struct CJSON_Tokens *tokens);

#endif

#ifdef __cplusplus
}
#endif
