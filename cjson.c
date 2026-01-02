#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cjson.h"
#include "lexer.h"
#include "tokens.h"
#include "allocator.h"
#include "util.h"

#define CJSON_DEFAULT_ARENA_SIZE     CJSON_ARENA_MINIMUM_SIZE
#define CJSON_DEFAULT_ARENA_NODE_MAX CJSON_ARENA_INFINITE_NODES

static bool CJSON_parse_tokens(struct CJSON_Parser*, struct CJSON*);

static bool CJSON_decode_string_token(struct CJSON_String *const string, struct CJSON_Parser *const parser) {
    assert(string != NULL);
    assert(parser != NULL);
    assert(parser->tokens.current_token->length >= 2);

    const struct CJSON_Token *const token = parser->tokens.current_token;

    char *output_current = CJSON_ARENA_ALLOC(&parser->string_arena, token->length - 1U, char);
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
        
        switch(*input_current) {
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

static bool CJSON_parse_string(struct CJSON *const json, struct CJSON_Parser *const parser) {
    assert(json != NULL);
    assert(parser != NULL);
    
    if(!CJSON_decode_string_token(&json->value.string, parser)) {
        json->type       = CJSON_ERROR;
        json->value.error = CJSON_ERROR_STRING;

        return false;
    }

    json->type = CJSON_STRING;

    parser->tokens.current_token++;

    return true;
}

static bool CJSON_parse_number(struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    char str[1 << 9] = {0};

    bool success;
    const struct CJSON_Token *const token = tokens->current_token; 

    memcpy(str, token->value, MIN(sizeof(str) - 1, token->length));

    if(token->type == CJSON_TOKEN_FLOAT) {
        const double value = parse_float64(str, &success);
        if(!success) {
            json->type = CJSON_ERROR;
            json->value.error = CJSON_ERROR_FLOAT64;

            return false;
        }

        json->type          = CJSON_FLOAT64;
        json->value.float64 = value;
    } else if(str[0] == '-') {
        if(token->type == CJSON_TOKEN_SCIENTIFIC_INT) {
            const long double value = parse_long_double(str, &success);
            if(!success || value < INT64_MIN || value > INT64_MAX) {
                json->type        = CJSON_ERROR;
                json->value.error = CJSON_ERROR_INT64;

                return false;
            }

            json->type        = CJSON_INT64;
            json->value.int64 = (int64_t)value;

        } else {
            const int64_t value = parse_int64(str, &success);
            if(!success) {
                json->type        = CJSON_ERROR;
                json->value.error = CJSON_ERROR_INT64;

                return false;     
            }

            json->type        = CJSON_INT64;
            json->value.int64 = value;
        }
    } else if(token->type == CJSON_TOKEN_SCIENTIFIC_INT) {
        const long double value = parse_long_double(str, &success);
        if(!success || value > UINT64_MAX) {
            json->type        = CJSON_ERROR;
            json->value.error = CJSON_ERROR_UINT64;

            return false;
        }

        json->type = CJSON_UINT64;
        json->value.uint64 = (uint64_t)value;
    } else {
        const uint64_t value = parse_uint64(str, &success);
        if(!success) {
            json->type        = CJSON_ERROR;
            json->value.error = CJSON_ERROR_UINT64;

            return false; 
        }

        json->type = CJSON_UINT64;
        json->value.uint64 = value;
    }

    tokens->current_token++;
    
    return true;
}

static void CJSON_parse_bool(struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    json->type = CJSON_BOOL;
    json->value.boolean = tokens->current_token->value[0] == 't';

    tokens->current_token++;
}

static void CJSON_parse_null(struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    json->type = CJSON_NULL;
    json->value.null = NULL;

    tokens->current_token++;
}

static bool CJSON_parse_object(struct CJSON *const json, struct CJSON_Parser *const parser) {
    assert(json != NULL);
    assert(parser != NULL);

    json->type = CJSON_ERROR;

    struct CJSON_Tokens *const tokens = &parser->tokens;

    const unsigned length = tokens->current_token->length;

    tokens->current_token++;

    const struct CJSON_Token *const last_token = tokens->data + tokens->count - 1;
    if(tokens->current_token == last_token) {
        json->type        = CJSON_ERROR;
        json->value.error = CJSON_ERROR_OBJECT;
        return false;
    }

    struct CJSON_Object *const object  = &json->value.object;
    CJSON_Object_init(object);
    if(!CJSON_Object_reserve(object, parser, length)) {
        json->value.error = CJSON_ERROR_MEMORY;
        return false;
    }

    if(tokens->current_token->type == CJSON_TOKEN_RCURLY) {
        json->type = CJSON_OBJECT;
        tokens->current_token++;
        return true;
    }

    while(last_token - tokens->current_token >= 4) {
        if(tokens->current_token->type != CJSON_TOKEN_STRING) {
            json->value.error = CJSON_ERROR_OBJECT_KEY;
            return false;
        }

        struct CJSON_String key;
        if(!CJSON_decode_string_token(&key, parser)) {
            json->value.error = CJSON_ERROR_OBJECT_KEY;
            return false;
        }

        tokens->current_token++;
    
        if(tokens->current_token->type != CJSON_TOKEN_COLON) {
            json->value.error = CJSON_ERROR_MISSING_COLON;
            return false;
        }

        tokens->current_token++;

        struct CJSON_KV *const entry = CJSON_Object_get_entry(object, parser, key.chars);
        if(entry == NULL) {
            json->value.error = CJSON_ERROR_MEMORY;
            return false;
        }
        entry->key = key.chars;

        if(!CJSON_parse_tokens(parser, &entry->value)) {
            if(entry->value.value.error == CJSON_ERROR_TOKEN) {
                json->value.error = CJSON_ERROR_OBJECT_VALUE;
            } else {
                json->value.error = entry->value.value.error;
            }
            return false;
        }

        if(tokens->current_token->type == CJSON_TOKEN_COMMA) {
            tokens->current_token++;
            continue;
        }

        if(tokens->current_token->type == CJSON_TOKEN_RCURLY) {
            json->type = CJSON_OBJECT;
            tokens->current_token++;
            return true;
        }

        json->value.error = CJSON_ERROR_MISSING_COMMA_OR_RCURLY;
        return false;
    }
    
    json->value.error = CJSON_ERROR_OBJECT;
    return false;
}

static bool CJSON_parse_array(struct CJSON *const json, struct CJSON_Parser *const parser) {
    assert(json != NULL);
    assert(parser != NULL);

    json->type = CJSON_ERROR;

    struct CJSON_Tokens *const tokens = &parser->tokens;

    const unsigned length = tokens->current_token->length;

    tokens->current_token++;

    const struct CJSON_Token *const last_token = tokens->data + tokens->count - 1U;
    if(tokens->current_token == last_token) {
        json->type       = CJSON_ERROR;
        json->value.error = CJSON_ERROR_ARRAY;
        return false;
    }
    
    struct CJSON_Array *const array = &json->value.array;
    CJSON_Array_init(array);
    if(!CJSON_Array_reserve(array, parser, length)) {
        json->value.error = CJSON_ERROR_MEMORY;
        return false;
    }

    if(tokens->current_token->type == CJSON_TOKEN_RBRACKET) {
        json->type = CJSON_ARRAY;
        tokens->current_token++;
        return true;
    }

    while(last_token - tokens->current_token >= 2) {
        struct CJSON *const next_json = CJSON_Array_next(array, parser);
        if(next_json == NULL) {
            json->value.error = CJSON_ERROR_MEMORY;
            return false;
        }

        if(!CJSON_parse_tokens(parser, next_json)) {
            if(next_json->value.error == CJSON_ERROR_TOKEN) {
                json->value.error = CJSON_ERROR_ARRAY_VALUE;
            } else {
                json->value.error = next_json->value.error;
            }
            return false;
        }
 
        if(tokens->current_token->type == CJSON_TOKEN_COMMA) {
            tokens->current_token++;
            continue;
        }

        if(tokens->current_token->type == CJSON_TOKEN_RBRACKET) {
            json->type = CJSON_ARRAY;
            tokens->current_token++;
            return true;
        }

        json->value.error = CJSON_ERROR_MISSING_COMMA_OR_RCURLY;
        return false;
    }

    json->value.error = CJSON_ERROR_ARRAY;
    return false;
}

static bool CJSON_parse_tokens(struct CJSON_Parser *const parser, struct CJSON *const json) {
    assert(parser != NULL);
    assert(json != NULL);

    switch(parser->tokens.current_token->type) {
    case CJSON_TOKEN_STRING: {
        return CJSON_parse_string(json, parser);
    }
    case CJSON_TOKEN_INT:
    case CJSON_TOKEN_FLOAT:
    case CJSON_TOKEN_SCIENTIFIC_INT: {
        return CJSON_parse_number(json, &parser->tokens);
    }
    case CJSON_TOKEN_BOOL: {
        CJSON_parse_bool(json, &parser->tokens);
        return true;
    }
    case CJSON_TOKEN_NULL: {
        CJSON_parse_null(json, &parser->tokens);
        return true;
    }
    case CJSON_TOKEN_LBRACKET: {
        return CJSON_parse_array(json, parser);
    }
    case CJSON_TOKEN_LCURLY: {
        return CJSON_parse_object(json, parser);
    }
    case CJSON_TOKEN_COLON:
    case CJSON_TOKEN_COMMA:
    case CJSON_TOKEN_RBRACKET:
    case CJSON_TOKEN_RCURLY:
    case CJSON_TOKEN_INVALID:
    case CJSON_TOKEN_DONE:
        json->type       = CJSON_ERROR;
        json->value.error = CJSON_ERROR_TOKEN;
    }

    return false;
}

static bool CJSON_reserve_arenas(struct CJSON_Parser *const parser, const unsigned sizes[3]) {
    assert(parser != NULL);
    assert(sizes != NULL);

    if(!CJSON_Arena_reserve(&parser->object_arena, sizes[0], CJSON_ALIGNOF(struct CJSON_KV))) {
        return false;
    }

    if(!CJSON_Arena_reserve(&parser->array_arena, sizes[1], CJSON_ALIGNOF(struct CJSON))) {
        return false;
    }

    if(!CJSON_Arena_reserve(&parser->string_arena, sizes[2], CJSON_ALIGNOF(char))) {
        return false;
    }

    return true;
}

static bool CJSON_init_arenas(struct CJSON_Parser *const parser, const unsigned sizes[3]) {
    assert(parser != NULL);
    assert(sizes != NULL);

    static const unsigned CJSON_SIZES[] = {
        (unsigned)sizeof(struct CJSON_KV),
        (unsigned)sizeof(struct CJSON),
        (unsigned)sizeof(char),
    };

    struct CJSON_Arena *const arenas[] = {
        &parser->object_arena,
        &parser->array_arena,
        &parser->string_arena,
        NULL
    };

    const unsigned *object_size = CJSON_SIZES;
    for(struct CJSON_Arena *const *arena = arenas; 
        *arena != NULL; 
        arena++, sizes++, object_size++
    ) {
        const unsigned arena_size = MAX(*sizes, CJSON_DEFAULT_ARENA_SIZE * *object_size);
        if(!CJSON_Arena_create_node(*arena, arena_size)) {
            return false;
        }
    }

    return true;
}

static void CJSON_Counters_init(struct CJSON_Counters *const counters) {
    assert(counters != NULL);

    counters->object          = 0U;
    counters->array           = 0U;
    counters->number          = 0U;
    counters->string          = 0U;
    counters->keyword         = 0U;
    counters->chars           = 0U;
    counters->comma           = 0U;
    counters->object_elements = 0U;
    counters->array_elements  = 0U;
}

static void CJSON_common_init(struct CJSON_Parser *const parser) {
    assert(parser != NULL);

    parser->json.type                  = CJSON_NULL;
    parser->json.value.null            = NULL;

    CJSON_Counters_init(&parser->counters);
    CJSON_Arena_init(&parser->object_arena, CJSON_DEFAULT_ARENA_NODE_MAX, "Object Arena");
    CJSON_Arena_init(&parser->array_arena, CJSON_DEFAULT_ARENA_NODE_MAX, "Array Arena");
    CJSON_Arena_init(&parser->string_arena, CJSON_DEFAULT_ARENA_NODE_MAX, "String Arena");
    CJSON_Tokens_init(&parser->tokens);
}

EXTERN_C bool CJSON_empty_init(struct CJSON_Parser *const parser) {
    assert(parser != NULL);

    CJSON_common_init(parser);

    static const unsigned arena_default_sizes[] = {0U, 0U, 0U};
    if(!CJSON_init_arenas(parser, arena_default_sizes)) {
        parser->json.type        = CJSON_ERROR;
        parser->json.value.error = CJSON_ERROR_MEMORY;
        return false;
    }

    return true;
}

EXTERN_C struct CJSON_Array *CJSON_make_array(struct CJSON *const json, struct CJSON_Parser *const parser) {
    assert(json != NULL);
    assert(parser != NULL);

    struct CJSON_Array *const array = &json->value.array;

    CJSON_Array_init(array);
    if(!CJSON_Array_reserve(array, parser, 0U)) {
        return NULL;
    }
    json->type = CJSON_ARRAY;

    return array;
}

EXTERN_C struct CJSON_Object *CJSON_make_object(struct CJSON *const json, struct CJSON_Parser *const parser) {
    assert(json != NULL);

    struct CJSON_Object *const object = &json->value.object;

    CJSON_Object_init(object);
    if(!CJSON_Object_reserve(object, parser, 0U)) {
        return NULL;
    }
    json->type = CJSON_OBJECT;

    return object;
}

EXTERN_C void CJSON_parse_init(struct CJSON_Parser *const parser) {
    assert(parser != NULL);

    CJSON_common_init(parser);
}

EXTERN_C bool CJSON_parse(struct CJSON_Parser *const parser, const char *const data, const unsigned length) {
    assert(parser != NULL);
    assert(data != NULL);
    assert(length > 0U);

    CJSON_Counters_init(&parser->counters);

    struct CJSON_Lexer lexer;
    CJSON_Lexer_init(&lexer, data, length);

    if(parser->tokens.data == NULL) {
        CJSON_Tokens_init(&parser->tokens);
    } else {
        CJSON_Tokens_reset(&parser->tokens);
    }

    if(!CJSON_Tokens_reserve(&parser->tokens, length / 2U)) {
        parser->json.value.error = CJSON_ERROR_MEMORY;
        parser->json.type = CJSON_ERROR;
        return false;
    }
    
    enum CJSON_Lexer_Error error = CJSON_Lexer_tokenize(&lexer, parser);
    if(error == CJSON_LEXER_ERROR_TOKEN) {
        parser->json.value.error = CJSON_ERROR_TOKEN;
        parser->json.type        = CJSON_ERROR;
        return false;
    }

    if(error == CJSON_LEXER_ERROR_MEMORY) {
        parser->json.value.error = CJSON_ERROR_MEMORY;
        parser->json.type        = CJSON_ERROR;
        return false;
    }

    const unsigned arena_sizes[] = {
        parser->counters.object_elements * (unsigned)sizeof(struct CJSON_KV),
        parser->counters.array_elements  * (unsigned)sizeof(struct CJSON),
        parser->counters.chars           * (unsigned)sizeof(char)
    };

    if(parser->object_arena.head != NULL) {
        if(!CJSON_reserve_arenas(parser, arena_sizes)) {
            parser->json.value.error = CJSON_ERROR_MEMORY;
            parser->json.type        = CJSON_ERROR;
            return false;
        }
    } else if(!CJSON_init_arenas(parser, arena_sizes)) {
        parser->json.value.error = CJSON_ERROR_MEMORY;
        parser->json.type        = CJSON_ERROR;
        return false;
    }

    CJSON_parse_tokens(parser, &parser->json);

    return parser->json.type != CJSON_ERROR;
}

EXTERN_C bool CJSON_parse_file(struct CJSON_Parser *const parser, const char *const path) {
    assert(parser != NULL);
    assert(path != NULL);
    assert(strlen(path) > 0);

    struct CJSON_Buffer buffer;
    const enum CJSON_UtilError error = file_get_contents(path, &buffer);  
    if(error != CJSON_UTIL_ERROR_NONE) {
        parser->json.type       = CJSON_ERROR;
        parser->json.value.error = CJSON_ERROR_FILE;
        return false;
    }

    const bool success = CJSON_parse(parser, (const char*)buffer.data, buffer.size);

    CJSON_FREE(buffer.data);

    return success;
}

EXTERN_C void CJSON_free(struct CJSON_Parser *const parser) {
    parser->json.type       = CJSON_NULL;
    parser->json.value.null = NULL;

    CJSON_Arena_free(&parser->object_arena);
    CJSON_Arena_free(&parser->array_arena);
    CJSON_Arena_free(&parser->string_arena);
    CJSON_Tokens_free(&parser->tokens);
}

EXTERN_C const char *CJSON_get_error(const struct CJSON_Parser *const parser) {
    assert(parser != NULL);

    if(parser->json.type != CJSON_ERROR) {
        return NULL;
    }

    switch(parser->json.value.error) {
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

    size_t key_size;
    while(true) {
        if(is_object_key && json->type == CJSON_OBJECT) {
            char *key;
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

            json = CJSON_Object_get(&json->value.object, key);
            CJSON_FREE(key);    
        } else if(!is_object_key && json->type == CJSON_ARRAY) {
            char key[UNSIGNED_MAX_LENGTH + 1U];
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

            memcpy(key, query - key_size, key_size);
            key[key_size] = '\0';

            bool success;
            uint64_t index = parse_uint64(key, &success);
            if(!success) {
                return NULL;
            }

            json = CJSON_Array_get(&json->value.array, (unsigned)index);
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

#define CJSON_GET_TYPE        CJSON_STRING
#define CJSON_GET_MEMBER      string.chars
#define CJSON_GET_SUFFIX      string
#define CJSON_GET_RETURN_TYPE const char*
#include "cjson-get-template.h"

#define CJSON_GET_TYPE        CJSON_FLOAT64
#define CJSON_GET_MEMBER      float64
#define CJSON_GET_SUFFIX      float64
#define CJSON_GET_RETURN_TYPE double
#include "cjson-get-template.h"

#define CJSON_GET_TYPE        CJSON_INT64
#define CJSON_GET_MEMBER      int64
#define CJSON_GET_SUFFIX      int64
#define CJSON_GET_RETURN_TYPE int64_t
#include "cjson-get-template.h"

#define CJSON_GET_TYPE        CJSON_UINT64
#define CJSON_GET_MEMBER      uint64
#define CJSON_GET_SUFFIX      uint64
#define CJSON_GET_RETURN_TYPE uint64_t
#include "cjson-get-template.h"

#define CJSON_GET_TYPE        CJSON_OBJECT
#define CJSON_GET_MEMBER      object
#define CJSON_GET_SUFFIX      object
#define CJSON_GET_RETURN_TYPE struct CJSON_Object*
#define CJSON_GET_RETURN_PTR
#include "cjson-get-template.h"

#define CJSON_GET_TYPE        CJSON_ARRAY
#define CJSON_GET_MEMBER      array
#define CJSON_GET_SUFFIX      array
#define CJSON_GET_RETURN_TYPE struct CJSON_Array*
#define CJSON_GET_RETURN_PTR
#include "cjson-get-template.h"

#define CJSON_GET_TYPE        CJSON_NULL
#define CJSON_GET_MEMBER      null
#define CJSON_GET_SUFFIX      null
#define CJSON_GET_RETURN_TYPE void*
#include "cjson-get-template.h"

#define CJSON_GET_TYPE        CJSON_BOOL
#define CJSON_GET_MEMBER      boolean
#define CJSON_GET_RETURN_TYPE bool
#define CJSON_GET_SUFFIX_BOOL
#include "cjson-get-template.h"

EXTERN_C bool CJSON_set_string(struct CJSON *const json, struct CJSON_Parser *const parser, const char *const value) {
    assert(json != NULL);
    assert(value != NULL);

    const size_t length = strlen(value);
    if(length >= (size_t)UINT_MAX) {
        return false;
    }

    char *const copy = CJSON_Arena_strdup(&parser->string_arena, value, &json->value.string.length);
    if(copy == NULL) {
        return false;
    }

    json->type              = CJSON_STRING;
    json->value.string.chars = copy;

    return true;
}

EXTERN_C void CJSON_set_float64(struct CJSON *const json, const double value) {
    assert(json != NULL);
    
    json->type         = CJSON_FLOAT64;
    json->value.float64 = value;
}

EXTERN_C void CJSON_set_int64(struct CJSON *const json, const int64_t value) {
    assert(json != NULL);

    json->type       = CJSON_INT64;
    json->value.int64 = value;
}

EXTERN_C void CJSON_set_uint64(struct CJSON *const json, const uint64_t value) {
    assert(json != NULL);

    json->type        = CJSON_UINT64;
    json->value.uint64 = value;
}

EXTERN_C void CJSON_set_object(struct CJSON *const json, const struct CJSON_Object *const value) {
    assert(json != NULL);
    assert(value != NULL);

    if((void*)&json->value == (const void*)value) {
        return;
    }

    json->type        = CJSON_OBJECT;
    json->value.object = *value;
}

EXTERN_C void CJSON_set_array(struct CJSON *const json, const struct CJSON_Array *const value) {
    assert(json != NULL);
    assert(value != NULL);

    if((void*)&json->value == (const void*)value) {
        return;
    }
    
    json->type       = CJSON_ARRAY;
    json->value.array = *value;
}

EXTERN_C void CJSON_set_null(struct CJSON *const json) {
    assert(json != NULL);

    json->type      = CJSON_NULL;
    json->value.null = NULL;
}

EXTERN_C void CJSON_set_bool(struct CJSON *const json, const bool value) {
    assert(json != NULL);

    json->type         = CJSON_BOOL;
    json->value.boolean = value;
}