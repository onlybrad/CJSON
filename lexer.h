#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_LEXER_H
#define CJSON_LEXER_H

#include <stdbool.h>
#include "token.h"
#include "tokens.h"

struct CJSON_Lexer {
    const char *data;
    unsigned    length;
    unsigned    position;
};

void CJSON_Lexer_init(struct CJSON_Lexer*, const char *data, unsigned length);
bool CJSON_Lexer_tokenize(struct CJSON_Lexer*, struct CJSON_Tokens*, struct CJSON_Token*);

#endif

#ifdef __cplusplus
}
#endif