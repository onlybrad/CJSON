#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "parser.h"
#include "lexer.h"
#include "tokens.h"
#include "allocator.h"
#include "util.h"

#define UNSIGNED_MAX_LENGTH 10U

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

    json->type               = CJSON_STRING;
    json->value.string.chars = copy;

    return true;
}

EXTERN_C void CJSON_set_float64(struct CJSON *const json, const double value) {
    assert(json != NULL);
    
    json->type          = CJSON_FLOAT64;
    json->value.float64 = value;
}

EXTERN_C void CJSON_set_int64(struct CJSON *const json, const int64_t value) {
    assert(json != NULL);

    json->type        = CJSON_INT64;
    json->value.int64 = value;
}

EXTERN_C void CJSON_set_uint64(struct CJSON *const json, const uint64_t value) {
    assert(json != NULL);

    json->type         = CJSON_UINT64;
    json->value.uint64 = value;
}

EXTERN_C void CJSON_set_object(struct CJSON *const json, const struct CJSON_Object *const value) {
    assert(json != NULL);
    assert(value != NULL);

    if((void*)&json->value == (const void*)value) {
        return;
    }

    json->type         = CJSON_OBJECT;
    json->value.object = *value;
}

EXTERN_C void CJSON_set_array(struct CJSON *const json, const struct CJSON_Array *const value) {
    assert(json != NULL);
    assert(value != NULL);

    if((void*)&json->value == (const void*)value) {
        return;
    }
    
    json->type        = CJSON_ARRAY;
    json->value.array = *value;
}

EXTERN_C void CJSON_set_null(struct CJSON *const json) {
    assert(json != NULL);

    json->type       = CJSON_NULL;
    json->value.null = NULL;
}

EXTERN_C void CJSON_set_bool(struct CJSON *const json, const bool value) {
    assert(json != NULL);

    json->type          = CJSON_BOOL;
    json->value.boolean = value;
}