#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "lexer.h"
#include "tokens.h"
#include "util.h"
#include "allocator.h"
#include "benchmark.h"

typedef enum CJSON_ObjectParsingError {
    CJSON_ObjectKeyError,
    CJSON_ObjectColonError,
    CSJON_ObjectValueError,
    CJSON_ObjectCommaError,
    CJSON_ObjectIncompleteError,
} CJSON_ObjectParsingError;

typedef enum CJSON_ArrayParsingError {
    CJSON_ArrayValueError,
    CJSON_ArrayCommaError,
    CJSON_ArrayIncompleteError
} CJSON_ArrayParsingError;
 
static bool CJSON_parse_tokens(CJSON_JSON *const node, CJSON_Tokens *const tokens);

static char *CJSON_parse_utf8_string(const CJSON_Token *const token) {
    assert(token != NULL);

    char* string = CJSON_MALLOC((token->length - 1) * sizeof(char));
    assert(string != NULL);

    unsigned int str_index = 0U;
    unsigned int tok_index = 1U;
    const char *const value = token->value;
    bool escaping = false;

    while(tok_index < token->length - 1U) {
        switch(value[tok_index]) {
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                goto cleanup;
        }

        if(!escaping) {
            if(value[tok_index] == '\\') {
                escaping = true;
                tok_index++;
                continue;
            }

            string[str_index++] = value[tok_index++];
            continue;
        } 
        
        else switch(value[tok_index]) {
        case '"':
            string[str_index++] = '"';
            tok_index++;
            escaping = false;
            continue;
        case 'b':
            string[str_index++] = '\b';
            tok_index++;
            escaping = false;
            continue;
        case 'f':
            string[str_index++] = '\f';
            tok_index++;
            escaping = false;
            continue;
        case 'n':
            string[str_index++] = '\n';
            tok_index++;
            escaping = false;
            continue;
        case 'r':
            string[str_index++] = '\r';
            tok_index++;
            escaping = false;
            continue;
        case 't':
            string[str_index++] = '\t';
            tok_index++;
            escaping = false;
            continue;
        case '/':
            string[str_index++] = '/';
            tok_index++;
            escaping = false;
            continue;
        case '\\': {
            string[str_index++] = '\\';
            tok_index++;
            escaping = false;
            continue;
        }
        case 'u': {
            if(tok_index + 4U >= token->length - 1U) {
                goto cleanup;
            }

            tok_index++;

            bool success;
            const uint16_t codepoint = parse_codepoint(value + tok_index, &success);
            //\u0000 is apparently an acceptable escaped character in JSON so if the parsing function returns 0 then it's either an error or \u0000. \0 would break cstrings so it's gonna be treated as an error as well.
            if(codepoint == 0U) {
                goto cleanup;    
            }

            if(VALID_2_BYTES_UTF16(codepoint)) {
                str_index += codepoint_utf16_to_utf8(string + str_index, codepoint);
                tok_index += 4U;
                escaping = false;
                continue;
            }

            if(tok_index + 9U >= token->length - 1U || value[tok_index + 4U] != '\\' || value[tok_index + 5U] != 'u') {
                goto cleanup;
            }

            const uint16_t low = parse_codepoint(value + tok_index + 6U, &success);
            if(!success || !VALID_4_BYTES_UTF16(codepoint, low)) {
                goto cleanup;  
            }

            surrogate_utf16_to_utf8(string + str_index, codepoint, low);
            str_index += 4U;
            tok_index += 10U;
            escaping = false;
            continue;
        }
        default:
            goto cleanup;         
    }
    }

    if(escaping) {
        goto cleanup;
    }

    string[str_index] = '\0';

    return string;

cleanup:
    CJSON_FREE(string);
    return NULL;
}

