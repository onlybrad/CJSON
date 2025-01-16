#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "lexer.h"
#include "tokens.h"
#include "util.h"
#include "allocator.h"

//this is the same thing as JSON but contains the tokens. This is the actual type JSON_parse returns. For convienance, JSON_parse returns a JSON*.
typedef struct JSON_Root {
    JSON json;
    JSON_Tokens tokens;
} JSON_Root;

typedef enum ObjectParsingError {
    ObjectKeyError,
    ObjectColonError,
    ObjectValueError,
    ObjectCommaError,
    ObjectIncompleteError,
} ObjectParsingError;

typedef enum ArrayParsingError {
    ArrayValueError,
    ArrayCommaError,
    ArrayIncompleteError
} ArrayParsingError;
 
static bool parse_tokens(JSON *const json, JSON_Tokens *const tokens);

static char *parse_utf8_string(const JSON_Token *const token) {
    assert(token != NULL);

    char* string = CJSON_MALLOC((token->length - 1) * sizeof(char));
    assert(string != NULL);

    unsigned int str_index = 0U;
    unsigned int tok_index = 1U;
    const unsigned int length = token->length - 1U;
    const char *const value = token->value;
    bool escaping = false;

    while(tok_index < length) {
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
            uint16_t codepoint = parse_codepoint(value + tok_index, &success);
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

            uint16_t low = parse_codepoint(value + tok_index + 6U, &success);
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

static bool parse_string(JSON *const json, JSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);
    
    const JSON_Token *const token = tokens->data + tokens->index; 

    char *string = parse_utf8_string(token);
    if(string == NULL) {
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error =  JSON_STRING_FAILED_TO_PARSE
        };
        return false;
    }

    *json = (JSON) {
        .type = JSON_STRING,
        .value.string = string
    };

    tokens->index++;

    return true;
}

static bool parse_number(JSON *const json, JSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);
    char str[1 << 9] = {0};

    bool success;
    const JSON_Token *const token = tokens->data + tokens->index; 

    memcpy(str, token->value, MIN(sizeof(str) - 1, token->length));

    if(token->type == JSON_TOKEN_FLOAT) {
        const double float64 = parse_float64(str, &success);
        if(!success) {
            *json = (JSON) {
                .type = JSON_ERROR,
                .value.error = JSON_FLOAT64_FAILED_TO_PARSE
            };
            return false;
        }

        *json = (JSON) {
            .type = JSON_FLOAT64,
            .value.float64 = float64
        };
    } else if(str[0] == '-') {
        if(token->type == JSON_TOKEN_SCIENTIFIC_INT) {
            const long double long_double = parse_long_double(str, &success);
            if(!success || long_double < INT64_MIN || long_double > INT64_MAX) {
                *json = (JSON) {
                    .type = JSON_ERROR,
                    .value.error = JSON_INT64_FAILED_TO_PARSE
                };
                return false;
            }

            *json = (JSON) {
                .type = JSON_INT64,
                .value.int64 = (int64_t)long_double,
            };
        } else {
            const int64_t int64 = parse_int64(str, &success);
            if(!success) {
                *json = (JSON) {
                    .type = JSON_ERROR,
                    .value.error = JSON_INT64_FAILED_TO_PARSE
                };
                return false;     
            }

            *json = (JSON) {
                .type = JSON_INT64,
                .value.int64 = int64,
            };
        }
    } else if(token->type == JSON_TOKEN_SCIENTIFIC_INT) {
        const long double long_double = parse_long_double(str, &success);
        if(!success || long_double > UINT64_MAX) {
            *json = (JSON) {
                .type = JSON_ERROR,
                .value.error = JSON_UINT64_FAILED_TO_PARSE
            };
            return false;
        }

        *json = (JSON) {
            .type = JSON_UINT64,
            .value.uint64 = (uint64_t)long_double,
        };
    } else {
        const uint64_t uint64 = parse_uint64(str, &success);
        if(!success) {
            *json = (JSON) {
                .type = JSON_ERROR,
                .value.error = JSON_UINT64_FAILED_TO_PARSE
            };
            return false; 
        }

        *json = (JSON) {
            .type = JSON_UINT64,
            .value.uint64 = uint64,
        };
    }

    tokens->index++;
    
    return true;
}

