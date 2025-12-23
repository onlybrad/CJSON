#include <string.h>
#include <assert.h>
#include "lexer.h"
#include "token.h"
#include "util.h"
#include "array.h"
#include "object.h"

static void CJSON_Lexer_skip_whitespace(struct CJSON_Lexer *const lexer) {
    assert(lexer != NULL);

    unsigned          position = lexer->position; 
    const unsigned    length   = lexer->length;
    const char *const data     = lexer->data;

    while(position < length && is_whitespace(data[position])
    ) {
        position++;
    }

    lexer->position = position;
}

static bool CJSON_Lexer_read_string(struct CJSON_Lexer *const lexer, struct CJSON_Token *const token) {
    assert(lexer != NULL);
    assert(token != NULL);

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

static bool CJSON_Lexer_read_number(struct CJSON_Lexer *const lexer, struct CJSON_Token *const token) {
    assert(lexer != NULL);
    assert(token != NULL);

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
    assert(lexer != NULL);
    assert(keyword != NULL);
    assert(keyword_length > 0U);

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

static bool CJSON_Lexer_read_keyword(struct CJSON_Lexer *const lexer, struct CJSON_Token *const token) {
    assert(lexer != NULL);
    assert(token != NULL);

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

static void CJSON_Lexer_read_invalid_token(struct CJSON_Lexer *const lexer, struct CJSON_Token *const token) {
    assert(lexer != NULL);
    assert(token != NULL);

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

static bool CJSON_Lexer_count_containers_elements(struct CJSON_Lexer *const lexer, struct CJSON_Tokens *const tokens) {
    assert(lexer != NULL);
    assert(tokens != NULL);

    struct CJSON_Stack stack;
    CJSON_Stack_init(&stack);
    if(!CJSON_Stack_reserve(&stack, tokens->counter.object + tokens->counter.array)) {
        return false;
    }

    bool success;
    const struct CJSON_Token *const last_token = tokens->data + tokens->count - 1;
    for(struct CJSON_Token *token = tokens->data, *container;
        token != last_token + 1; 
        token++
    ) {
        switch(token->type) {
        case CJSON_TOKEN_LCURLY:
        case CJSON_TOKEN_LBRACKET:
            CJSON_Stack_unsafe_push(&stack, token);
            continue;
            
        case CJSON_TOKEN_RCURLY:
        case CJSON_TOKEN_RBRACKET:
            container = (struct CJSON_Token*)CJSON_Stack_pop(&stack, &success);
            if(!success) {
                CJSON_Stack_free(&stack);
                return false;
            }

            assert(container->type == CJSON_TOKEN_LCURLY || container->type == CJSON_TOKEN_LBRACKET);
            if(container->type == CJSON_TOKEN_LCURLY) {
                tokens->counter.object_elements += MAX(container->length, CJSON_OBJECT_MINIMUM_CAPACITY);
            } else {
                tokens->counter.array_elements += MAX(container->length, CJSON_ARRAY_MINIMUM_CAPACITY);
            }

            continue;

        case CJSON_TOKEN_COMMA:
            container = (struct CJSON_Token*)CJSON_Stack_peek(&stack, &success);
            if(!success) {
                CJSON_Stack_free(&stack);
                return false;
            }

            assert(container->type == CJSON_TOKEN_LCURLY || container->type == CJSON_TOKEN_LBRACKET);
            container->length++;
            
            continue;

        default:
            continue;
        }
    }

    CJSON_Stack_free(&stack);
    return true;
}

EXTERN_C void CJSON_Lexer_init(struct CJSON_Lexer *const lexer, const char *const data, const unsigned length) {
    assert(lexer != NULL);
    assert(data != NULL);
    assert(length > 0U);

    lexer->data     = data;
    lexer->length   = length;
    lexer->position = 0U;
}

EXTERN_C enum CJSON_Lexer_Error CJSON_Lexer_tokenize(struct CJSON_Lexer *const lexer, struct CJSON_Tokens *const tokens, struct CJSON_Token *const token) {
    assert(lexer != NULL);
    assert(token != NULL);

    CJSON_Lexer_skip_whitespace(lexer);

    if(lexer->position == lexer->length) {
        token->type = CJSON_TOKEN_DONE;
        token->length = 0U;
        return CJSON_Lexer_count_containers_elements(lexer, tokens) 
            ? CJSON_LEXER_ERROR_DONE
            : CJSON_LEXER_ERROR_MEMORY;
    }
    
    token->value = lexer->data + lexer->position;
    
    switch(*token->value) {
    case '{':
        token->length = 1U;
        token->type   = CJSON_TOKEN_LCURLY;
        break;
    case '}':
        token->length = 1U;
        token->type   = CJSON_TOKEN_RCURLY;
        tokens->counter.object++;
        break;
    case '[':
        token->length = 1U;
        token->type   = CJSON_TOKEN_LBRACKET;
        break;
    case ']':
        token->length = 1U;
        token->type   = CJSON_TOKEN_RBRACKET;
        tokens->counter.array++;
        break;
    case ':':
        token->length = 1U;
        token->type   = CJSON_TOKEN_COLON;
        break;
    case ',': {
        token->length = 1U;
        token->type   = CJSON_TOKEN_COMMA;
        tokens->counter.comma++;
        break;
    }
    case '"': {
        if(!CJSON_Lexer_read_string(lexer, token)) {
            return CJSON_LEXER_ERROR_TOKEN;
        }
        assert(token->length >= 2U);
        tokens->counter.string++;
        tokens->counter.chars += token->length - 1U;
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
        if(!CJSON_Lexer_read_number(lexer, token)) {
             return CJSON_LEXER_ERROR_TOKEN;
        }
        assert(token->length >= 1U);
        tokens->counter.number++;
        break;
    }
    default: {
        if(!CJSON_Lexer_read_keyword(lexer, token)) {
            CJSON_Lexer_read_invalid_token(lexer, token);
            return CJSON_LEXER_ERROR_TOKEN;
        }
        assert(token->length >= 4U);
        tokens->counter.keyword++;
        break;
    }
    }
    
    lexer->position += token->length;

    return CJSON_LEXER_ERROR_NONE;
}