static bool CJSON_parse_string(CJSON_JSON *const node, CJSON_Tokens *const tokens) {
    assert(node != NULL);
    assert(tokens != NULL);

    BENCHMARK_START();
    
    const CJSON_Token *const token = tokens->data + tokens->index; 

    char *string = CJSON_parse_utf8_string(token);
    if(string == NULL) {
        *node = (CJSON_JSON) {
            .type = CJSON_ERROR,
            .value.error =  CJSON_STRING_FAILED_TO_PARSE
        };
        BENCHMARK_END();

        return false;
    }

    *node = (CJSON_JSON) {
        .type = CJSON_STRING,
        .value.string = string
    };

    tokens->index++;

    BENCHMARK_END();

    return true;
}

static bool CJSON_parse_number(CJSON_JSON *const node, CJSON_Tokens *const tokens) {
    assert(node != NULL);
    assert(tokens != NULL);

    BENCHMARK_START();

    char str[1 << 9] = {0};

    bool success;
    const CJSON_Token *const token = tokens->data + tokens->index; 

    memcpy(str, token->value, MIN(sizeof(str) - 1, token->length));

    if(token->type == CJSON_TOKEN_FLOAT) {
        const double float64 = parse_float64(str, &success);
        if(!success) {
            *node = (CJSON_JSON) {
                .type = CJSON_ERROR,
                .value.error = CJSON_FLOAT64_FAILED_TO_PARSE
            };
            BENCHMARK_END();

            return false;
        }

        *node = (CJSON_JSON) {
            .type = CJSON_FLOAT64,
            .value.float64 = float64
        };
    } else if(str[0] == '-') {
        if(token->type == CJSON_TOKEN_SCIENTIFIC_INT) {
            const long double long_double = parse_long_double(str, &success);
            if(!success || long_double < INT64_MIN || long_double > INT64_MAX) {
                *node = (CJSON_JSON) {
                    .type = CJSON_ERROR,
                    .value.error = CJSON_INT64_FAILED_TO_PARSE
                };
                BENCHMARK_END();

                return false;
            }

            *node = (CJSON_JSON) {
                .type = CJSON_INT64,
                .value.int64 = (int64_t)long_double,
            };
        } else {
            const int64_t int64 = parse_int64(str, &success);
            if(!success) {
                *node = (CJSON_JSON) {
                    .type = CJSON_ERROR,
                    .value.error = CJSON_INT64_FAILED_TO_PARSE
                };
                BENCHMARK_END();

                return false;     
            }

            *node = (CJSON_JSON) {
                .type = CJSON_INT64,
                .value.int64 = int64,
            };
        }
    } else if(token->type == CJSON_TOKEN_SCIENTIFIC_INT) {
        const long double long_double = parse_long_double(str, &success);
        if(!success || long_double > UINT64_MAX) {
            *node = (CJSON_JSON) {
                .type = CJSON_ERROR,
                .value.error = CJSON_UINT64_FAILED_TO_PARSE
            };
            BENCHMARK_END();

            return false;
        }

        *node = (CJSON_JSON) {
            .type = CJSON_UINT64,
            .value.uint64 = (uint64_t)long_double,
        };
    } else {
        const uint64_t uint64 = parse_uint64(str, &success);
        if(!success) {
            *node = (CJSON_JSON) {
                .type = CJSON_ERROR,
                .value.error = CJSON_UINT64_FAILED_TO_PARSE
            };
            BENCHMARK_END();

            return false; 
        }

        *node = (CJSON_JSON) {
            .type = CJSON_UINT64,
            .value.uint64 = uint64,
        };
    }

    tokens->index++;

    BENCHMARK_END();
    
    return true;
}

static void CJSON_parse_bool(CJSON_JSON *const node, CJSON_Tokens *const tokens) {
    assert(node != NULL);
    assert(tokens != NULL);

    BENCHMARK_START();

    const CJSON_Token *const token = tokens->data + tokens->index; 

    node->type = CJSON_BOOL;
    node->value.boolean = token->value[0] == 't';

    tokens->index++;

    BENCHMARK_END();
}