static void parse_bool(JSON *const json, JSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    const JSON_Token *const token = tokens->data + tokens->index; 

    json->type = JSON_BOOL;
    json->value.boolean = token->value[0] == 't';

    tokens->index++;
}

static void parse_null(JSON *const json, JSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    json->type = JSON_NULL;
    json->value.null = NULL;

    tokens->index++;
}

static bool parse_object(JSON *const json, JSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    tokens->index++;

    if(tokens->length == tokens->index) {
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_OBJECT_FAILED_TO_PARSE
        };

        return false;
    }

    JSON_Object *const object = &json->value.object;
    json->type = JSON_OBJECT;
    JSON_Object_init(object);

    ObjectParsingError error;
    JSON json_copy;
    const JSON_Token *token = tokens->data + tokens->index;
    char *key = NULL;

    if(token->type == JSON_TOKEN_RCURLY) {
        tokens->index++;
        return true;
    }

    while(tokens->index + 4U < tokens->length) {
        if(token->type != JSON_TOKEN_STRING) {
            error = ObjectKeyError;
            goto cleanup;
        }

        key = parse_utf8_string(token);
        if(key == NULL) {
            error = ObjectKeyError;
            goto cleanup;
        }

        token++;
    
        if(token->type != JSON_TOKEN_COLON) {
            error = ObjectColonError;
            tokens->index++;
            goto cleanup;
        }

        tokens->index += 2U;

        JSON_Key_Value *const entry = JSON_Object_get_entry(object, key);
        if(entry->key != NULL) {
            CJSON_FREE(key);
            _JSON_free(&entry->value);
        } else {
            entry->key = key;
        }
        key = NULL;

        if(!parse_tokens(&entry->value, tokens)) {
            error = ObjectValueError;
            json_copy = entry->value;
            goto cleanup;
        }

        token = tokens->data + tokens->index;

        if(token->type == JSON_TOKEN_COMMA) {
            tokens->index++;
            token++;
            continue;
        }

        if(token->type == JSON_TOKEN_RCURLY) {
            tokens->index++;
            return true;
        }

        error = ObjectCommaError;
        goto cleanup;
    }
    
    error = ObjectIncompleteError;

cleanup:
    CJSON_FREE(key);
    JSON_Object_free(object);

    switch(error) {
    case ObjectIncompleteError: {
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_OBJECT_FAILED_TO_PARSE
        };
        break;
    }
    case ObjectKeyError: {
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_OBJECT_INVALID_KEY
        };
        break;
    }

    case ObjectColonError: {
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_OBJECT_MISSING_COLON
        };
        break;
    }

    case ObjectValueError: {
        if(json_copy.value.error == JSON_TOKEN_ERROR) {
            *json = (JSON) {
                .type = JSON_ERROR,
                .value.error = JSON_OBJECT_INVALID_VALUE
            };
        } else {
            *json = json_copy;
        }
        break;
    }

    case ObjectCommaError: {
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_OBJECT_MISSING_COMMA_OR_RCURLY
        };
        break;
    }
    }

    return false;
}

static bool parse_array(JSON *const json, JSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    tokens->index++;

    if(tokens->length == tokens->index) {
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_ARRAY_FAILED_TO_PARSE
        };
        return false;
    }

    JSON json_copy;
    JSON_Array *const array = &json->value.array;
    json->type = JSON_ARRAY;
    JSON_Array_init(array);

    const JSON_Token *token = tokens->data + tokens->index;
    ArrayParsingError error;

    if(token->type == JSON_TOKEN_RBRACKET) {
        tokens->index++;
        return true;
    }

    while(tokens->index + 2U < tokens->length) {
        JSON *const next_json = JSON_Array_next(array);
        if(!parse_tokens(next_json, tokens)) {
            error = ArrayValueError;
            json_copy = *next_json;
            goto cleanup;
        }

        token = tokens->data + tokens->index;
 
        if(token->type == JSON_TOKEN_COMMA) {
            tokens->index++;
            token++;
            continue;
        }

        if(token->type == JSON_TOKEN_RBRACKET) {
            tokens->index++;
            return true;
        }

        error = ArrayCommaError;
        goto cleanup;
    }

    error = ArrayIncompleteError;

