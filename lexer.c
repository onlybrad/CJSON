#include <string.h>
#include <assert.h>
#include "lexer.h"
#include "token.h"
#include "util.h"

static void skip_whitespace(struct CJSON_Lexer *const lexer) {
    unsigned position = lexer->position; 
    const unsigned length = lexer->length;
    const char *const data = lexer->data;

    while(position < length && is_whitespace(data[position])
    ) {
        position++;
    }

    lexer->position = position;
}

static bool read_string(struct CJSON_Lexer *const lexer, struct CJSON_Token *const token) {
    const unsigned position = lexer->position + 1U; 
    const unsigned length = lexer->length;
    const char *const data = lexer->data;
    bool escaping = false;

    unsigned i;
    for(i = 0U; position + i < length; i++) {
        const char c = data[position + i];
        
        if(c == '\\' && !escaping) {
            escaping = true;
        } else if(escaping) {
            escaping = false;
        } else if(c == '"') {
            token->type = CJSON_TOKEN_STRING;
            token->length = i + 2U;
            return true;
        }
    }
    
    token->type = CJSON_TOKEN_INVALID;
    token->length = i + 1U;
    return false;
}

static bool read_number(struct CJSON_Lexer *const lexer, struct CJSON_Token *const token) {
    unsigned position, i, length;
    length = lexer->length;
    const char *data = lexer->data;
    bool success = true;
    bool read_dot = false;
    bool read_e = false;
    bool read_sign = false;

    if(data[lexer->position] == '-') {
        position = lexer->position + 1U;
        token->length = 1U;
    } else {
        position = lexer->position;
        token->length = 0U;
    }

    data += position;
    length -= position;
    token->type = CJSON_TOKEN_INT;

    //0 as the first character is only allowed if it's followed by a dot or by an non-digit character
    if(data[0] == '0' && length > 1U && data[1] != '.' && is_digit(data[1])) {
        success = false;
        token->type = CJSON_TOKEN_INVALID;

        for(i = 0U; i < length; i++) {
            const char c = data[i];

            if(is_whitespace(c) || is_delimiter(c)) {
                break;
            }
        }
    } 
    
    else for(i = 0U; i < length; i++) {
        const char c = data[i];

        if(is_whitespace(c) || is_delimiter(c)) {
            break;
        }
        
        switch(c) {
        case '.': {
            token->type = CJSON_TOKEN_FLOAT;
            if(!read_dot) {
                read_dot = true;
            } else {
                token->type = CJSON_TOKEN_INVALID;
                success = false;
            }
            break;
        }

        case 'e':
        case 'E': {
            if(!read_e) {
                read_e = true;
                token->type = CJSON_TOKEN_SCIENTIFIC_INT;
            } else {
                token->type = CJSON_TOKEN_INVALID;
                success = false;
            }
            break;
        }

        case '+':
        case '-': {
            if(read_e && !read_sign) {
                read_sign = true;
            } else {
                token->type = CJSON_TOKEN_INVALID;
                success = false;                
            }
            break;
        }

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            continue;

        default: {
            token->type = CJSON_TOKEN_INVALID;
            success = false;
        }
        }
    }

    token->length += i;

    return success;
}

static bool next_token_is_keyword(const struct CJSON_Lexer *const lexer, const char *const keyword, const unsigned keyword_length) {
    const unsigned position = lexer->position; 
    const unsigned lexer_length = lexer->length;
    const char *const data = lexer->data;

    if(position + keyword_length - 1U < lexer_length 
    && strncmp(data + position, keyword, keyword_length) == 0
    ){
        if(position + keyword_length >= lexer_length) {
            return true;
        }

        const char next_char = data[position + keyword_length];

        return is_whitespace(next_char) || is_delimiter(next_char);
    }

    return false;
}

static bool read_keyword(struct CJSON_Lexer *const lexer, struct CJSON_Token *const token) {
    static const char null_string[] = "null";
    static const char true_string[] = "true";
    static const char false_string[] = "false";

    
    if(next_token_is_keyword(lexer, null_string, (unsigned)static_strlen(null_string))) {
        token->type = CJSON_TOKEN_NULL;
        token->length = (unsigned)static_strlen(null_string);
        return true;
    }
    
    if(next_token_is_keyword(lexer, true_string, (unsigned)static_strlen(true_string))) {
        token->type = CJSON_TOKEN_BOOL;
        token->length = (unsigned)static_strlen(true_string);
        return true;
    }
    
    if(next_token_is_keyword(lexer, false_string, (unsigned)static_strlen(false_string))) {
        token->type = CJSON_TOKEN_BOOL;
        token->length = (unsigned)static_strlen(false_string);
        return true;
    }

    return false;
}

static void read_invalid_token(struct CJSON_Lexer *const lexer, struct CJSON_Token *const token) {
    const unsigned position = lexer->position; 
    const unsigned length = lexer->length;
    const char *const data = lexer->data;

    unsigned i;
    for(i = position; i < length; i++) {
        if(is_whitespace(data[i]) || is_delimiter(data[i])) {
            i++;
            break;
        };
    }
    
    token->type = CJSON_TOKEN_INVALID;
    token->length = i - position - 1U;
}

EXTERN_C void CJSON_Lexer_init(struct CJSON_Lexer *const lexer, const char *const data, const unsigned length) {
    assert(lexer != NULL);
    assert(data != NULL);
    assert(length > 0U);

    lexer->data     = data;
    lexer->length   = length;
    lexer->position = 0U;
}

EXTERN_C bool CJSON_Lexer_tokenize(struct CJSON_Lexer *const lexer, struct CJSON_Tokens *const tokens,  struct CJSON_Token *const token) {
    assert(lexer != NULL);
    assert(token != NULL);

    skip_whitespace(lexer);

    if(lexer->position == lexer->length) {
        token->type = CJSON_TOKEN_DONE;
        token->length = 0U;
        return false;
    }
    
    token->value = lexer->data + lexer->position;
    
    switch(*token->value) {
    case '{':
        token->length = 1U;
        token->type = CJSON_TOKEN_LCURLY;
        break;
    case '}':
        token->length = 1U;
        token->type = CJSON_TOKEN_RCURLY;
        tokens->counter.object++;
        break;
    case '[':
        token->length = 1U;
        token->type = CJSON_TOKEN_LBRACKET;
        break;
    case ']':
        token->length = 1U;
        token->type = CJSON_TOKEN_RBRACKET;
        tokens->counter.array++;
        break;
    case ':':
        token->length = 1U;
        token->type = CJSON_TOKEN_COLON;
        break;
    case ',':
        token->length = 1U;
        token->type = CJSON_TOKEN_COMMA;
        break;
    case '"': {
        if(!read_string(lexer, token)) {
            return false;
        }
        tokens->counter.string++;
        break;
    }
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        if(!read_number(lexer, token)) {
            return false;
        }
        tokens->counter.number++;
        break;
    }
    default: {
        if(!read_keyword(lexer, token)) {
            read_invalid_token(lexer, token);
            return false;
        }
        tokens->counter.keyword++;
        break;
    }
    }
    
    lexer->position += token->length;
    
    return true;
}