static void CJSON_parse_null(CJSON_JSON *const node, CJSON_Tokens *const tokens) {
    assert(node != NULL);
    assert(tokens != NULL);

    BENCHMARK_START();

    node->type = CJSON_NULL;
    node->value.null = NULL;

    tokens->index++;

    BENCHMARK_END();
}

static bool CJSON_parse_object(CJSON_JSON *const node, CJSON_Tokens *const tokens) {
    assert(node != NULL);
    assert(tokens != NULL);

    BENCHMARK_START();

    tokens->index++;

    if(tokens->length == tokens->index) {
        *node = (CJSON_JSON) {
            .type = CJSON_ERROR,
            .value.error = CJSON_OBJECT_FAILED_TO_PARSE
        };

        BENCHMARK_END();

        return false;
    }

    CJSON_Object *const object = &node->value.object;
    node->type = CJSON_OBJECT;
    CJSON_Object_init(object);

    CJSON_Error next_error;
    CJSON_ObjectParsingError error;
    const CJSON_Token *token = tokens->data + tokens->index;
    char *key = NULL;

    if(token->type == CJSON_TOKEN_RCURLY) {
        tokens->index++;
        BENCHMARK_END();

        return true;
    }

    while(tokens->index + 4U < tokens->length) {
        if(token->type != CJSON_TOKEN_STRING) {
            error = CJSON_ObjectKeyError;
            goto cleanup;
        }

        key = CJSON_parse_utf8_string(token);
        if(key == NULL) {
            error = CJSON_ObjectKeyError;
            goto cleanup;
        }

        token++;
    
        if(token->type != CJSON_TOKEN_COLON) {
            error = CJSON_ObjectColonError;
            tokens->index++;
            goto cleanup;
        }

        tokens->index += 2U;

        CJSON_Key_Value *const entry = CJSON_Object_get_entry(object, key);
        if(entry->key != NULL) {
            CJSON_FREE(key);
            CJSON_internal_free(&entry->value);
        } else {
            entry->key = key;
        }
        key = NULL;

        if(!CJSON_parse_tokens(&entry->value, tokens)) {
            error = CSJON_ObjectValueError;
            next_error = entry->value.value.error;
            goto cleanup;
        }

        token = tokens->data + tokens->index;

        if(token->type == CJSON_TOKEN_COMMA) {
            tokens->index++;
            token++;
            continue;
        }

        if(token->type == CJSON_TOKEN_RCURLY) {
            tokens->index++;
            BENCHMARK_END();

            return true;
        }

        error = CJSON_ObjectCommaError;
        goto cleanup;
    }
    
    error = CJSON_ObjectIncompleteError;

cleanup:
    CJSON_FREE(key);
    CJSON_Object_free(object);
    node->type = CJSON_ERROR;

    switch(error) {
    case CJSON_ObjectIncompleteError: {
        node->value.error = CJSON_OBJECT_FAILED_TO_PARSE;
        break;
    }
    case CJSON_ObjectKeyError: {
        node->value.error = CJSON_OBJECT_INVALID_KEY;
        break;
    }

    case CJSON_ObjectColonError: {
        node->value.error = CJSON_OBJECT_MISSING_COLON;
        break;
    }

    case CSJON_ObjectValueError: {
        if(next_error == CJSON_TOKEN_ERROR) {
            node->value.error = CJSON_OBJECT_INVALID_VALUE;
        } else {
            node->value.error = next_error;
        }
        break;
    }

    case CJSON_ObjectCommaError: {
        node->value.error = CJSON_OBJECT_MISSING_COMMA_OR_RCURLY;
        break;
    }
    }

    BENCHMARK_END();

    return false;
}