cleanup:
    JSON_Array_free(array);

    switch(error) {
    case ArrayIncompleteError: {
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_ARRAY_FAILED_TO_PARSE
        };
        break;
    }

    case ArrayValueError: {
        if(json_copy.value.error == JSON_TOKEN_ERROR) {
            *json = (JSON) {
                .type = JSON_ERROR,
                .value.error = JSON_ARRAY_INVALID_VALUE
            };
        } else {
            *json = json_copy;
        }
        break;
    }

    case ArrayCommaError: {
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_ARRAY_MISSING_COMMA_OR_RBRACKET
        };
        break;
    }
    }

    return false;
}

static bool parse_tokens(JSON *const json, JSON_Tokens *const tokens) {
    assert(json != NULL);
    assert(tokens != NULL);

    const JSON_Token *const token = tokens->data + tokens->index; 

    switch(token->type) {
    case JSON_TOKEN_STRING:
        return parse_string(json, tokens);
    case JSON_TOKEN_INT:
    case JSON_TOKEN_FLOAT:
    case JSON_TOKEN_SCIENTIFIC_INT:
        return parse_number(json, tokens);
    case JSON_TOKEN_BOOL:
        parse_bool(json, tokens);
        return true;
    case JSON_TOKEN_NULL:
        parse_null(json, tokens);
        return true;
    case JSON_TOKEN_LBRACKET:
        return parse_array(json, tokens);
    case JSON_TOKEN_LCURLY:
        return parse_object(json, tokens);
    case JSON_TOKEN_COLON:
    case JSON_TOKEN_COMMA:
    case JSON_TOKEN_RBRACKET:
    case JSON_TOKEN_RCURLY:
    case JSON_TOKEN_INVALID:
        *json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_TOKEN_ERROR
        };
    }

    return false;
}

JSON *JSON_init(void) {
    JSON_Root *root = CJSON_MALLOC(sizeof(JSON_Root));
    assert(root != NULL);

    root->json.type = JSON_NULL;
    root->json.value.null = NULL;
    root->tokens = (JSON_Tokens){0};

    return (JSON*)root;
}

JSON_Array *JSON_make_array(JSON *const json) {
    _JSON_free(json);
    JSON_Array_init(&json->value.array);
    json->type = JSON_ARRAY;

    return &json->value.array;
}

JSON_Object *JSON_make_object(JSON *const json) {
    _JSON_free(json);
    JSON_Object_init(&json->value.object);
    json->type = JSON_OBJECT;

    return &json->value.object;
}

JSON *JSON_parse(const char *const data, const unsigned int length) {
    assert(data != NULL);
    assert(length > 0);

    JSON_Lexer lexer;
    JSON_Lexer_init(&lexer, data, length);

    JSON_Root *root = CJSON_MALLOC(sizeof(JSON_Root));
    assert(root != NULL);

    JSON_Tokens *tokens = &root->tokens;
    JSON_Tokens_init(tokens);

    JSON_Token *token = JSON_Tokens_next(tokens);
    while(JSON_Lexer_tokenize(&lexer, token)) {
        token = JSON_Tokens_next(tokens);
    }

    if(token->type == JSON_TOKEN_INVALID) {
        root->json = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_TOKEN_ERROR
        };
        return (JSON*)root;
    }

    parse_tokens(&root->json, tokens);

    return (JSON*)root;
}

