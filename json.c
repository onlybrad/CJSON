#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <inttypes.h>

#include "json.h"
#include "parser.h"
#include "lexer.h"
#include "tokens.h"
#include "allocator.h"
#include "util.h"
#include "file.h"

#define UNSIGNED_MAX_LENGTH 10U

static unsigned CJSON_JSON_to_string_size(const struct CJSON *const json, const unsigned indentation, const unsigned level);

static char *CJSON_JSON_to_string(const struct CJSON *const json, char *const string, const unsigned indentation, const unsigned level);

static char *CJSON_String_to_string(const struct CJSON_String *const value, char *string) {
    assert(value != NULL);
    assert(string != NULL);

    *(string++) = '"';
    *string     = '\0';
    strcat(string, value->chars);
    string[value->length     ] = '"';
    string[value->length + 1U] = '\0';

    return string + value->length + 1U;
}

static char *CJSON_Array_element_to_string(const struct CJSON *const json, char *string, const unsigned indentation, const unsigned level) {
    assert(json != NULL);
    assert(string != NULL);

    const size_t whitespace_size = (size_t)(indentation * level);

    *(string++) = '\n';
    memset(string, ' ', whitespace_size);
    string += whitespace_size;

    return CJSON_JSON_to_string(json, string, indentation, level + 1U);
}

static char *CJSON_Array_to_string(const struct CJSON_Array *const array, char *string, const unsigned indentation, const unsigned level) {
    assert(array != NULL);
    assert(string != NULL);

    *(string++) = '[';

    if(indentation > 0U) {
        for(unsigned i = 0U; i < array->count - 1U; i++) {
            string = CJSON_Array_element_to_string(array->values + i, string, indentation, level);
            *(string++) = ',';
        }
        string = CJSON_Array_element_to_string(array->values + array->count - 1U, string, indentation, level);
        *(string++) = '\n';
        const size_t whitespace_size = (size_t)(indentation * (level - 1U));
        memset(string, ' ', whitespace_size);
        string += whitespace_size;

    } else {
        for(unsigned i = 0U; i < array->count - 1U; i++) {
            string = CJSON_JSON_to_string(array->values + i, string, 0U, 0U);
            *(string++) = ',';
        }
        string = CJSON_JSON_to_string(array->values + array->count - 1U, string, 0U, 0U);
    }

    *(string++) = ']';
    *string     = '\0';

    return string;
}

static char *CJSON_KV_to_string_with_indentation(const struct CJSON_KV *const key_value, char *string, const unsigned indentation, const unsigned level) {
    assert(key_value != NULL);
    assert(string != NULL);

    const size_t whitespace_size = (size_t)(indentation * level);

    const size_t key_length = strlen(key_value->key);
    assert(key_length < UINT_MAX);

    struct CJSON_String json_string;
    json_string.chars  = key_value->key;
    json_string.length = (unsigned)key_length;

    *(string++) = '\n';
    memset(string, ' ', whitespace_size);
    string     += whitespace_size;
    string      = CJSON_String_to_string(&json_string, string);
    *(string++) = ':';
    *(string++) = ' ';
    
    return CJSON_JSON_to_string(&key_value->value, string, indentation, level + 1U);
}

static char *CJSON_KV_to_string(const struct CJSON_KV *const key_value, char *string) {
    assert(key_value != NULL);
    assert(string != NULL);

    const size_t key_length = strlen(key_value->key);
    assert(key_length < UINT_MAX);

    struct CJSON_String json_string;
    json_string.chars  = key_value->key;
    json_string.length = (unsigned)key_length;

    string      = CJSON_String_to_string(&json_string, string);
    *(string++) = ':';

    return CJSON_JSON_to_string(&key_value->value, string, 0U, 0U);
}

