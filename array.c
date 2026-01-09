#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "json.h"
#include "parser.h"
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
        bool success;
        const unsigned new_capacity = safe_unsigned_mult(array->capacity, 2U, &success);
        if(!success || !CJSON_Array_reserve(array, parser, new_capacity)) {
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
            bool success;
            capacity = safe_unsigned_mult(capacity, 2U, &success);
            if(!success) {
                return false;
            }
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

EXTERN_C const char *CJSON_Array_get_string(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    assert(array != NULL);
    assert(success != NULL);

    struct CJSON *const json = CJSON_Array_get(array, index);
    if(json == NULL) {
        *success = false;
        return 0;
    }
    
    return CJSON_as_string(json, success);
}

EXTERN_C double CJSON_Array_get_float64(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    assert(array != NULL);
    assert(success != NULL);

    struct CJSON *const json = CJSON_Array_get(array, index);
    if(json == NULL) {
        *success = false;
        return 0;
    }
    
    return CJSON_as_float64(json, success);
}

EXTERN_C int64_t CJSON_Array_get_int64(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    assert(array != NULL);
    assert(success != NULL);

    struct CJSON *const json = CJSON_Array_get(array, index);
    if(json == NULL) {
        *success = false;
        return 0;
    }
    
    return CJSON_as_int64(json, success);
}

EXTERN_C uint64_t CJSON_Array_get_uint64(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    assert(array != NULL);
    assert(success != NULL);

    struct CJSON *const json = CJSON_Array_get(array, index);
    if(json == NULL) {
        *success = false;
        return 0;
    }
    
    return CJSON_as_uint64(json, success);
}

EXTERN_C struct CJSON_Object *CJSON_Array_get_object(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    assert(array != NULL);
    assert(success != NULL);

    struct CJSON *const json = CJSON_Array_get(array, index);
    if(json == NULL) {
        *success = false;
        return 0;
    }
    
    return CJSON_as_object(json, success);
}

EXTERN_C struct CJSON_Array *CJSON_Array_get_array(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    assert(array != NULL);
    assert(success != NULL);

    struct CJSON *const json = CJSON_Array_get(array, index);
    if(json == NULL) {
        *success = false;
        return 0;
    }
    
    return CJSON_as_array(json, success);
}

EXTERN_C void *CJSON_Array_get_null(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    assert(array != NULL);
    assert(success != NULL);

    struct CJSON *const json = CJSON_Array_get(array, index);
    if(json == NULL) {
        *success = false;
        return 0;
    }
    
    return CJSON_as_null(json, success);
}

EXTERN_C bool CJSON_Array_get_bool(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    assert(array != NULL);
    assert(success != NULL);

    struct CJSON *const json = CJSON_Array_get(array, index);
    if(json == NULL) {
        *success = false;
        return 0;
    }
    
    return CJSON_as_bool(json, success);
}

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