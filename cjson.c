#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cjson.h"
#include "lexer.h"
#include "tokens.h"
#include "allocator.h"
#include "util.h"

#define CJSON_INIT_ARENA_SIZE          1024U
#define CJSON_INIT_ARENA_MAX_NODES     CJSON_ARENA_INFINITE_NODES
#define CJSON_PARSE_TOKENS_SIZE_FACTOR 2U
#define CJSON_PARSE_ARENA_MAX_NODES    CJSON_ARENA_INFINITE_NODES

enum CJSON_ObjectError {
    CJSON_OBJECT_ERROR_KEY,
    CJSON_OBJECT_ERROR_COLON,
    CJSON_OBJECT_ERROR_VALUE,
    CJSON_OBJECT_ERROR_COMMA,
    CJSON_OBJECT_ERROR_INCOMPLETE,
    CJSON_OBJECT_ERROR_MEMORY
};

enum CJSON_ArrayError {
    CJSON_ARRAY_ERROR_VALUE,
    CJSON_ARRAY_ERROR_COMMA,
    CJSON_ARRAY_ERROR_INCOMPLETE,
    CJSON_ARRAY_ERROR_MEMORY
};

static const char *CJSON_ARENA_NAMES[] = {
    "Object Arena",
    "Array Arena",
    "String Arena"
};
 
static bool CJSON_parse_tokens(struct CJSON_Root*, struct CJSON*, struct CJSON_Tokens*);

static bool CJSON_decode_string_token(struct CJSON_String *const string, struct CJSON_Root *const root, const struct CJSON_Token *const token) {
    assert(string != NULL);
    assert(root != NULL);
    assert(token != NULL);

    char *output_current = CJSON_ARENA_ALLOC(&root->string_arena, token->length - 1U, char);
    if(output_current == NULL) {
        return false;
    }
    char *const output_start    = output_current;
    const char *const input_end = token->value + token->length - 2;
    const char *input_current   = token->value + 1;
    bool escaping               = false;

    while(input_current != input_end + 1) {
        switch(*input_current) {
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                return false;
        }

        if(!escaping) {
            if(*input_current == '\\') {
                escaping = true;
                input_current++;
                continue;
            }

            *(output_current++) = *(input_current++);
            continue;
        } 
        
        else switch(*input_current) {
        case '"':
            escaping            = false;
            *(output_current++) = '"';
            input_current++;
            continue;
        case 'b':
            escaping            = false;
            *(output_current++) = '\b';
            input_current++;
            continue;
        case 'f':
            escaping            = false;
            *(output_current++) = '\f';
            input_current++;
            continue;
        case 'n':
            escaping            = false;
            *(output_current++) = '\n';
            input_current++;
            continue;
        case 'r':
            escaping            = false;
            *(output_current++) = '\r';
            input_current++;
            continue;
        case 't':
            escaping            = false;
            *(output_current++) = '\t';
            input_current++;
            continue;
        case '/':
            escaping            = false;
            *(output_current++) = '/';
            input_current++;
            continue;
        case '\\': {
            escaping            = false;
            *(output_current++) = '\\';
            input_current++;
            continue;
        }
        case 'u': {
            if(input_end - input_current < 4) {
                return false;
            }

            input_current++;

            bool success;
            const uint16_t high = hex_to_utf16(input_current, &success);
            //\u0000 is apparently an acceptable escaped character in JSON so if the parsing function returns 0 then it's either an error or \u0000. \0 would break cstrings so it's gonna be treated as an error as well.
            if(high == 0U) {
                return false;    
            }

            if(IS_VALID_2_BYTES_UTF16(high)) {
                output_current += utf16_to_utf8_2bytes(output_current, high);
                input_current  += 4;
                escaping        = false;
                continue;
            }

            if(input_end - input_current < 9 || input_current[4] != '\\' || input_current[5] != 'u') {
                return false;
            }

            const uint16_t low = hex_to_utf16(input_current + 6, &success);
            if(!success || !IS_VALID_4_BYTES_UTF16(high, low)) {
                return false; 
            }

            utf16_to_utf8_4bytes(output_current, high, low);
            output_current += 4;
            input_current  += 10;
            escaping        = false;
            continue;
        }
        default:
            return false;        
    }
    }

    if(escaping) {
        return false;
    }

    const unsigned length  = (unsigned)(output_current - output_start);
    output_current[length] = '\0';
    string->chars          = output_start;
    string->length         = length;

    return true;
}