JSON *JSON_parse_file(const char *const path) {
    assert(path != NULL);
    assert(strlen(path) > 0);

    size_t filesize;
    char *const data = file_get_contents(path, &filesize);
    if(data == NULL) {
        JSON *const ret = malloc(sizeof(JSON));
        *ret = (JSON) {
            .type = JSON_ERROR,
            .value.error = JSON_FAILED_TO_OPEN_FILE
        };
        return ret;
    }

    JSON *const json = JSON_parse(data, (unsigned int)filesize);

    CJSON_FREE(data);

    return json;
}

void JSON_free(JSON *const json) {
    assert(json != NULL);

    JSON_Root *root = (JSON_Root*)json;
    if(root->tokens.data != NULL) {
        JSON_Tokens_free(&root->tokens);
    }
    _JSON_free(json);
    *root = (JSON_Root){0};
    CJSON_FREE(root);
}

void _JSON_free(JSON *const json) {
    assert(json != NULL);

    switch(json->type) {
    case JSON_OBJECT:
        JSON_Object_free(&json->value.object);
        break;
    case JSON_ARRAY:
        JSON_Array_free(&json->value.array);
        break;
    case JSON_STRING:
        CJSON_FREE(json->value.string);
        break;
    default:;
    }

    *json = (JSON){0};
}

const char *JSON_get_error(const JSON *const json) {
    assert(json != NULL);

    if(json->type != JSON_ERROR) {
        return NULL;
    }

    switch(json->value.error) {
    case JSON_TOKEN_ERROR:
        return "Token error";
    case JSON_STRING_FAILED_TO_PARSE:
        return "String failed to parse";
    case JSON_FLOAT64_FAILED_TO_PARSE:
        return "Float64 failed to parse";
    case JSON_INT64_FAILED_TO_PARSE:
        return "Int64 failed to parse";
    case JSON_UINT64_FAILED_TO_PARSE:
        return "Uint64 failed to parse";
    case JSON_OBJECT_FAILED_TO_PARSE:
        return "Object failed to parse";
    case JSON_OBJECT_INVALID_KEY:
        return "Object invalid key";
    case JSON_OBJECT_INVALID_VALUE:
        return "Object invalid value";
    case JSON_OBJECT_MISSING_COLON:
        return "Object missing colon";
    case JSON_OBJECT_MISSING_COMMA_OR_RCURLY:
        return "Object missing comma or right curly bracket";
    case JSON_ARRAY_FAILED_TO_PARSE:
        return "Array failed to parse";
    case JSON_ARRAY_MISSING_COMMA_OR_RBRACKET:
        return "Array missing comma or right bracket";
    case JSON_ARRAY_INVALID_VALUE:
        return "Array invalid value";
    case JSON_FAILED_TO_OPEN_FILE:
        return "Failed to open file";
    }

    return NULL;
}

JSON *JSON_get(JSON *json, const char *query) {
    assert(json != NULL);
    assert(query != NULL);
    
    if(json->type != JSON_OBJECT && json->type != JSON_ARRAY) {
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
        if(is_object_key && json->type == JSON_OBJECT) {
            unsigned int counter = 0U;
            while(query[i] != '.' && query[i] != '[' && i < length) {
                i++;
                counter++;
            }

            key = CJSON_MALLOC((size_t)(counter + 1U) * sizeof(char));
            assert(key != NULL);
            memcpy(key, query + i - counter, counter);
            key[counter] = '\0';

            json = JSON_Object_get(&json->value.object, key);
            CJSON_FREE(key);
            key = NULL;
      
        } else if(!is_object_key && json->type == JSON_ARRAY) {
            unsigned int counter = 0U;
            while(i < length && query[i] != ']') {
                if(query[i] < '0' || query[i] > '9') {
                    return NULL;
                }
                i++;
                counter++;
            }

            if(query[i] != ']' || counter > UNSIGNED_MAX_LENGTH) {
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
                return NULL;
            }

            json = JSON_Array_get(&json->value.array, (unsigned int)index);
            CJSON_FREE(key);
            key = NULL;
            i++;
        } else {
            return NULL;
        }

        if(json == NULL) {
            return NULL;
        }

        is_object_key = query[i++] != '[';
    } 
    
    return json;
}

