#ifndef CJSON_LEXER_H
#define CJSON_LEXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "token.h"

struct CJSON_Lexer {
    const char *data;
    unsigned    length;
    unsigned    position;
};

void CJSON_Lexer_init(struct CJSON_Lexer*, const char *data, unsigned length);
bool CJSON_Lexer_tokenize(struct CJSON_Lexer*, struct CJSON_Token*);

#ifdef __cplusplus
}
#endif

#endif