static bool CJSON_parse_string(struct CJSON *const json, struct CJSON_Root *const root, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(root != NULL);
    assert(tokens != NULL);
    
    const struct CJSON_Token *const token = tokens->data + tokens->index; 

    if(!CJSON_decode_string_token(&json->data.string, root, token)) {
        json->type       = CJSON_ERROR;
        json->data.error = CJSON_ERROR_STRING;

        return false;
    }

    json->type = CJSON_STRING;

    tokens->index++;

    return true;
}

static bool CJSON_parse_number(struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    char str[1 << 9] = {0};

    bool success;
    const struct CJSON_Token *const token = tokens->data + tokens->index; 

    memcpy(str, token->value, MIN(sizeof(str) - 1, token->length));

    if(token->type == CJSON_TOKEN_FLOAT) {
        const double value = parse_float64(str, &success);
        if(!success) {
            json->type = CJSON_ERROR;
            json->data.error = CJSON_ERROR_FLOAT64;

            return false;
        }

        json->type          = CJSON_FLOAT64;
        json->data.float64 = value;
    } else if(str[0] == '-') {
        if(token->type == CJSON_TOKEN_SCIENTIFIC_INT) {
            const long double value = parse_long_double(str, &success);
            if(!success || value < INT64_MIN || value > INT64_MAX) {
                json->type        = CJSON_ERROR;
                json->data.error = CJSON_ERROR_INT64;

                return false;
            }

            json->type        = CJSON_INT64;
            json->data.int64 = (int64_t)value;

        } else {
            const int64_t value = parse_int64(str, &success);
            if(!success) {
                json->type        = CJSON_ERROR;
                json->data.error = CJSON_ERROR_INT64;

                return false;     
            }

            json->type        = CJSON_INT64;
            json->data.int64 = value;
        }
    } else if(token->type == CJSON_TOKEN_SCIENTIFIC_INT) {
        const long double value = parse_long_double(str, &success);
        if(!success || value > UINT64_MAX) {
            json->type        = CJSON_ERROR;
            json->data.error = CJSON_ERROR_UINT64;

            return false;
        }

        json->type = CJSON_UINT64;
        json->data.uint64 = (uint64_t)value;
    } else {
        const uint64_t value = parse_uint64(str, &success);
        if(!success) {
            json->type        = CJSON_ERROR;
            json->data.error = CJSON_ERROR_UINT64;

            return false; 
        }

        json->type = CJSON_UINT64;
        json->data.uint64 = value;
    }

    tokens->index++;
    
    return true;
}

static void CJSON_parse_bool(struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    const struct CJSON_Token *const token = tokens->data + tokens->index; 

    json->type = CJSON_BOOL;
    json->data.boolean = token->value[0] == 't';

    tokens->index++;
}

static void CJSON_parse_null(struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    json->type = CJSON_NULL;
    json->data.null = NULL;

    tokens->index++;
}

