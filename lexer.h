#ifndef JSON_LEXER_H
#define JSON_LEXER_H

#include <stdbool.h>
#include "token.h"

typedef struct JSON_Lexer {
    const char *data;
    unsigned int length;
    unsigned int position;
} JSON_Lexer;

void JSON_Lexer_init(JSON_Lexer *const lexer, const char *const data, const unsigned int length);
bool JSON_Lexer_tokenize(JSON_Lexer *const lexer, JSON_Token *const token);

#endif