static bool CJSON_parse_array(CJSON_JSON *const node, CJSON_Tokens *const tokens) {
    assert(node != NULL);
    assert(tokens != NULL);

    BENCHMARK_START();

    tokens->index++;

    if(tokens->length == tokens->index) {
        *node = (CJSON_JSON) {
            .type = CJSON_ERROR,
            .value.error = CJSON_ARRAY_FAILED_TO_PARSE
        };
        BENCHMARK_END();

        return false;
    }

    CJSON_Error next_error;
    CJSON_ArrayParsingError error;
    CJSON_Array *const array = &node->value.array;
    node->type = CJSON_ARRAY;
    CJSON_Array_init(array);

    const CJSON_Token *token = tokens->data + tokens->index;

    if(token->type == CJSON_TOKEN_RBRACKET) {
        tokens->index++;
        BENCHMARK_END();

        return true;
    }

    while(tokens->index + 2U < tokens->length) {
        CJSON_JSON *const next_json = CJSON_Array_next(array);
        if(!CJSON_parse_tokens(next_json, tokens)) {
            error = CJSON_ArrayValueError;
            next_error = next_json->value.error;
            goto cleanup;
        }

        token = tokens->data + tokens->index;
 
        if(token->type == CJSON_TOKEN_COMMA) {
            tokens->index++;
            token++;
            continue;
        }

        if(token->type == CJSON_TOKEN_RBRACKET) {
            tokens->index++;
            BENCHMARK_END();

            return true;
        }

        error = CJSON_ArrayCommaError;
        goto cleanup;
    }

    error = CJSON_ArrayIncompleteError;

cleanup:
    node->type = CJSON_ERROR;
    CJSON_Array_free(array);
    
    switch(error) {
    case CJSON_ArrayIncompleteError: {
        *node = (CJSON_JSON) {
            .type = CJSON_ERROR,
            .value.error = CJSON_ARRAY_FAILED_TO_PARSE
        };
        break;
    }

    case CJSON_ArrayValueError: {
        if(next_error == CJSON_TOKEN_ERROR) {
            node->value.error = CJSON_ARRAY_INVALID_VALUE;
        } else {
            node->value.error = next_error;
        }
        break;
    }

    case CJSON_ArrayCommaError: {
        node->value.error = CJSON_ARRAY_MISSING_COMMA_OR_RBRACKET;
        break;
    }
    }

    BENCHMARK_END();

    return false;
}

static bool CJSON_parse_tokens(CJSON_JSON *const node, CJSON_Tokens *const tokens) {
    assert(node != NULL);
    assert(tokens != NULL);

    BENCHMARK_START();

    const CJSON_Token *const token = tokens->data + tokens->index; 

    switch(token->type) {
    case CJSON_TOKEN_STRING: {
        const bool success = CJSON_parse_string(node, tokens);
        BENCHMARK_END();
        return success;
    }
    case CJSON_TOKEN_INT:
    case CJSON_TOKEN_FLOAT:
    case CJSON_TOKEN_SCIENTIFIC_INT: {
        const bool success = CJSON_parse_number(node, tokens);
        BENCHMARK_END();
        return success;
    }
    case CJSON_TOKEN_BOOL: {
        CJSON_parse_bool(node, tokens);
        BENCHMARK_END();
        return true;
    }
    case CJSON_TOKEN_NULL: {
        CJSON_parse_null(node, tokens);
        BENCHMARK_END();
        return true;
    }
    case CJSON_TOKEN_LBRACKET: {
        const bool success = CJSON_parse_array(node, tokens);
        BENCHMARK_END();
        return success;
    }
    case CJSON_TOKEN_LCURLY: {
        const bool success = CJSON_parse_object(node, tokens);
        BENCHMARK_END();
        return success;
    }
    case CJSON_TOKEN_COLON:
    case CJSON_TOKEN_COMMA:
    case CJSON_TOKEN_RBRACKET:
    case CJSON_TOKEN_RCURLY:
    case CJSON_TOKEN_INVALID:
        *node = (CJSON_JSON) {
            .type = CJSON_ERROR,
            .value.error = CJSON_TOKEN_ERROR
        };
    }

    BENCHMARK_END();

    return false;
}

CJSON_JSON *CJSON_init(void) {
    CJSON_JSON *root = CJSON_MALLOC(sizeof(CJSON_JSON));
    assert(root != NULL);

    root->type = CJSON_NULL;
    root->value.null = NULL;

    return root;
}

