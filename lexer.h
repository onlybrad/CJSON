#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_LEXER_H
#define CJSON_LEXER_H

#include <stdbool.h>
#include "token.h"
#include "tokens.h"
#include "stack.h"

enum CJSON_Lexer_Error {
    CJSON_LEXER_ERROR_NONE,
    CJSON_LEXER_ERROR_TOKEN,
    CJSON_LEXER_ERROR_MEMORY,
    CJSON_LEXER_ERROR_DONE
};

struct CJSON_Lexer {
    struct CJSON_Stack stack;
    const char        *data;
    unsigned           length,
                       position;
};

void CJSON_Lexer_init(struct CJSON_Lexer*, const char *data, unsigned length);
void CJSON_Lexer_free(struct CJSON_Lexer*);
enum CJSON_Lexer_Error CJSON_Lexer_tokenize(struct CJSON_Lexer*, struct CJSON_Tokens*, struct CJSON_Token*);

#endif

#ifdef __cplusplus
}
#endif