static bool CJSON_parse_object(struct CJSON *const json, struct CJSON_Root *const root, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(root != NULL);
    assert(tokens != NULL);

    tokens->index++;

    if(tokens->count == tokens->index) {
        json->type       = CJSON_ERROR;
        json->data.error = CJSON_ERROR_OBJECT;

        return false;
    }

    enum CJSON_Error           error;
    enum CJSON_ObjectError     object_error;
    const struct CJSON_Token  *token   = tokens->data + tokens->index;
    struct CJSON_Object *const object  = &json->data.object;
    json->type = CJSON_OBJECT;
    
    if(!CJSON_Object_init(object, root, 8U)) {
        object_error = CJSON_OBJECT_ERROR_MEMORY;
        goto end;
    }

    if(token->type == CJSON_TOKEN_RCURLY) {
        tokens->index++;
        return true;
    }

    while(tokens->index + 4U < tokens->count) {
        if(token->type != CJSON_TOKEN_STRING) {
            object_error = CJSON_OBJECT_ERROR_KEY;
            goto end;
        }

        struct CJSON_String key;
        if(!CJSON_decode_string_token(&key, root, token)) {
            object_error = CJSON_OBJECT_ERROR_KEY;
            goto end;
        }

        token++;
    
        if(token->type != CJSON_TOKEN_COLON) {
            object_error = CJSON_OBJECT_ERROR_COLON;
            tokens->index++;
            goto end;
        }

        tokens->index += 2U;

        struct CJSON_KV *const entry = CJSON_Object_get_entry(object, root, key.chars);
        if(entry == NULL) {
            object_error = CJSON_OBJECT_ERROR_MEMORY;
            goto end;
        }

        if(entry->key == NULL) {
            entry->key = key.chars;
        }

        if(!CJSON_parse_tokens(root, &entry->value, tokens)) {
            object_error = CJSON_OBJECT_ERROR_VALUE;
            error        = entry->value.data.error;
            goto end;
        }

        token = tokens->data + tokens->index;

        if(token->type == CJSON_TOKEN_COMMA) {
            tokens->index++;
            token++;
            continue;
        }

        if(token->type == CJSON_TOKEN_RCURLY) {
            tokens->index++;
            return true;
        }

        object_error = CJSON_OBJECT_ERROR_COMMA;
        goto end;
    }
    
    object_error = CJSON_OBJECT_ERROR_INCOMPLETE;

end:
    json->type = CJSON_ERROR;

    switch(object_error) {
    case CJSON_OBJECT_ERROR_INCOMPLETE: {
        json->data.error = CJSON_ERROR_OBJECT;
        break;
    }
    case CJSON_OBJECT_ERROR_KEY: {
        json->data.error = CJSON_ERROR_OBJECT_KEY;
        break;
    }

    case CJSON_OBJECT_ERROR_COLON: {
        json->data.error = CJSON_ERROR_MISSING_COLON;
        break;
    }

    case CJSON_OBJECT_ERROR_VALUE: {
        if(error == CJSON_ERROR_TOKEN) {
            json->data.error = CJSON_ERROR_OBJECT_VALUE;
        } else {
            json->data.error = error;
        }
        break;
    }

    case CJSON_OBJECT_ERROR_COMMA: {
        json->data.error = CJSON_ERROR_MISSING_COMMA_OR_RCURLY;
        break;
    }

    case CJSON_OBJECT_ERROR_MEMORY: {
        json->data.error = CJSON_ERROR_MEMORY;
        break;
    }
    }

    return false;
}

static bool CJSON_parse_array(struct CJSON *const json, struct CJSON_Root *const root, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(root != NULL);
    assert(tokens != NULL);

    tokens->index++;

    if(tokens->count == tokens->index) {
        json->type       = CJSON_ERROR;
        json->data.error = CJSON_ERROR_ARRAY;
        return false;
    }

    const struct CJSON_Token *token;

    enum CJSON_Error          error;
    enum CJSON_ArrayError     array_error;
    struct CJSON_Array *const array = &json->data.array;
    json->type = CJSON_ARRAY;
    if(!CJSON_Array_init(array, root)) {
        array_error = CJSON_ARRAY_ERROR_MEMORY;
        goto end;
    }

    token = tokens->data + tokens->index;
    if(token->type == CJSON_TOKEN_RBRACKET) {
        tokens->index++;
        return true;
    }

    while(tokens->index + 2U < tokens->count) {
        struct CJSON *const next_json = CJSON_Array_next(array, root);
        if(next_json == NULL) {
            array_error = CJSON_ARRAY_ERROR_MEMORY;
            goto end;
        }

        if(!CJSON_parse_tokens(root, next_json, tokens)) {
            array_error = CJSON_ARRAY_ERROR_VALUE;
            error       = next_json->data.error;
            goto end;
        }

        token = tokens->data + tokens->index;
 
        if(token->type == CJSON_TOKEN_COMMA) {
            tokens->index++;
            token++;
            continue;
        }

        if(token->type == CJSON_TOKEN_RBRACKET) {
            tokens->index++;
            return true;
        }

        array_error = CJSON_ARRAY_ERROR_COMMA;
        goto end;
    }

    array_error = CJSON_ARRAY_ERROR_INCOMPLETE;

end:
    json->type = CJSON_ERROR;
    
    switch(array_error) {
    case CJSON_ARRAY_ERROR_INCOMPLETE: {
        json->data.error = CJSON_ERROR_ARRAY;
        break;
    }

    case CJSON_ARRAY_ERROR_VALUE: {
        if(error == CJSON_ERROR_TOKEN) {
            json->data.error = CJSON_ERROR_ARRAY_VALUE;
        } else {
            json->data.error = error;
        }
        break;
    }

    case CJSON_ARRAY_ERROR_COMMA: {
        json->data.error = CJSON_ERROR_MISSING_COMMA_OR_RCURLY;
        break;
    }

    case CJSON_ARRAY_ERROR_MEMORY: {
        json->data.error = CJSON_ERROR_MEMORY;
        break;
    }
    }

    return false;
}

