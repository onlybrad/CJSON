#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "cjson.h"
#include "util.h"

#define CJSON_ARRAY_DEFAULT_CAPACITY 8U

static bool CJSON_Array_resize(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned capacity) {
    assert(array != NULL);
    assert(root != NULL);
    assert(capacity > array->capacity);

    struct CJSON *values = CJSON_ARENA_ALLOC(
        &root->array_arena,
        capacity,
        struct CJSON
    );
    if(values == NULL) {
        return false;
    }

    memcpy(values, array->values, (size_t)array->count * sizeof(*values));
    array->values   = values;
    array->capacity = capacity;

    return true;
}

EXTERN_C bool CJSON_Array_init(struct CJSON_Array *const array, struct CJSON_Root *const root, unsigned capacity) {
    assert(array != NULL);
    assert(root != NULL);

    if(capacity < CJSON_ARRAY_DEFAULT_CAPACITY) {
        capacity = CJSON_ARRAY_DEFAULT_CAPACITY;
    }

    struct CJSON *values = CJSON_ARENA_ALLOC(&root->array_arena, capacity, struct CJSON);
    if(values == NULL) {
        return false;
    }

    array->values   = values;
    array->count    = 0U;
    array->capacity = capacity;

    return true;
}

EXTERN_C struct CJSON *CJSON_Array_next(struct CJSON_Array *const array, struct CJSON_Root *const root) {
    assert(array != NULL);
    assert(root != NULL);

    if(array->count == array->capacity) {
        if(!CJSON_Array_resize(array, root, array->capacity * 2)) {
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

EXTERN_C bool CJSON_Array_set(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned index, const struct CJSON *const value) {
    assert(array != NULL);
    assert(root != NULL);
    assert(value != NULL);

    if(index >= array->capacity) {
        unsigned capacity = array->capacity;
        do {
            capacity *= 2U;
        } while(index >= capacity);
        if(!CJSON_Array_resize(array, root, capacity)) {
            return false;
        }
    } else if(index >= array->count) {
        array->count = index + 1U;
    }

    array->values[index] = *value;
    return true;
}

EXTERN_C bool CJSON_Array_push(struct CJSON_Array *const array, struct CJSON_Root *const root,  const struct CJSON *const value) {
    assert(array != NULL);
    assert(root != NULL);
    assert(value != NULL);

    if(array->count == UINT_MAX) {
        return false;
    }

    struct CJSON *const next = CJSON_Array_next(array, root);
    if(next == NULL) {
        return false;
    }

    *next = *value;

    return true;
}

#define JSON_ARRAY_GET_VALUE(JSON_TYPE, MEMBER)\
    assert(array != NULL);\
    assert(success != NULL);\
                            \
    struct CJSON *const ret = CJSON_Array_get(array, index);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        return 0;\
    }\
    *success = true;\
    return ret->data.MEMBER;

#define JSON_ARRAY_GET_PTR(JSON_TYPE, MEMBER)\
    assert(array != NULL);\
    assert(success != NULL);\
                            \
    struct CJSON *const ret = CJSON_Array_get(array, index);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        return 0;\
    }\
    *success = true;\
    return &ret->data.MEMBER;

EXTERN_C const char *CJSON_Array_get_string(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_STRING, string.chars)
}

EXTERN_C double CJSON_Array_get_float64(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_FLOAT64, float64)
}

EXTERN_C int64_t CJSON_Array_get_int64(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_INT64, int64)    
}

EXTERN_C uint64_t CJSON_Array_get_uint64(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_UINT64, uint64)
}

EXTERN_C struct CJSON_Array *CJSON_Array_get_array(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    JSON_ARRAY_GET_PTR(CJSON_ARRAY, array)
}

EXTERN_C struct CJSON_Object *CJSON_Array_get_object(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    JSON_ARRAY_GET_PTR(CJSON_OBJECT, object)    
}

EXTERN_C void *CJSON_Array_get_null(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_NULL, null)
}

EXTERN_C bool CJSON_Array_get_bool(const struct CJSON_Array *const array, const unsigned index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_BOOL, boolean)
}

EXTERN_C bool CJSON_Array_set_string(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned index, const char *const value) {
    assert(array != NULL);
    assert(root != NULL);
    assert(value != NULL);

    struct CJSON json;
    char *const copy = CJSON_Arena_strdup(&root->string_arena, value, &json.data.string.length);
    if(copy == NULL) {
        return false;
    }

    json.type              = CJSON_STRING;
    json.data.string.chars = copy;
    
    return CJSON_Array_set(array, root, index, &json);
}

EXTERN_C bool CJSON_Array_set_float64(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned index, const double value) {
    assert(array != NULL);
    assert(root != NULL);
    
    struct CJSON json;
    json.type         = CJSON_FLOAT64;
    json.data.float64 = value;

    return CJSON_Array_set(array, root, index, &json);
}

EXTERN_C bool CJSON_Array_set_int64(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned index, const int64_t value) {
    assert(array != NULL);
    assert(root != NULL);

    struct CJSON json;
    json.type       = CJSON_INT64;
    json.data.int64 = value;

    return CJSON_Array_set(array, root, index, &json);
}

EXTERN_C bool CJSON_Array_set_uint64(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned index, const uint64_t value) {
    assert(array != NULL);
    assert(root != NULL);

    struct CJSON json;
    json.type        = CJSON_UINT64;
    json.data.uint64 = value;

    return CJSON_Array_set(array, root, index, &json);
}

EXTERN_C bool CJSON_Array_set_array(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned index, const struct CJSON_Array *const value) {
    assert(array != NULL);
    assert(root != NULL);
    assert(value != NULL);

    struct CJSON json;
    json.type       = CJSON_ARRAY;
    json.data.array = *value;

    return CJSON_Array_set(array, root, index, &json);
}

EXTERN_C bool CJSON_Array_set_object(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned index, const struct CJSON_Object *const value) {
    assert(array != NULL);
    assert(root != NULL);
    assert(value != NULL);

    struct CJSON json;
    json.type        = CJSON_OBJECT;
    json.data.object = *value;

    return CJSON_Array_set(array, root, index, &json);
}

EXTERN_C bool CJSON_Array_set_null(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned index) {
    assert(array != NULL);
    assert(root != NULL);

    struct CJSON json;
    json.type      = CJSON_NULL;
    json.data.null = NULL;

    return CJSON_Array_set(array, root, index, &json);
}

EXTERN_C bool CJSON_Array_set_bool(struct CJSON_Array *const array, struct CJSON_Root *const root, const unsigned index, const bool value) {
    assert(array != NULL);
    assert(root != NULL);

    struct CJSON json;
    json.type         = CJSON_BOOL;
    json.data.boolean = value;

    return CJSON_Array_set(array, root, index, &json);
}

EXTERN_C unsigned CJSON_Array_total_objects(const struct CJSON_Array *const array) {
    assert(array != NULL);

    unsigned total = 0U;

    for(const struct CJSON *value = array->values,
        *const last_value = value + array->count - 1; 
        value != last_value + 1;
        value++
    ) {
        total += CJSON_total_objects(value);
    }

    return total;
}

EXTERN_C unsigned CJSON_Array_total_arrays(const struct CJSON_Array *const array) {
    assert(array != NULL);

    unsigned total = 0U;

    for(const struct CJSON *value = array->values,
        *const last_value = value + array->count - 1; 
        value != last_value + 1;
        value++
    ) {
        total += CJSON_total_arrays(value);
    }

    return total + 1U;
}