static char *CJSON_Object_to_string(const struct CJSON_Object *const object, char *string, const unsigned indentation, const unsigned level) {
    assert(object != NULL);
    assert(string != NULL);

    const struct CJSON_KV *key_value;

    *(string++) = '{';

    if(indentation > 0U) {
        for(unsigned i = 0U; i < object->capacity - 1U; i++) {
            key_value = object->entries + i;
            if(CJSON_KV_is_used(key_value)) {
                string = CJSON_KV_to_string_with_indentation(key_value, string, indentation, level);
                *(string++) = ',';
            }
        }

        key_value = object->entries + object->capacity - 1U;
        if(CJSON_KV_is_used(key_value)) {
            string = CJSON_KV_to_string_with_indentation(key_value, string, indentation, level);
        } else if(*string == ',') {
            string--;
        }

        *(string++) = '\n';
        const size_t whitespace_size = (size_t)(indentation * (level - 1U));
        memset(string, ' ', whitespace_size);
        string += whitespace_size;

    } else {
        for(unsigned i = 0U; i < object->capacity - 1U; i++) {
            key_value = object->entries + i;
            if(CJSON_KV_is_used(key_value)) {
                string = CJSON_KV_to_string(key_value, string);
                *(string++) = ',';
            }
        }
        key_value = object->entries + object->capacity - 1U;
        if(CJSON_KV_is_used(key_value)) {
            string = CJSON_KV_to_string(key_value, string);
        } else if(*string == ',') {
            string--;
        }
    }

    *(string++) = '}';
    *string     = '\0';

    return string;
}

static char *CJSON_JSON_to_string(const struct CJSON *const json, char *const string, const unsigned indentation, const unsigned level) {
    assert(json != NULL);
    assert(string != NULL);

    switch(json->type) {
    case CJSON_STRING:
        return CJSON_String_to_string(&json->value.string, string);

    case CJSON_FLOAT64: {
        const int count = sprintf(string, "%.*g", DBL_PRECISION, json->value.float64); 
        assert(count > 0);

        return string + count;
    }

    case CJSON_INT64: {
        const int count = sprintf(string, "%" PRIi64, json->value.int64); 
        assert(count > 0);

        return string + count;
    }

    case CJSON_UINT64: {
        const int count = sprintf(string, "%" PRIu64, json->value.uint64);
        assert(count > 0);

        return string + count;
    }

    case CJSON_ARRAY:
        return CJSON_Array_to_string(&json->value.array, string, indentation, level);

    case CJSON_OBJECT:
        return CJSON_Object_to_string(&json->value.object, string, indentation, level);

    case CJSON_NULL:
        *string = '\0';
        strcat(string, "null");
        return string + static_strlen("null");

    case CJSON_BOOL:
        *string = '\0';
        if(json->value.boolean) {
            strcat(string, "true");
            return string + static_strlen("true");
        }
        strcat(string, "false");
        return string + static_strlen("false");
    }

    return NULL;
}

static unsigned CJSON_Array_to_string_size(const struct CJSON_Array *const array, const unsigned indentation, const unsigned level) {
    assert(array != NULL);

    unsigned size = (unsigned)(static_strlen("[") + static_strlen("]"));

    if(array->count > 0U) {
        size += (array->count - 1U) * (unsigned)static_strlen(",");
    }

    if(indentation > 0U) {
        const unsigned whitespace_size = indentation * level;
        size += ((unsigned)static_strlen("\n") + whitespace_size) * array->count + (unsigned)static_strlen("\n") + whitespace_size - indentation;
    }

    for(unsigned i = 0U; i < array->count; i++) {
        size += CJSON_JSON_to_string_size(array->values + i, indentation, level + 1U);
    }

    return size;
}

static unsigned CJSON_Object_to_string_size(const struct CJSON_Object *const object, const unsigned indentation, const unsigned level) {
    assert(object != NULL);

    unsigned entry_count = 0U;
    unsigned size = (unsigned)(static_strlen("{") + static_strlen("}"));

    for(unsigned i = 0U; i < object->capacity; i++) {
        const struct CJSON_KV *const key_value = object->entries + i;
        if(CJSON_KV_is_used(key_value)) {
            const size_t key_length = strlen(key_value->key);
            assert(key_length < UINT_MAX);

            size += (unsigned)(static_strlen("\"") + key_length + static_strlen("\""));
            size += CJSON_JSON_to_string_size(&object->entries[i].value, indentation, level + 1U);

            entry_count++;
        }
    }

    if(entry_count > 0U) {
        const unsigned colon_size = indentation > 0U
            ? (unsigned)(static_strlen(": "))
            : (unsigned)(static_strlen(":"));

        size += (entry_count - 1U) * (unsigned)(static_strlen(","));
        size += entry_count        * colon_size;
    }

    if(indentation > 0U) {
        const unsigned whitespace_size = indentation * level;
        size += ((unsigned)static_strlen("\n") + whitespace_size) * entry_count + (unsigned)static_strlen("\n") + whitespace_size - indentation;
    }

    return size;
}