static bool CJSON_parse_tokens(struct CJSON_Root *const root, struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(root != NULL);
    assert(tokens != NULL);

    const struct CJSON_Token *const token = tokens->data + tokens->index; 

    switch(token->type) {
    case CJSON_TOKEN_STRING: {
        return CJSON_parse_string(json, root, tokens);
    }
    case CJSON_TOKEN_INT:
    case CJSON_TOKEN_FLOAT:
    case CJSON_TOKEN_SCIENTIFIC_INT: {
        return CJSON_parse_number(json, tokens);
    }
    case CJSON_TOKEN_BOOL: {
        CJSON_parse_bool(json, tokens);
        return true;
    }
    case CJSON_TOKEN_NULL: {
        CJSON_parse_null(json, tokens);
        return true;
    }
    case CJSON_TOKEN_LBRACKET: {
        return CJSON_parse_array(json, root, tokens);
    }
    case CJSON_TOKEN_LCURLY: {
        return CJSON_parse_object(json, root, tokens);
    }
    case CJSON_TOKEN_COLON:
    case CJSON_TOKEN_COMMA:
    case CJSON_TOKEN_RBRACKET:
    case CJSON_TOKEN_RCURLY:
    case CJSON_TOKEN_INVALID:
    case CJSON_TOKEN_DONE:
        json->type       = CJSON_ERROR;
        json->data.error = CJSON_ERROR_TOKEN;
    }

    return false;
}

static bool CJSON_init_arenas(struct CJSON_Root *const root, unsigned size, const unsigned max_nodes) {
    assert(root != NULL);

    if(size < CJSON_INIT_ARENA_SIZE) {
        size = CJSON_INIT_ARENA_SIZE;
    }

    struct CJSON_Arena *arenas[] = {
        &root->object_arena,
        &root->array_arena,
        &root->string_arena
    };
        
    for(int i = 0; i < (int)(sizeof(arenas)/sizeof(*arenas)); i++) {
        if(!CJSON_Arena_init(arenas[i], size, max_nodes, CJSON_ARENA_NAMES[i])) {
            return false;
        }
    }

    return true;
}

EXTERN_C bool CJSON_init(struct CJSON_Root *const root) {
    assert(root != NULL);

    root->json.type      = CJSON_NULL;
    root->json.data.null = NULL;
    root->string_length  = 0U;

    CJSON_Tokens_init(&root->tokens, 0U);

    return CJSON_init_arenas(root, CJSON_INIT_ARENA_SIZE, CJSON_INIT_ARENA_MAX_NODES);
}

EXTERN_C struct CJSON_Array *CJSON_make_array(struct CJSON *const json, struct CJSON_Root *const root) {
    assert(json != NULL);
    assert(root != NULL);

    if(!CJSON_Array_init(&json->data.array, root)) {
        return NULL;
    }
    json->type = CJSON_ARRAY;

    return &json->data.array;
}

EXTERN_C struct CJSON_Object *CJSON_make_object(struct CJSON *const json, struct CJSON_Root *const root) {
    assert(json != NULL);

    if(!CJSON_Object_init(&json->data.object, root, 0U)) {
        return NULL;
    }
    json->type = CJSON_OBJECT;

    return &json->data.object;
}

EXTERN_C bool CJSON_parse(struct CJSON_Root *const root, const char *const data, const unsigned length) {
    assert(root != NULL);
    assert(data != NULL);
    assert(length > 0U);

    root->string_length = length;

    if(!CJSON_init_arenas(root, length, CJSON_PARSE_ARENA_MAX_NODES)) {
        return false;
    }

    if(!CJSON_Tokens_init(&root->tokens, length / CJSON_PARSE_TOKENS_SIZE_FACTOR)) {
        root->json.type       = CJSON_ERROR;
        root->json.data.error = CJSON_ERROR_MEMORY;
        return false;
    }

    struct CJSON_Lexer lexer;
    CJSON_Lexer_init(&lexer, data, length);
    
    struct CJSON_Token *token;
    do {
        if((token = CJSON_Tokens_next(&root->tokens)) == NULL) {
            root->json.type       = CJSON_ERROR;
            root->json.data.error = CJSON_ERROR_MEMORY;
            return false;
        }
    } while(CJSON_Lexer_tokenize(&lexer, &root->tokens, token));

    if(token->type != CJSON_TOKEN_DONE) {
        root->json.type       = CJSON_ERROR;
        root->json.data.error = CJSON_ERROR_TOKEN;

        return false;
    }

    CJSON_parse_tokens(root, &root->json, &root->tokens);

    return root->json.type != CJSON_ERROR;
}

