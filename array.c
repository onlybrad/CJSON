#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "cjson.h"
#include "util.h"

EXTERN_C void CJSON_Array_init(struct CJSON_Array *const array) {
    assert(array != NULL);

    array->values   = NULL;
    array->count    = 0U;
    array->capacity = 0U;
}

EXTERN_C bool CJSON_Array_reserve(struct CJSON_Array *const array, struct CJSON_Parser *const parser, unsigned capacity) {
    assert(array != NULL);
    assert(parser != NULL);

    if(capacity < CJSON_ARRAY_MINIMUM_CAPACITY) {
        capacity = CJSON_ARRAY_MINIMUM_CAPACITY;
    }

    if(capacity <= array->capacity) {
        return true;
    }

    struct CJSON *values = CJSON_ARENA_ALLOC(&parser->array_arena, capacity, struct CJSON);
    if(values == NULL) {
        return false;
    }

    array->values   = values;
    array->capacity = capacity;

    return true;
}

EXTERN_C struct CJSON *CJSON_Array_next(struct CJSON_Array *const array, struct CJSON_Parser *const parser) {
    assert(array != NULL);
    assert(parser != NULL);

    if(array->count == array->capacity) {
        if(!CJSON_Array_reserve(array, parser, array->capacity * 2)) {
            return NULL;
        }
    }

    struct CJSON *const ret = array->values + array->count;
    array->count++;

    return ret;
}

EXTERN_C struct CJSON *CJSON_Array_get(const struct CJSON_Array *const array, const unsigned index) {
    assert(array != NULL);

    return index >= array->count ? NULL : array->values + index;
}

EXTERN_C bool CJSON_Array_set(struct CJSON_Array *const array, struct CJSON_Parser *const parser, const unsigned index, const struct CJSON *const value) {
    assert(array != NULL);
    assert(parser != NULL);
    assert(value != NULL);

    if(index >= array->capacity) {
        unsigned capacity = array->capacity == 0 ? CJSON_ARRAY_MINIMUM_CAPACITY : array->capacity;
        while(index >= capacity) {
            capacity *= 2U;
        }

        if(!CJSON_Array_reserve(array, parser, capacity)) {
            return false;
        }
    } else if(index >= array->count) {
        array->count = index + 1U;
    }

    array->values[index] = *value;
    return true;
}

EXTERN_C bool CJSON_Array_push(struct CJSON_Array *const array, struct CJSON_Parser *const parser,  const struct CJSON *const value) {
    assert(array != NULL);
    assert(parser != NULL);
    assert(value != NULL);

    if(array->count == UINT_MAX) {
        return false;
    }

    struct CJSON *const next = CJSON_Array_next(array, parser);
    if(next == NULL) {
        return false;
    }

    *next = *value;

    return true;
}

#define CJSON_GET_TYPE        CJSON_STRING
#define CJSON_GET_MEMBER      string.chars
#define CJSON_GET_SUFFIX      string
#define CJSON_GET_RETURN_TYPE const char*
#include "cjson-array-get-template.h"

#define CJSON_GET_TYPE        CJSON_FLOAT64
#define CJSON_GET_MEMBER      float64
#define CJSON_GET_SUFFIX      float64
#define CJSON_GET_RETURN_TYPE double
#include "cjson-array-get-template.h"

#define CJSON_GET_TYPE        CJSON_INT64
#define CJSON_GET_MEMBER      int64
#define CJSON_GET_SUFFIX      int64
#define CJSON_GET_RETURN_TYPE int64_t
#include "cjson-array-get-template.h"

#define CJSON_GET_TYPE        CJSON_UINT64
#define CJSON_GET_MEMBER      uint64
#define CJSON_GET_SUFFIX      uint64
#define CJSON_GET_RETURN_TYPE uint64_t
#include "cjson-array-get-template.h"

#define CJSON_GET_TYPE        CJSON_OBJECT
#define CJSON_GET_MEMBER      object
#define CJSON_GET_SUFFIX      object
#define CJSON_GET_RETURN_TYPE struct CJSON_Object*
#define CJSON_GET_RETURN_PTR
#include "cjson-array-get-template.h"

