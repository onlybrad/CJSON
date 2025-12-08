#ifndef CJSON_Tokens_H
#define CJSON_Tokens_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "token.h"

struct CJSON_Tokens {
    struct CJSON_Token *data;
    unsigned            index;
    unsigned            count;
    unsigned            capacity;
};

bool CJSON_Tokens_init(struct CJSON_Tokens *tokens, unsigned capacity);
void CJSON_Tokens_free(struct CJSON_Tokens *tokens);
struct CJSON_Token *CJSON_Tokens_next(struct CJSON_Tokens *tokens);

#ifdef __cplusplus
}
#endif

#endif