EXTERN_C bool CJSON_parse_file(struct CJSON_Root *const root, const char *const path) {
    assert(root != NULL);
    assert(path != NULL);
    assert(strlen(path) > 0);

    struct CJSON_Buffer buffer;
    const enum CJSON_UtilError error = file_get_contents(path, &buffer);  
    if(error != CJSON_UTIL_ERROR_NONE) {
        root->json.type       = CJSON_ERROR;
        root->json.data.error = CJSON_ERROR_FILE;
        return false;
    }

    const bool success = CJSON_parse(root, (const char*)buffer.data, buffer.size);

    CJSON_FREE(buffer.data);

    return success;
}

EXTERN_C void CJSON_free(struct CJSON_Root *const root) {
    root->json.type      = CJSON_NULL;
    root->json.data.null = NULL;

    struct CJSON_Arena *arenas[] = {
        &root->object_arena,
        &root->array_arena,
        &root->string_arena
    };
    
    for(int i = 0; i < (int)(sizeof(arenas)/sizeof(*arenas)); i++) {
        CJSON_Arena_free(arenas[i]);
    }

    CJSON_Tokens_free(&root->tokens);
}

EXTERN_C const char *CJSON_get_error(const struct CJSON *const json) {
    assert(json != NULL);

    if(json->type != CJSON_ERROR) {
        return NULL;
    }

    switch(json->data.error) {
    case CJSON_ERROR_TOKEN:
        return "Token error";
    case CJSON_ERROR_STRING:
        return "String failed to parse";
    case CJSON_ERROR_FLOAT64:
        return "Float64 failed to parse";
    case CJSON_ERROR_INT64:
        return "Int64 failed to parse";
    case CJSON_ERROR_UINT64:
        return "Uint64 failed to parse";
    case CJSON_ERROR_OBJECT:
        return "Object failed to parse";
    case CJSON_ERROR_OBJECT_KEY:
        return "Object invalid key";
    case CJSON_ERROR_OBJECT_VALUE:
        return "Object invalid value";
    case CJSON_ERROR_MISSING_COLON:
        return "Object missing colon";
    case CJSON_ERROR_MISSING_COMMA_OR_RCURLY:
        return "Missing comma or right curly bracket";
    case CJSON_ERROR_ARRAY:
        return "Array failed to parse";
    case CJSON_ERROR_ARRAY_VALUE:
        return "Array invalid value";
    case CJSON_ERROR_FILE:
        return "Failed to open file";
    case CJSON_ERROR_MEMORY:
        return "Failed to allocate memory";
    }

    return NULL;
}

EXTERN_C struct CJSON *CJSON_get(struct CJSON *json, const char *query) {
    assert(json != NULL);
    assert(query != NULL);
    
    if(json->type != CJSON_OBJECT && json->type != CJSON_ARRAY) {
        return NULL;
    }

    if(*query == '\0' || strlen(query) > (size_t)UINT_MAX) {
        return NULL;
    }

    bool is_object_key = *query != '[';
    if(*query == '.' || *query == '[') {
        query++;
    }

    char *key;
    size_t key_size;
    while(true) {
        if(is_object_key && json->type == CJSON_OBJECT) {
            key_size = 0U;
            while(*query != '.' && *query != '[' && *query != '\0') {
                query++;
                key_size++;
            }

            key = (char*)CJSON_MALLOC((key_size + 1U) * sizeof(char));
            if(key == NULL) {
                return NULL;
            }
            memcpy(key, query - key_size, key_size);
            key[key_size] = '\0';

            json = CJSON_Object_get(&json->data.object, key);
            CJSON_FREE(key);    
        } else if(!is_object_key && json->type == CJSON_ARRAY) {
            key_size = 0U;
            while(*query != ']' && *query != '\0') {
                if(*query < '0' || *query > '9') {
                    return NULL;
                }
                query++;
                key_size++;
            }

            if(*query != ']' || key_size > UNSIGNED_MAX_LENGTH) {
                return NULL;
            }

            key = (char*)CJSON_MALLOC((key_size + 1U) * sizeof(char));
            if(key == NULL) {
                return NULL;
            }
            memcpy(key, query - key_size, key_size);
            key[key_size] = '\0';

            bool success;
            uint64_t index = parse_uint64(key, &success);
            CJSON_FREE(key);
            if(!success) {
                return NULL;
            }

            json = CJSON_Array_get(&json->data.array, (unsigned)index);
            query++;
        } else {
            return NULL;
        }

        if(json == NULL || *query == '\0') {
            break;
        }
        
        is_object_key = *query != '[';
        query++;
    }
    
    return json;
}

