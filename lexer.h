#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_LEXER_H
#define CJSON_LEXER_H

#include <stdbool.h>
#include "token.h"
#include "tokens.h"
#include "stack.h"

struct CJSON_Lexer {
    struct CJSON_Stack stack;
    const char        *data;
    unsigned           length,
                       position;
};

void CJSON_Lexer_init(struct CJSON_Lexer*, const char *data, unsigned length);
void CJSON_Lexer_free(struct CJSON_Lexer*);
bool CJSON_Lexer_tokenize(struct CJSON_Lexer*, struct CJSON_Tokens*, struct CJSON_Token*);
bool CJSON_Lexer_finalize(struct CJSON_Lexer*, struct CJSON_Tokens*);

#endif

#ifdef __cplusplus
}
#endif