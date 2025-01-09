#include <string.h>
#include <assert.h>
#include "lexer.h"
#include "token.h"
#include "util.h"

static void skip_whitespace(JSON_Lexer *const lexer) {
    unsigned int position = lexer->position; 
    const unsigned int length = lexer->length;
    const char *const data = lexer->data;

    while(position < length && is_whitespace(data[position])
    ) {
        position++;
    }

    lexer->position = position;
}

static bool read_string(JSON_Lexer *const lexer, JSON_Token *const token) {
    const unsigned int position = lexer->position + 1; 
    const unsigned int length = lexer->length;
    const char *const data = lexer->data;
    bool escaping = false;

    unsigned int i;
    for(i = 0U; position + i < length; i++) {
        const char c = data[position + i];
        
        if(c == '\\' && !escaping) {
            escaping = true;
        } else if(escaping) {
            escaping = false;
        } else if(c == '"') {
            token->type = JSON_TOKEN_STRING;
            token->length = i + 2U;
            return true;
        }
    }
    
    token->type = JSON_TOKEN_INVALID;
    token->length = i + 1U;
    return false;
}

static bool read_number(JSON_Lexer *const lexer, JSON_Token *const token) {
    unsigned int position, i;
    unsigned int length = lexer->length;
    const char *data = lexer->data;
    bool success = true;
    bool read_dot = false;
    bool read_e = false;
    bool read_sign = false;

    if(data[lexer->position] == '-') {
        position = lexer->position + 1;
        token->length = 1;
    } else {
        position = lexer->position;
        token->length = 0;
    }

    data += position;
    length -= position;
    token->type = JSON_TOKEN_INT;

    //0 as the first character is only allowed if it's followed by a dot or by an non-digit character
    if(data[0] == '0' && length > 1 && data[1] != '.' && is_digit(data[1])) {
        success = false;
        token->type = JSON_TOKEN_INVALID;

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
            token->type = JSON_TOKEN_FLOAT;
            if(!read_dot) {
                read_dot = true;
            } else {
                token->type = JSON_TOKEN_INVALID;
                success = false;
            }
            break;
        }

        case 'e':
        case 'E': {
            if(!read_e) {
                read_e = true;
                token->type = JSON_TOKEN_SCIENTIFIC_INT;
            } else {
                token->type = JSON_TOKEN_INVALID;
                success = false;
            }
            break;
        }

        case '+':
        case '-': {
            if(read_e && !read_sign) {
                read_sign = true;
            } else {
                token->type = JSON_TOKEN_INVALID;
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
            token->type = JSON_TOKEN_INVALID;
            success = false;
        }
        }
    }

    token->length += i;

    return success;
}

static bool is_keyword(const JSON_Lexer *const lexer, const char *const keyword, const unsigned int keyword_length) {
    const unsigned int position = lexer->position; 
    const unsigned int lexer_length = lexer->length;
    const char *const data = lexer->data;

    if(position + keyword_length - 1 < lexer_length 
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

static bool read_keyword(JSON_Lexer *const lexer, JSON_Token *const token) {
    static const char null_string[] = "null";
    static const char true_string[] = "true";
    static const char false_string[] = "false";

    
    if(is_keyword(lexer, null_string, static_strlen(null_string))) {
        token->type = JSON_TOKEN_NULL;
        token->length = (unsigned int)static_strlen(null_string);
        return true;
    }
    
    if(is_keyword(lexer, true_string, static_strlen(true_string))) {
        token->type = JSON_TOKEN_BOOL;
        token->length = (unsigned int)static_strlen(true_string);
        return true;
    }
    
    if(is_keyword(lexer, false_string, static_strlen(false_string))) {
        token->type = JSON_TOKEN_BOOL;
        token->length = (unsigned int)static_strlen(false_string);
        return true;
    }

    return false;
}

static void read_invalid_token(JSON_Lexer *const lexer, JSON_Token *const token) {
    const unsigned int position = lexer->position; 
    const unsigned int length = lexer->length;
    const char *const data = lexer->data;

    unsigned int i;
    for(i = position; i < length; i++) {
        if(is_whitespace(data[i]) || is_delimiter(data[i])) {
            i++;
            break;
        };
    }
    
    token->type = JSON_TOKEN_INVALID;
    token->length = i - position - 1;
}

void JSON_Lexer_init(JSON_Lexer *const lexer, const char *const data, const unsigned int length) {
    assert(lexer != NULL);
    assert(data != NULL);
    assert(length > 0);

    *lexer = (JSON_Lexer) {
        .data = data,
        .length = length
    };
}

bool JSON_Lexer_tokenize(JSON_Lexer *const lexer, JSON_Token *const token) {
    assert(lexer != NULL);
    assert(token != NULL);

    skip_whitespace(lexer);

    if(lexer->position == lexer->length) {
        return false;
    }
    
    token->value = lexer->data + lexer->position;
    
    switch(*token->value) {
    case '{':
        token->length = 1;
        token->type = JSON_TOKEN_LCURLY;
        break;
    case '}':
        token->length = 1;
        token->type = JSON_TOKEN_RCURLY;
        break;
    case '[':
        token->length = 1;
        token->type = JSON_TOKEN_LBRACKET;
        break;
    case ']':
        token->length = 1;
        token->type = JSON_TOKEN_RBRACKET;
        break;
    case ':':
        token->length = 1;
        token->type = JSON_TOKEN_COLON;
        break;
    case ',':
        token->length = 1;
        token->type = JSON_TOKEN_COMMA;
        break;
    case '"': {
        if(!read_string(lexer, token)) {
            return false;
        }
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
        break;
    }
    default: {
        if(!read_keyword(lexer, token)) {
            read_invalid_token(lexer, token);
            return false;
        }
        break;
    }
    }
    
    lexer->position += token->length;
    
    return true;
}
