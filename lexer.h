#ifndef CJSON_LEXER_H
#define CJSON_LEXER_H

#include <stdbool.h>
#include "token.h"

typedef struct CJSON_Lexer {
    const char *data;
    unsigned int length;
    unsigned int position;
} CJSON_Lexer;

void CJSON_Lexer_init(CJSON_Lexer *const lexer, const char *const data, const unsigned int length);
bool CJSON_Lexer_tokenize(CJSON_Lexer *const lexer, CJSON_Token *const token);

#endif
