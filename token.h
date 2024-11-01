#ifndef JSON_TOKEN_H
#define JSON_TOKEN_H

#include <stdio.h>

typedef enum JSON_Token_Type {
    JSON_TOKEN_LCURLY,
    JSON_TOKEN_RCURLY,
    JSON_TOKEN_LBRACKET,
    JSON_TOKEN_RBRACKET,
    JSON_TOKEN_COLON,
    JSON_TOKEN_COMMA,
    JSON_TOKEN_STRING,
    JSON_TOKEN_INT,
    JSON_TOKEN_FLOAT,
    JSON_TOKEN_SCIENTIFIC_INT,
    JSON_TOKEN_BOOL,
    JSON_TOKEN_NULL,
    JSON_TOKEN_INVALID
} JSON_Token_Type;

typedef struct JSON_Token {
    const char *value;
    unsigned length;
    JSON_Token_Type type;
} JSON_Token;

#endif