#define JSON_GET(JSON_TYPE)\
    assert(json != NULL);\
    assert(json->type != JSON_ERROR);\
    assert(query != NULL);\
    assert(success != NULL);\
                            \
    JSON *const ret = JSON_get(json, query);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        return 0;\
    }\
    *success = true;\

#define JSON_GET_VALUE(JSON_TYPE, MEMBER)\
    JSON_GET(JSON_TYPE)\
    return ret->value.MEMBER;

#define JSON_GET_PTR(JSON_TYPE, MEMBER)\
    JSON_GET(JSON_TYPE)\
    return &ret->value.MEMBER;

char *JSON_get_string(JSON *const json, const char *query, bool *const success) {
    JSON_GET_VALUE(JSON_STRING, string)
}

double JSON_get_float64(JSON *const json, const char *query, bool *const success) {
    JSON_GET_VALUE(JSON_FLOAT64, float64)
}

int64_t JSON_get_int64(JSON *const json, const char *query, bool *const success) {
    JSON_GET_VALUE(JSON_INT64, int64)
}

uint64_t JSON_get_uint64(JSON *const json, const char *query, bool *const success) {
    JSON_GET_VALUE(JSON_UINT64, uint64)
}

JSON_Object *JSON_get_object(JSON *const json, const char *query, bool *const success) {
    JSON_GET_PTR(JSON_OBJECT, object)
}

JSON_Array *JSON_get_array(JSON *const json, const char *query, bool *const success) {
    JSON_GET_PTR(JSON_ARRAY, array)
}

void *JSON_get_null(JSON *const json, const char *query, bool *const success) {
    JSON_GET_VALUE(JSON_NULL, null)
}

bool JSON_get_bool(JSON *const json, const char *query, bool *const success) {
    JSON_GET_VALUE(JSON_BOOL, boolean)
}

void JSON_set_string(JSON *const json, const char *const value) {
    assert(json != NULL);

    char *copy = value != NULL ? CJSON_STRDUP(value) : NULL;
    assert(value != NULL && copy != NULL);

    _JSON_free(json);

    *json = (JSON) {
        .type = JSON_STRING,
        .value = {.string = copy} 
    };
}

void JSON_set_float64(JSON *const json, const double value) {
    assert(json != NULL);

    _JSON_free(json);

    *json = (JSON) {
        .type = JSON_FLOAT64,
        .value = {.float64 = value}
    };
}

void JSON_set_int64(JSON *const json, const int64_t value) {
    assert(json != NULL);

    _JSON_free(json);

    *json = (JSON) {
        .type = JSON_INT64,
        .value = {.int64 = value}
    };
}

void JSON_set_uint64(JSON *const json, const uint64_t value) {
    assert(json != NULL);

    _JSON_free(json);

    *json = (JSON) {
        .type = JSON_UINT64,
        .value = {.uint64 = value}
    }; 
}

void JSON_set_object(JSON *const json, const JSON_Object *const value) {
    assert(json != NULL);
    assert(value != NULL);

    _JSON_free(json);

    *json = (JSON) {
        .type = JSON_OBJECT,
        .value = {.object = *value}
    }; 
}

void JSON_set_array(JSON *const json, const JSON_Array *const value) {
    assert(json != NULL);
    assert(value != NULL);

    _JSON_free(json);
    
    *json = (JSON) {
        .type = JSON_ARRAY,
        .value = {.array = *value}
    }; 
}

void JSON_set_null(JSON *const json) {
    assert(json != NULL);

    _JSON_free(json);

    *json = (JSON) {
        .type = JSON_NULL,
    }; 
}

void JSON_set_bool(JSON *const json, const bool value) {
    assert(json != NULL);

    _JSON_free(json);

    *json = (JSON) {
        .type = JSON_BOOL,
        .value = {.boolean = value}
    };
}