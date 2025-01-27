#ifndef JSON_LEXER_H
#define JSON_LEXER_H

#include <stdbool.h>
#include "token.h"

typedef struct CJSON_Lexer {
    const char *data;
    unsigned int length;
    unsigned int position;
} CJSON_Lexer;

void CJSON_Lexer_init(CJSON_Lexer *const lexer, const char *const data, const unsigned int length);
bool CJSON_Lexer_tokenize(CJSON_Lexer *const lexer, JSON_Token *const token);

#endif