#define CJSON_GET_VALUE(JSON_TYPE, MEMBER)\
    assert(json != NULL);\
    assert(json->type != CJSON_ERROR);\
    assert(query != NULL);\
    assert(success != NULL);\
                            \
    struct CJSON *const ret = CJSON_get(json, query);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        return 0;\
    }\
    *success = true;\
    return ret->data.MEMBER;
 
#define CJSON_GET_PTR(JSON_TYPE, MEMBER)\
    assert(json != NULL);\
    assert(json->type != CJSON_ERROR);\
    assert(query != NULL);\
    assert(success != NULL);\
                            \
    struct CJSON *const ret = CJSON_get(json, query);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        return 0;\
    }\
    *success = true;\
    return &ret->data.MEMBER;

const char *CJSON_get_string(struct CJSON *const json, const char *const query, bool *const success) {
    CJSON_GET_VALUE(CJSON_STRING, string.chars)
}

double CJSON_get_float64(struct CJSON *const json, const char *const query, bool *const success) {
    CJSON_GET_VALUE(CJSON_FLOAT64, float64)
}

int64_t CJSON_get_int64(struct CJSON *const json, const char *const query, bool *const success) {
    CJSON_GET_VALUE(CJSON_INT64, int64)
}

uint64_t CJSON_get_uint64(struct CJSON *const json, const char *const query, bool *const success) {
    CJSON_GET_VALUE(CJSON_UINT64, uint64)
}

struct CJSON_Object *CJSON_get_object(struct CJSON *const json, const char *const query, bool *const success) {
    CJSON_GET_PTR(CJSON_OBJECT, object)
}

struct CJSON_Array *CJSON_get_array(struct CJSON *const json, const char *const query, bool *const success) {
    CJSON_GET_PTR(CJSON_ARRAY, array)
}

void *CJSON_get_null(struct CJSON *const json, const char *const query, bool *const success) {
    CJSON_GET_VALUE(CJSON_NULL, null)
}

bool CJSON_get_bool(struct CJSON *const json, const char *const query, bool *const success) {
    CJSON_GET_VALUE(CJSON_BOOL, boolean)
}

bool CJSON_set_string(struct CJSON *const json, struct CJSON_Root *const root, const char *const value) {
    assert(json != NULL);
    assert(value != NULL);

    const size_t length = strlen(value);
    if(length >= (size_t)UINT_MAX) {
        return false;
    }

    char *const copy = CJSON_Arena_strdup(&root->string_arena, value, &json->data.string.length);
    if(copy == NULL) {
        return false;
    }

    json->type              = CJSON_STRING;
    json->data.string.chars = copy;

    return true;
}

void CJSON_set_float64(struct CJSON *const json, const double value) {
    assert(json != NULL);

    json->type         = CJSON_FLOAT64;
    json->data.float64 = value;
}

void CJSON_set_int64(struct CJSON *const json, const int64_t value) {
    assert(json != NULL);

    json->type       = CJSON_INT64;
    json->data.int64 = value;
}

void CJSON_set_uint64(struct CJSON *const json, const uint64_t value) {
    assert(json != NULL);

    json->type        = CJSON_UINT64;
    json->data.uint64 = value;
}

void CJSON_set_object(struct CJSON *const json, const struct CJSON_Object *const value) {
    assert(json != NULL);
    assert(value != NULL);

    json->type        = CJSON_OBJECT;
    json->data.object = *value;
}

void CJSON_set_array(struct CJSON *const json, const struct CJSON_Array *const value) {
    assert(json != NULL);
    assert(value != NULL);
    
    json->type       = CJSON_ARRAY;
    json->data.array = *value;
}

void CJSON_set_null(struct CJSON *const json) {
    assert(json != NULL);

    json->type      = CJSON_NULL;
    json->data.null = NULL;
}

void CJSON_set_bool(struct CJSON *const json, const bool value) {
    assert(json != NULL);

    json->type         = CJSON_BOOL;
    json->data.boolean = value;
}
