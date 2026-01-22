#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "allocator.h"
#include "util.h"
#include "file.h"
#include "lexer.h"

#define CJSON_DEFAULT_ARENA_SIZE     CJSON_ARENA_MINIMUM_SIZE
#define CJSON_DEFAULT_ARENA_NODE_MAX CJSON_ARENA_INFINITE_NODES

static bool CJSON_parse_token(struct CJSON_Parser*, struct CJSON*, struct CJSON_Tokens*);

static bool CJSON_decode_string_token(struct CJSON_Parser *const parser, struct CJSON_String *const string, struct CJSON_Tokens *const tokens) {
    assert(parser != NULL);
    assert(string != NULL);
    assert(tokens != NULL);
    assert(tokens->current_token->length >= 2);

    const struct CJSON_Token *const token = tokens->current_token;
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
            const uint16_t high = CJSON_hex_to_utf16(input_current, &success);
            //\u0000 is apparently an acceptable escaped character in JSON so if the parsing function returns 0 then it's either an error or \u0000. \0 would break cstrings so it's gonna be treated as an error as well.
            if(high == 0U) {
                return false;    
            }

            if(IS_VALID_2_BYTES_UTF16(high)) {
                output_current += CJSON_utf16_to_utf8_2bytes(output_current, high);
                input_current  += 4;
                escaping        = false;
                continue;
            }

            if(input_end - input_current < 9 || input_current[4] != '\\' || input_current[5] != 'u') {
                return false;
            }

            const uint16_t low = CJSON_hex_to_utf16(input_current + 6, &success);
            if(!success || !IS_VALID_4_BYTES_UTF16(high, low)) {
                return false; 
            }

            CJSON_utf16_to_utf8_4bytes(output_current, high, low);
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

    const unsigned length = (unsigned)(output_current - output_start);
    output_start[length]  = '\0';
    string->chars         = output_start;
    string->length        = length;

    return true;
}

static bool CJSON_parse_string(struct CJSON_Parser *const parser, struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(parser != NULL);
    assert(json != NULL);
    assert(tokens != NULL);
    assert(tokens->current_token != NULL);
    
    if(!CJSON_decode_string_token(parser, &json->value.string, tokens)) {
        parser->error = CJSON_ERROR_STRING;
        return false;
    }

    json->type = CJSON_STRING;
    tokens->current_token++;
    return true;
}

static bool CJSON_parse_number(struct CJSON_Parser *const parser, struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(parser != NULL);
    assert(json != NULL);
    assert(tokens != NULL);
    assert(tokens->current_token != NULL);

    char number[1 << 9] = {0};

    bool success;
    const struct CJSON_Token *const token = tokens->current_token;
    if((size_t)token->length >= sizeof(number)) {
        return false;
    }

    memcpy(number, token->value, (size_t)token->length);

    if(token->type == CJSON_TOKEN_FLOAT) {
        const double value = CJSON_parse_float64(number, &success);
        if(!success) {
            parser->error = CJSON_ERROR_FLOAT64;
            return false;
        }

        json->type          = CJSON_FLOAT64;
        json->value.float64 = value;
    } else if(number[0] == '-') {
        if(token->type == CJSON_TOKEN_SCIENTIFIC_INT) {
            const long double value = CJSON_parse_long_double(number, &success);
            if(!success || value < (long double)INT64_MIN || value > (long double)INT64_MAX) {
                parser->error = CJSON_ERROR_INT64;
                return false;
            }
            json->type        = CJSON_INT64;
            json->value.int64 = (int64_t)value;

        } else {
            const int64_t value = CJSON_parse_int64(number, &success);
            if(!success) {
                parser->error = CJSON_ERROR_INT64;
                return false;     
            }
            json->type        = CJSON_INT64;
            json->value.int64 = value;
        }
    } else if(token->type == CJSON_TOKEN_SCIENTIFIC_INT) {
        const long double value = CJSON_parse_long_double(number, &success);
        if(!success || value > (long double)UINT64_MAX) {
            parser->error = CJSON_ERROR_UINT64;
            return false;
        }
        json->type         = CJSON_UINT64;
        json->value.uint64 = (uint64_t)value;
    } else {
        const uint64_t value = CJSON_parse_uint64(number, &success);
        if(!success) {
            parser->error = CJSON_ERROR_UINT64;
            return false; 
        }
        json->type         = CJSON_UINT64;
        json->value.uint64 = value;
    }

    tokens->current_token++;
    return true;
}

static void CJSON_parse_bool(struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);
    assert(tokens->current_token != NULL);

    json->type = CJSON_BOOL;
    json->value.boolean = tokens->current_token->value[0] == 't';

    tokens->current_token++;
}

static void CJSON_parse_null(struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);
    assert(tokens->current_token != NULL);

    json->type = CJSON_NULL;
    json->value.null = NULL;

    tokens->current_token++;
}