#define CJSON_GET_TYPE        CJSON_ARRAY
#define CJSON_GET_MEMBER      array
#define CJSON_GET_SUFFIX      array
#define CJSON_GET_RETURN_TYPE struct CJSON_Array*
#define CJSON_GET_RETURN_PTR
#include "cjson-array-get-template.h"

#define CJSON_GET_TYPE        CJSON_NULL
#define CJSON_GET_MEMBER      null
#define CJSON_GET_SUFFIX      null
#define CJSON_GET_RETURN_TYPE void*
#include "cjson-array-get-template.h"

#define CJSON_GET_TYPE        CJSON_BOOL
#define CJSON_GET_MEMBER      boolean
#define CJSON_GET_RETURN_TYPE bool
#define CJSON_GET_SUFFIX_BOOL
#include "cjson-array-get-template.h"

EXTERN_C bool CJSON_Array_set_string(struct CJSON_Array *const array, struct CJSON_Parser *const parser, const unsigned index, const char *const value) {
    assert(array != NULL);
    assert(parser != NULL);
    assert(value != NULL);

    struct CJSON json;
    char *const copy = CJSON_Arena_strdup(&parser->string_arena, value, &json.value.string.length);
    if(copy == NULL) {
        return false;
    }

    json.type               = CJSON_STRING;
    json.value.string.chars = copy;
    
    return CJSON_Array_set(array, parser, index, &json);
}

EXTERN_C bool CJSON_Array_set_float64(struct CJSON_Array *const array, struct CJSON_Parser *const parser, const unsigned index, const double value) {
    assert(array != NULL);
    assert(parser != NULL);
    
    struct CJSON json;
    json.type          = CJSON_FLOAT64;
    json.value.float64 = value;

    return CJSON_Array_set(array, parser, index, &json);
}

EXTERN_C bool CJSON_Array_set_int64(struct CJSON_Array *const array, struct CJSON_Parser *const parser, const unsigned index, const int64_t value) {
    assert(array != NULL);
    assert(parser != NULL);

    struct CJSON json;
    json.type        = CJSON_INT64;
    json.value.int64 = value;

    return CJSON_Array_set(array, parser, index, &json);
}

EXTERN_C bool CJSON_Array_set_uint64(struct CJSON_Array *const array, struct CJSON_Parser *const parser, const unsigned index, const uint64_t value) {
    assert(array != NULL);
    assert(parser != NULL);

    struct CJSON json;
    json.type         = CJSON_UINT64;
    json.value.uint64 = value;

    return CJSON_Array_set(array, parser, index, &json);
}

EXTERN_C bool CJSON_Array_set_array(struct CJSON_Array *const array, struct CJSON_Parser *const parser, const unsigned index, const struct CJSON_Array *const value) {
    assert(array != NULL);
    assert(parser != NULL);
    assert(value != NULL);

    if(array == value) {
        return true;
    }

    struct CJSON json;
    json.type        = CJSON_ARRAY;
    json.value.array = *value;

    return CJSON_Array_set(array, parser, index, &json);
}

EXTERN_C bool CJSON_Array_set_object(struct CJSON_Array *const array, struct CJSON_Parser *const parser, const unsigned index, const struct CJSON_Object *const value) {
    assert(array != NULL);
    assert(parser != NULL);
    assert(value != NULL);

    struct CJSON json;
    json.type         = CJSON_OBJECT;
    json.value.object = *value;

    return CJSON_Array_set(array, parser, index, &json);
}

EXTERN_C bool CJSON_Array_set_null(struct CJSON_Array *const array, struct CJSON_Parser *const parser, const unsigned index) {
    assert(array != NULL);
    assert(parser != NULL);

    struct CJSON json;
    json.type       = CJSON_NULL;
    json.value.null = NULL;

    return CJSON_Array_set(array, parser, index, &json);
}

EXTERN_C bool CJSON_Array_set_bool(struct CJSON_Array *const array, struct CJSON_Parser *const parser, const unsigned index, const bool value) {
    assert(array != NULL);
    assert(parser != NULL);

    struct CJSON json;
    json.type          = CJSON_BOOL;
    json.value.boolean = value;

    return CJSON_Array_set(array, parser, index, &json);
}