CJSON_Array *CJSON_make_array(CJSON_JSON *const node) {
    CJSON_internal_free(node);
    CJSON_Array_init(&node->value.array);
    node->type = CJSON_ARRAY;

    return &node->value.array;
}

CJSON_Object *CJSON_make_object(CJSON_JSON *const node) {
    CJSON_internal_free(node);
    CJSON_Object_init(&node->value.object);
    node->type = CJSON_OBJECT;

    return &node->value.object;
}

CJSON_JSON *CJSON_parse(const char *const data, const unsigned int length) {
    assert(data != NULL);
    assert(length > 0);

    BENCHMARK_START();

    CJSON_Lexer lexer;
    CJSON_Lexer_init(&lexer, data, length);

    CJSON_JSON *root = CJSON_MALLOC(sizeof(CJSON_JSON));
    assert(root != NULL);

    CJSON_Tokens tokens;
    CJSON_Tokens_init(&tokens);
    
    CJSON_Token *token;
    do {
        token = CJSON_Tokens_next(&tokens);
    } while(CJSON_Lexer_tokenize(&lexer, token));

    if(token->type == CJSON_TOKEN_INVALID) {
        *root = (CJSON_JSON) {
            .type = CJSON_ERROR,
            .value.error = CJSON_TOKEN_ERROR
        };
        CJSON_Tokens_free(&tokens);
        return root;
    }

    CJSON_parse_tokens(root, &tokens);
    CJSON_Tokens_free(&tokens);
    BENCHMARK_END();

    return root;
}

CJSON_JSON *CJSON_parse_file(const char *const path) {
    assert(path != NULL);
    assert(strlen(path) > 0);

    size_t filesize;
    CJSON_JSON *root;
    char *const data = file_get_contents(path, &filesize);  

    if(data == NULL) {
        root = CJSON_MALLOC(sizeof(CJSON_JSON));
        assert(root != NULL);
        *root = (CJSON_JSON) {
            .type = CJSON_ERROR,
            .value.error = CJSON_FAILED_TO_OPEN_FILE
        };
        return root;
    }

    root = CJSON_parse(data, (unsigned int)filesize);
    assert(root != NULL);
    CJSON_FREE(data);

    return root;
}

void CJSON_free(CJSON_JSON *const json) {
    assert(json != NULL);
    
    CJSON_internal_free(json);
    CJSON_FREE(json);
}

void CJSON_internal_free(CJSON_JSON *const json) {
    assert(json != NULL);

    BENCHMARK_START();

    switch(json->type) {
    case CJSON_OBJECT:
        CJSON_Object_free(&json->value.object);
        break;
    case CJSON_ARRAY:
        CJSON_Array_free(&json->value.array);
        break;
    case CJSON_STRING:
        CJSON_FREE(json->value.string);
        break;
    default:;
    }

    *json = (CJSON_JSON){0};

    BENCHMARK_END();
}

const char *CJSON_get_error(const CJSON_JSON *const node) {
    assert(node != NULL);

    if(node->type != CJSON_ERROR) {
        return NULL;
    }

    switch(node->value.error) {
    case CJSON_TOKEN_ERROR:
        return "Token error";
    case CJSON_STRING_FAILED_TO_PARSE:
        return "String failed to parse";
    case CJSON_FLOAT64_FAILED_TO_PARSE:
        return "Float64 failed to parse";
    case CJSON_INT64_FAILED_TO_PARSE:
        return "Int64 failed to parse";
    case CJSON_UINT64_FAILED_TO_PARSE:
        return "Uint64 failed to parse";
    case CJSON_OBJECT_FAILED_TO_PARSE:
        return "Object failed to parse";
    case CJSON_OBJECT_INVALID_KEY:
        return "Object invalid key";
    case CJSON_OBJECT_INVALID_VALUE:
        return "Object invalid value";
    case CJSON_OBJECT_MISSING_COLON:
        return "Object missing colon";
    case CJSON_OBJECT_MISSING_COMMA_OR_RCURLY:
        return "Object missing comma or right curly bracket";
    case CJSON_ARRAY_FAILED_TO_PARSE:
        return "Array failed to parse";
    case CJSON_ARRAY_MISSING_COMMA_OR_RBRACKET:
        return "Array missing comma or right bracket";
    case CJSON_ARRAY_INVALID_VALUE:
        return "Array invalid value";
    case CJSON_FAILED_TO_OPEN_FILE:
        return "Failed to open file";
    }

    return NULL;
}