static bool CJSON_parse_object( struct CJSON_Parser *const parser, struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(parser != NULL);
    assert(json != NULL);
    assert(tokens != NULL);
    assert(tokens->current_token != NULL);

    const unsigned length = tokens->current_token->length;
    tokens->current_token++;

    const struct CJSON_Token *const last_token = tokens->data + tokens->count - 1;
    if(tokens->current_token == last_token) {
        parser->error = CJSON_ERROR_OBJECT;
        return false;
    }

    struct CJSON_Object *const object = &json->value.object;
    CJSON_Object_init(object);
    if(!CJSON_Object_reserve(object, parser, length)) {
        parser->error = CJSON_ERROR_MEMORY;
        return false;
    }

    if(tokens->current_token->type == CJSON_TOKEN_RCURLY) {
        json->type = CJSON_OBJECT;
        tokens->current_token++;
        return true;
    }

    while(last_token - tokens->current_token >= 4) {
        if(tokens->current_token->type != CJSON_TOKEN_STRING) {
            parser->error = CJSON_ERROR_OBJECT_KEY;
            return false;
        }

        struct CJSON_String key;
        if(!CJSON_decode_string_token(parser, &key, tokens)) {
            parser->error = CJSON_ERROR_OBJECT_KEY;
            return false;
        }

        tokens->current_token++;
    
        if(tokens->current_token->type != CJSON_TOKEN_COLON) {
            parser->error = CJSON_ERROR_MISSING_COLON;
            return false;
        }

        tokens->current_token++;

        struct CJSON_KV *const entry = CJSON_Object_get_entry(object, parser, key.chars);
        if(entry == NULL) {
            parser->error = CJSON_ERROR_MEMORY;
            return false;
        }
        entry->key = key.chars;

        if(!CJSON_parse_token(parser, &entry->value, tokens)) {
            if(parser->error == CJSON_ERROR_TOKEN) {
                parser->error = CJSON_ERROR_OBJECT_VALUE;
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

        parser->error = CJSON_ERROR_MISSING_COMMA_OR_RCURLY;
        return false;
    }
    
    parser->error = CJSON_ERROR_OBJECT;
    return false;
}

static bool CJSON_parse_array(struct CJSON_Parser *const parser, struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(parser != NULL);
    assert(json != NULL);
    assert(tokens != NULL);
    assert(tokens->current_token != NULL);

    const unsigned length = tokens->current_token->length;
    tokens->current_token++;

    const struct CJSON_Token *const last_token = tokens->data + tokens->count - 1U;
    if(tokens->current_token == last_token) {
        parser->error = CJSON_ERROR_ARRAY;
        return false;
    }
    
    struct CJSON_Array *const array = &json->value.array;
    CJSON_Array_init(array);
    if(!CJSON_Array_reserve(array, parser, length)) {
        parser->error = CJSON_ERROR_MEMORY;
        return false;
    }

    if(tokens->current_token->type == CJSON_TOKEN_RBRACKET) {
        json->type = CJSON_ARRAY;
        tokens->current_token++;
        return true;
    }

    while(last_token - tokens->current_token >= 2) {
        struct CJSON *const next_json = CJSON_Array_next(array, parser);
        assert(next_json != NULL);

        if(!CJSON_parse_token(parser, next_json, tokens)) {
            if(parser->error == CJSON_ERROR_TOKEN) {
                parser->error = CJSON_ERROR_ARRAY_VALUE;
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

        parser->error = CJSON_ERROR_MISSING_COMMA_OR_RBRACKET;
        return false;
    }

    parser->error = CJSON_ERROR_ARRAY;
    return false;
}

static bool CJSON_parse_token(struct CJSON_Parser *const parser, struct CJSON *const json, struct CJSON_Tokens *const tokens) {
    assert(parser != NULL);
    assert(json != NULL);

    switch(tokens->current_token->type) {
    case CJSON_TOKEN_STRING:
        return CJSON_parse_string(parser, json, tokens);
    
    case CJSON_TOKEN_INT:
    case CJSON_TOKEN_FLOAT:
    case CJSON_TOKEN_SCIENTIFIC_INT:
        return CJSON_parse_number(parser, json, tokens);

    case CJSON_TOKEN_BOOL:
        CJSON_parse_bool(json, tokens);
        return true;

    case CJSON_TOKEN_NULL:
        CJSON_parse_null(json, tokens);
        return true;

    case CJSON_TOKEN_LBRACKET:
        return CJSON_parse_array(parser, json, tokens);

    case CJSON_TOKEN_LCURLY:
        return CJSON_parse_object(parser, json, tokens);

    default:
        parser->error = CJSON_ERROR_TOKEN;
        return false;
    }

}

static bool CJSON_reserve_arenas(struct CJSON_Parser *const parser, const unsigned sizes[4]) {
    assert(parser != NULL);
    assert(sizes != NULL);

    static const unsigned CJSON_SIZES[] = {
        (unsigned)sizeof(struct CJSON_KV),
        (unsigned)sizeof(struct CJSON),
        (unsigned)sizeof(char),
        (unsigned)sizeof(struct CJSON)
    };

    struct CJSON_Arena *const arenas[] = {
        &parser->object_arena,
        &parser->array_arena,
        &parser->string_arena,
        &parser->json_arena,
        NULL
    };

    const unsigned alignments[] = {
        (unsigned)CJSON_ALIGNOF(struct CJSON_KV),
        (unsigned)CJSON_ALIGNOF(struct CJSON),
        (unsigned)CJSON_ALIGNOF(char),
        (unsigned)CJSON_ALIGNOF(struct CJSON) 
    };

    const unsigned *object_size = CJSON_SIZES;
    const unsigned *alignment = alignments;
    for(struct CJSON_Arena *const *arena = arenas; 
        *arena != NULL; 
        arena++, sizes++, object_size++, alignment++
    ) {
        assert(!CJSON_check_unsigned_mult_overflow(CJSON_DEFAULT_ARENA_SIZE, *object_size));

        const unsigned arena_size = MAX(*sizes, CJSON_DEFAULT_ARENA_SIZE * *object_size);
        if(!CJSON_Arena_reserve(*arena, arena_size, *alignment)) {
            return false;
        }
    }

    return true;
}

static bool CJSON_init_arenas(struct CJSON_Parser *const parser, const unsigned sizes[4]) {
    assert(parser != NULL);
    assert(sizes != NULL);

    static const unsigned CJSON_SIZES[] = {
        (unsigned)sizeof(struct CJSON_KV),
        (unsigned)sizeof(struct CJSON),
        (unsigned)sizeof(char),
        (unsigned)sizeof(struct CJSON)
    };

    struct CJSON_Arena *const arenas[] = {
        &parser->object_arena,
        &parser->array_arena,
        &parser->string_arena,
        &parser->json_arena,
        NULL
    };

    const unsigned *cjson_sizes = CJSON_SIZES;
    for(struct CJSON_Arena *const *arena = arenas; 
        *arena != NULL; 
        arena++, sizes++, cjson_sizes++
    ) {
        assert(!CJSON_check_unsigned_mult_overflow(CJSON_DEFAULT_ARENA_SIZE, *cjson_sizes));

        const unsigned arena_size = MAX(*sizes, CJSON_DEFAULT_ARENA_SIZE * *cjson_sizes);
        if(!CJSON_Arena_create_node(*arena, arena_size)) {
            return false;
        }
    }

    return true;
}

static struct CJSON *CJSON_Parser_new_json(struct CJSON_Parser *const parser) {
    assert(parser != NULL);

    struct CJSON *const json = CJSON_ARENA_ALLOC(&parser->json_arena, 1U, struct CJSON);
    if(json == NULL) {
        return NULL;
    }
    CJSON_set_null(json);

    return json;
}

EXTERN_C void CJSON_Parser_init(struct CJSON_Parser *const parser) {
    assert(parser != NULL);

    parser->error = CJSON_ERROR_NONE;
    CJSON_Arena_init(&parser->object_arena, CJSON_DEFAULT_ARENA_NODE_MAX, "Object Arena");
    CJSON_Arena_init(&parser->array_arena,  CJSON_DEFAULT_ARENA_NODE_MAX, "Array Arena");
    CJSON_Arena_init(&parser->string_arena, CJSON_DEFAULT_ARENA_NODE_MAX, "String Arena");
    CJSON_Arena_init(&parser->json_arena,   CJSON_DEFAULT_ARENA_NODE_MAX, "JSON Arena");
}

EXTERN_C void CJSON_Parser_free(struct CJSON_Parser *const parser) {
    assert(parser != NULL);

    CJSON_Arena_free(&parser->object_arena);
    CJSON_Arena_free(&parser->array_arena);
    CJSON_Arena_free(&parser->string_arena);
    CJSON_Arena_free(&parser->json_arena);
}

EXTERN_C struct CJSON *CJSON_parse(struct CJSON_Parser *const parser, const char *const data, const unsigned length) {
    assert(parser != NULL);
    assert(data != NULL);
    assert(length > 0U);

    struct CJSON *json = NULL;
    parser->error = CJSON_ERROR_NONE;

    struct CJSON_Tokens tokens;
    CJSON_Tokens_init(&tokens);
    if(!CJSON_Tokens_reserve(&tokens, length / 2U)) {
        parser->error = CJSON_ERROR_MEMORY;
        return NULL;
    }
    
    struct CJSON_Counters counters;
    CJSON_Counters_init(&counters);

    struct CJSON_Lexer lexer;
    CJSON_Lexer_init(&lexer, data, length);

    do {
        const enum CJSON_Lexer_Error error = CJSON_Lexer_tokenize(&lexer, &tokens, &counters);
        if(error == CJSON_LEXER_ERROR_TOKEN) {
            parser->error = CJSON_ERROR_TOKEN;
            break;
        }
        if(error == CJSON_LEXER_ERROR_MEMORY) {
            parser->error = CJSON_ERROR_MEMORY;
            break;
        }

        const unsigned arena_sizes[] = {
            counters.object_elements * (unsigned)sizeof(struct CJSON_KV),
            counters.array_elements  * (unsigned)sizeof(struct CJSON),
            counters.chars           * (unsigned)sizeof(char),
            CJSON_DEFAULT_ARENA_SIZE
        };

        if(parser->object_arena.head != NULL) {
            if(!CJSON_reserve_arenas(parser, arena_sizes)) {
                parser->error = CJSON_ERROR_MEMORY;
                break;
            }
        } else if(!CJSON_init_arenas(parser, arena_sizes)) {
            parser->error = CJSON_ERROR_MEMORY;
            break;
        }

        json = CJSON_Parser_new_json(parser);
        if(json == NULL) {
            parser->error = CJSON_ERROR_MEMORY;
            break;
        }

        if(!CJSON_parse_token(parser, json, &tokens)) {
            //remove the last CJSON allocated from the arena here
            json = NULL;
            break;
        }
    } while(0);

    CJSON_Tokens_free(&tokens);
    if(parser->error != CJSON_ERROR_NONE) {
        CJSON_Parser_free(parser);
    }
    return json;
}

EXTERN_C struct CJSON *CJSON_parse_file(struct CJSON_Parser *const parser, const char *const path) {
    assert(parser != NULL);
    assert(path != NULL);
    assert(path[0] != '\0');

    struct CJSON_FileContents file_contents;
    CJSON_FileContents_init(&file_contents);

    const enum CJSON_FileContents_Error error = CJSON_FileContents_get(&file_contents, path);  
    if(error != CJSON_FILECONTENTS_ERROR_NONE) {
        parser->error = CJSON_ERROR_FILE;
        return NULL;
    }

    struct CJSON *const json = CJSON_parse(parser, (const char*)file_contents.data, file_contents.size);

    CJSON_FileContents_free(&file_contents);
    
    return json;
}

EXTERN_C struct CJSON *CJSON_new(struct CJSON_Parser *const parser) {
    assert(parser != NULL);

    static const unsigned arena_default_sizes[] = {0U, 0U, 0U, 0U};

    struct CJSON *json = NULL;
    do {
        if(parser->object_arena.head != NULL 
            && !CJSON_init_arenas(parser, arena_default_sizes)
        ) {
            parser->error = CJSON_ERROR_MEMORY;
            break;
        }

        json = CJSON_Parser_new_json(parser);
        if(json == NULL) {
            parser->error = CJSON_ERROR_MEMORY;
            break;
        }
    } while(0);

    if(parser->error != CJSON_ERROR_NONE) {
        CJSON_Parser_free(parser);
    }
    
    return json;
}

EXTERN_C const char *CJSON_get_error(const struct CJSON_Parser *const parser) {
    assert(parser != NULL);

    switch(parser->error) {
    case CJSON_ERROR_NONE:
        return "No Error.";
    case CJSON_ERROR_TOKEN:
        return "Token error.";
    case CJSON_ERROR_STRING:
        return "String failed to parse.";
    case CJSON_ERROR_FLOAT64:
        return "Float64 failed to parse.";
    case CJSON_ERROR_INT64:
        return "Int64 failed to parse.";
    case CJSON_ERROR_UINT64:
        return "Uint64 failed to parse.";
    case CJSON_ERROR_OBJECT:
        return "Object failed to parse.";
    case CJSON_ERROR_OBJECT_KEY:
        return "Object invalid key.";
    case CJSON_ERROR_OBJECT_VALUE:
        return "Object invalid value.";
    case CJSON_ERROR_MISSING_COLON:
        return "Object missing colon.";
    case CJSON_ERROR_MISSING_COMMA_OR_RCURLY:
        return "Missing comma or right curly bracket in an object.";
    case CJSON_ERROR_MISSING_COMMA_OR_RBRACKET:
        return "Missing comma or right bracket in an array.";
    case CJSON_ERROR_ARRAY:
        return "Array failed to parse.";
    case CJSON_ERROR_ARRAY_VALUE:
        return "Array invalid value.";
    case CJSON_ERROR_FILE:
        return "Failed to open file.";
    case CJSON_ERROR_MEMORY:
        return "Failed to allocate memory.";
    }

    return NULL;
}

