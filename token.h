#ifndef CJSON_TOKEN_H
#define CJSON_TOKEN_H

#include <stdio.h>

typedef enum CJSON_Token_Type {
    CJSON_TOKEN_LCURLY,
    CJSON_TOKEN_RCURLY,
    CJSON_TOKEN_LBRACKET,
    CJSON_TOKEN_RBRACKET,
    CJSON_TOKEN_COLON,
    CJSON_TOKEN_COMMA,
    CJSON_TOKEN_STRING,
    CJSON_TOKEN_INT,
    CJSON_TOKEN_FLOAT,
    CJSON_TOKEN_SCIENTIFIC_INT,
    CJSON_TOKEN_BOOL,
    CJSON_TOKEN_NULL,
    CJSON_TOKEN_INVALID
} CJSON_Token_Type;

typedef struct CJSON_Token {
    const char *value;
    unsigned int length;
    CJSON_Token_Type type;
} CJSON_Token;

#endif