CJSON_JSON *CJSON_get(CJSON_JSON *node, const char *query) {
    assert(node != NULL);
    assert(query != NULL);

    BENCHMARK_START();
    
    if(node->type != CJSON_OBJECT && node->type != CJSON_ARRAY) {
        BENCHMARK_END();

        return NULL;
    }

    size_t length = strlen(query);
    assert(length > 0 && length <= UINT_MAX);

    char *key;
    bool is_object_key = query[0] != '[';
    unsigned int i = 0U;

    if(query[0] == '.' || query[0] == '[') {
        length--;
        query++;
    }

    while(i < (unsigned int)length) {
        if(is_object_key && node->type == CJSON_OBJECT) {
            unsigned int counter = 0U;
            while(query[i] != '.' && query[i] != '[' && i < length) {
                i++;
                counter++;
            }

            key = CJSON_MALLOC((size_t)(counter + 1U) * sizeof(char));
            assert(key != NULL);
            memcpy(key, query + i - counter, counter);
            key[counter] = '\0';

            node = CJSON_Object_get(&node->value.object, key);
            CJSON_FREE(key);
            key = NULL;
      
        } else if(!is_object_key && node->type == CJSON_ARRAY) {
            unsigned int counter = 0U;
            while(i < length && query[i] != ']') {
                if(query[i] < '0' || query[i] > '9') {
                    return NULL;
                }
                i++;
                counter++;
            }

            if(query[i] != ']' || counter > UNSIGNED_MAX_LENGTH) {
                BENCHMARK_END();

                return NULL;
            }

            key = CJSON_MALLOC((size_t)(counter + 1U) * sizeof(char));
            assert(key != NULL);
            memcpy(key, query + i - counter, counter);
            key[counter] = '\0';

            bool success;
            uint64_t index = parse_uint64(key, &success);
            if(!success) {
                CJSON_FREE(key);
                BENCHMARK_END();

                return NULL;
            }

            node = CJSON_Array_get(&node->value.array, (unsigned int)index);
            CJSON_FREE(key);
            key = NULL;
            i++;
        } else {
            BENCHMARK_END();
            return NULL;
        }

        if(node == NULL) {
            BENCHMARK_END();
            return NULL;
        }

        is_object_key = query[i++] != '[';
    } 
    
    BENCHMARK_END();
    return node;
}

#define CJSON_GET(JSON_TYPE)\
    assert(node != NULL);\
    assert(node->type != CJSON_ERROR);\
    assert(query != NULL);\
    assert(success != NULL);\
                            \
    BENCHMARK_START();\
    CJSON_JSON *const ret = CJSON_get(node, query);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        BENCHMARK_END();\
        return 0;\
    }\
    *success = true;\

#define CJSON_GET_VALUE(JSON_TYPE, MEMBER)\
    CJSON_GET(JSON_TYPE)\
    BENCHMARK_END();\
    return ret->value.MEMBER;
 
#define CJSON_GET_PTR(JSON_TYPE, MEMBER)\
    CJSON_GET(JSON_TYPE)\
    BENCHMARK_END();\
    return &ret->value.MEMBER;

char *CJSON_get_string(CJSON_JSON *const node, const char *query, bool *const success) {
    CJSON_GET_VALUE(CJSON_STRING, string)
}