static unsigned CJSON_JSON_to_string_size(const struct CJSON *const json, const unsigned indentation, const unsigned level) {
    assert(json != NULL);

    switch(json->type) {
    case CJSON_STRING:
        return static_strlen("\"") + json->value.string.length + static_strlen("\"");

    case CJSON_FLOAT64: {
        return (unsigned)snprintf(NULL, 0, "%.*g", DBL_PRECISION, json->value.float64); 
    }

    case CJSON_INT64: {
        return (unsigned)snprintf(NULL, 0, "%" PRIi64, json->value.int64); 
    }

    case CJSON_UINT64: {
        return (unsigned)snprintf(NULL, 0, "%" PRIu64, json->value.uint64); 
    }

    case CJSON_ARRAY:
        return CJSON_Array_to_string_size(&json->value.array, indentation, level);
    
    case CJSON_OBJECT:
        return CJSON_Object_to_string_size(&json->value.object, indentation, level);

    case CJSON_NULL:
        return 4U;

    case CJSON_BOOL:
        return (json->value.boolean 
            ? (unsigned)sizeof("true") 
            : (unsigned)sizeof("false")
        ) - 1U;
    }

    return 0U;
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
            uint64_t index = CJSON_parse_uint64(key, &success);
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

EXTERN_C const char *CJSON_get_string(struct CJSON *const json, const char *const query, bool *const success) {
    assert(json != NULL);
    assert(query != NULL);
    assert(success != NULL);
                            
    struct CJSON *const ret = CJSON_get(json, query);
    if(ret == NULL) {
        *success = false;
        return 0;
    }

    return CJSON_as_string(ret, success);
}

EXTERN_C double CJSON_get_float64(struct CJSON *const json, const char *const query, bool *const success) {
    assert(json != NULL);
    assert(query != NULL);
    assert(success != NULL);
                            
    struct CJSON *const ret = CJSON_get(json, query);
    if(ret == NULL) {
        *success = false;
        return 0;
    }

    return CJSON_as_float64(ret, success);
}

EXTERN_C int64_t CJSON_get_int64(struct CJSON *const json, const char *const query, bool *const success) {
    assert(json != NULL);
    assert(query != NULL);
    assert(success != NULL);
                            
    struct CJSON *const ret = CJSON_get(json, query);
    if(ret == NULL) {
        *success = false;
        return 0;
    }

    return CJSON_as_int64(ret, success);
}

EXTERN_C uint64_t CJSON_get_uint64(struct CJSON *const json, const char *const query, bool *const success) {
    assert(json != NULL);
    assert(query != NULL);
    assert(success != NULL);
                            
    struct CJSON *const ret = CJSON_get(json, query);
    if(ret == NULL) {
        *success = false;
        return 0;
    }

    return CJSON_as_uint64(ret, success);
}

EXTERN_C struct CJSON_Object *CJSON_get_object(struct CJSON *const json, const char *const query, bool *const success) {
    assert(json != NULL);
    assert(query != NULL);
    assert(success != NULL);
                            
    struct CJSON *const ret = CJSON_get(json, query);
    if(ret == NULL) {
        *success = false;
        return 0;
    }

    return CJSON_as_object(ret, success);
}

EXTERN_C struct CJSON_Array *CJSON_get_array(struct CJSON *const json, const char *const query, bool *const success) {
    assert(json != NULL);
    assert(query != NULL);
    assert(success != NULL);
                            
    struct CJSON *const ret = CJSON_get(json, query);
    if(ret == NULL) {
        *success = false;
        return 0;
    }

    return CJSON_as_array(ret, success);
}

EXTERN_C void *CJSON_get_null(struct CJSON *const json, const char *const query, bool *const success) {
    assert(json != NULL);
    assert(query != NULL);
    assert(success != NULL);
                            
    struct CJSON *const ret = CJSON_get(json, query);
    if(ret == NULL) {
        *success = false;
        return 0;
    }

    return CJSON_as_null(ret, success);
}

EXTERN_C bool CJSON_get_bool(struct CJSON *const json, const char *const query, bool *const success) {
    assert(json != NULL);
    assert(query != NULL);
    assert(success != NULL);
                            
    struct CJSON *const ret = CJSON_get(json, query);
    if(ret == NULL) {
        *success = false;
        return 0;
    }

    return CJSON_as_bool(ret, success);
}

EXTERN_C const char *CJSON_as_string(struct CJSON *const json, bool *success) {
    assert(json != NULL);
    assert(success != NULL);

    if(json->type != CJSON_STRING) {
        *success = false;
        return NULL;
    }
    *success = true;
    return json->value.string.chars;
}

double CJSON_as_float64(struct CJSON *const json, bool *success) {
    assert(json != NULL);
    assert(success != NULL);

    switch(json->type) {
    case CJSON_INT64:
        *success = true;
        return (double)json->value.int64;

    case CJSON_UINT64:
        *success = true;
        return (double)json->value.uint64;

    case CJSON_FLOAT64:
        *success = true;
        return json->value.float64;

    default:
        *success = false;
        return 0.0;
    }
}

int64_t CJSON_as_int64(struct CJSON *const json, bool *success) {
    switch(json->type) {
    case CJSON_INT64:
        *success = true;
        return json->value.int64;

    case CJSON_UINT64:
        if(json->value.uint64 > (uint64_t)INT64_MAX) {
            *success = false;
            return 0;
        }
        *success = true;
        return (int64_t)json->value.uint64;

    case CJSON_FLOAT64:
        if(json->value.float64 < (double)INT64_MIN || json->value.float64 > (double)INT64_MAX) {
            *success = false;
            return 0;
        }
        *success = true;
        return (int64_t)json->value.float64;

    default:
        *success = false;
        return 0.0;
    }
}

uint64_t CJSON_as_uint64(struct CJSON *const json, bool *success) {
    switch(json->type) {
    case CJSON_INT64:
        if(json->value.int64 < 0) {
            *success = false;
            return 0;
        }
        *success = true;
        return (uint64_t)json->value.int64;

    case CJSON_UINT64:
        *success = true;
        return json->value.uint64;

    case CJSON_FLOAT64:
        if(json->value.float64 < 0.0 || json->value.float64 > (double)UINT64_MAX) {
            *success = false;
            return 0;
        }
        *success = true;
        return (uint64_t)json->value.float64;

    default:
        *success = false;
        return 0.0;
    }
}

struct CJSON_Object *CJSON_as_object(struct CJSON *const json, bool *success) {
    assert(json != NULL);
    assert(success != NULL);

    if(json->type != CJSON_OBJECT) {
        *success = false;
        return NULL;
    }
    *success = true;
    return &json->value.object;
}

struct CJSON_Array  *CJSON_as_array(struct CJSON *const json, bool *success) {
    assert(json != NULL);
    assert(success != NULL);

    if(json->type != CJSON_ARRAY) {
        *success = false;
        return NULL;
    }
    *success = true;
    return &json->value.array;
}

void *CJSON_as_null(struct CJSON *const json, bool *success) {
    assert(json != NULL);
    assert(success != NULL);

    *success = json->type == CJSON_NULL;
    return NULL;
}

bool CJSON_as_bool(struct CJSON *const json, bool *success) {
    assert(json != NULL);
    assert(success != NULL);

    if(json->type != CJSON_BOOL) {
        *success = false;
        return NULL;
    }
    *success = true;
    return json->value.boolean;
}

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

EXTERN_C unsigned CJSON_to_string_size(const struct CJSON *const json, const unsigned indentation) {
    assert(json != NULL);

    return CJSON_JSON_to_string_size(json, indentation, 1U);
}

EXTERN_C char *CJSON_to_string(const struct CJSON *const json, const unsigned indentation) { 
    assert(json != NULL);

    const unsigned total_size = CJSON_to_string_size(json, indentation);
    assert(total_size > 0U);

    char *const string = (char*)CJSON_MALLOC(((size_t)total_size + 1) * sizeof(char));
    if(string == NULL) {
        return NULL;
    }

    const char *const end = CJSON_JSON_to_string(json, string, indentation, 1U);

    assert(end - string == (unsigned)total_size);
    (void)end;

    return string;
}

bool CJSON_to_file(const struct CJSON *const json, const char *const path, const unsigned indentation) {
    assert(json != NULL);
    assert(path != NULL);
    assert(path[0] != '\0');

    char *const string = CJSON_to_string(json, indentation);
    if(string == NULL) {
        return false;
    }

    struct CJSON_FileContents file_contents;
    file_contents.data = (unsigned char*)string;
    file_contents.size = 0U;

    if(CJSON_FileContents_put(&file_contents, path) != CJSON_FILECONTENTS_ERROR_NONE) {
        CJSON_FREE(string);
        return false;
    }

    CJSON_FREE(string);
    return true;
}
