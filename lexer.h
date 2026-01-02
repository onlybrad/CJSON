#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_LEXER_H
#define CJSON_LEXER_H

#include <stdbool.h>
#include "token.h"
#include "tokens.h"
#include "stack.h"

struct CJSON_Parser;

enum CJSON_Lexer_Error {
    CJSON_LEXER_ERROR_NONE,
    CJSON_LEXER_ERROR_TOKEN,
    CJSON_LEXER_ERROR_MEMORY,
    CJSON_LEXER_ERROR_DONE
};

struct CJSON_Lexer {
    const char *data;
    unsigned    length,
                position;
};

void CJSON_Lexer_init(struct CJSON_Lexer*, const char *data, unsigned length);
enum CJSON_Lexer_Error CJSON_Lexer_tokenize(struct CJSON_Lexer*, struct CJSON_Parser*);

#endif

#ifdef __cplusplus
}
#endif