double CJSON_get_float64(CJSON_JSON *const node, const char *query, bool *const success) {
    CJSON_GET_VALUE(CJSON_FLOAT64, float64)
}

int64_t CJSON_get_int64(CJSON_JSON *const node, const char *query, bool *const success) {
    CJSON_GET_VALUE(CJSON_INT64, int64)
}

uint64_t CJSON_get_uint64(CJSON_JSON *const node, const char *query, bool *const success) {
    CJSON_GET_VALUE(CJSON_UINT64, uint64)
}

CJSON_Object *CJSON_get_object(CJSON_JSON *const node, const char *query, bool *const success) {
    CJSON_GET_PTR(CJSON_OBJECT, object)
}

CJSON_Array *CJSON_get_array(CJSON_JSON *const node, const char *query, bool *const success) {
    CJSON_GET_PTR(CJSON_ARRAY, array)
}

void *CJSON_get_null(CJSON_JSON *const node, const char *query, bool *const success) {
    CJSON_GET_VALUE(CJSON_NULL, null)
}

bool CJSON_get_bool(CJSON_JSON *const node, const char *query, bool *const success) {
    CJSON_GET_VALUE(CJSON_BOOL, boolean)
}

void CJSON_set_string(CJSON_JSON *const node, const char *const value) {
    assert(node != NULL);

    BENCHMARK_START();

    char *copy = value != NULL ? CJSON_STRDUP(value) : NULL;
    assert(value != NULL && copy != NULL);

    CJSON_internal_free(node);

    *node = (CJSON_JSON) {
        .type = CJSON_STRING,
        .value = {.string = copy} 
    };

    BENCHMARK_END();
}

void CJSON_set_float64(CJSON_JSON *const node, const double value) {
    assert(node != NULL);

    BENCHMARK_START();

    CJSON_internal_free(node);

    *node = (CJSON_JSON) {
        .type = CJSON_FLOAT64,
        .value = {.float64 = value}
    };

    BENCHMARK_END();
}

void CJSON_set_int64(CJSON_JSON *const node, const int64_t value) {
    assert(node != NULL);

    BENCHMARK_START();

    CJSON_internal_free(node);

    *node = (CJSON_JSON) {
        .type = CJSON_INT64,
        .value = {.int64 = value}
    };

    BENCHMARK_END();
}

void CJSON_set_uint64(CJSON_JSON *const node, const uint64_t value) {
    assert(node != NULL);

    BENCHMARK_START();

    CJSON_internal_free(node);

    *node = (CJSON_JSON) {
        .type = CJSON_UINT64,
        .value = {.uint64 = value}
    }; 

    BENCHMARK_END();
}

void CJSON_set_object(CJSON_JSON *const node, const CJSON_Object *const value) {
    assert(node != NULL);
    assert(value != NULL);

    BENCHMARK_START();

    CJSON_internal_free(node);

    *node = (CJSON_JSON) {
        .type = CJSON_OBJECT,
        .value = {.object = *value}
    };

    BENCHMARK_END();
}

void CJSON_set_array(CJSON_JSON *const node, const CJSON_Array *const value) {
    assert(node != NULL);
    assert(value != NULL);

    BENCHMARK_START();

    CJSON_internal_free(node);
    
    *node = (CJSON_JSON) {
        .type = CJSON_ARRAY,
        .value = {.array = *value}
    };

    BENCHMARK_END();
}

void CJSON_set_null(CJSON_JSON *const node) {
    assert(node != NULL);

    BENCHMARK_START();

    CJSON_internal_free(node);

    *node = (CJSON_JSON) {
        .type = CJSON_NULL,
    };

    BENCHMARK_END();
}

void CJSON_set_bool(CJSON_JSON *const node, const bool value) {
    assert(node != NULL);

    BENCHMARK_START();

    CJSON_internal_free(node);

    *node = (CJSON_JSON) {
        .type = CJSON_BOOL,
        .value = {.boolean = value}
    };

    BENCHMARK_END();
}
