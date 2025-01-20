#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "parser.h"

#define INITIAL_JSON_ARRAY_CAPACITY (1 << 3)

static void JSON_Array_resize(JSON_Array *const array, const unsigned int capacity) {
    assert(array != NULL);
    assert(capacity > array->capacity); //new size must be larger than current size

    JSON *data = CJSON_REALLOC(array->data, capacity * sizeof(JSON), array->capacity * sizeof(JSON));
    
    assert(data != NULL);

    array->data = data;
    array->capacity = capacity;
}

void JSON_Array_init(JSON_Array *const array) {
    assert(array != NULL);

    JSON *data = CJSON_MALLOC(INITIAL_JSON_ARRAY_CAPACITY * sizeof(JSON));
    assert(data != NULL);

    *array = (JSON_Array) {
        .data = data,
        .capacity = INITIAL_JSON_ARRAY_CAPACITY
    };
}

void JSON_Array_free(JSON_Array *const array) {
    for(unsigned int i = 0U; i < array->length; i++) {
        _JSON_free(array->data + i);
    }
    CJSON_FREE(array->data);
    *array = (JSON_Array){0};
}

JSON *JSON_Array_next(JSON_Array *const array) {
    assert(array != NULL);

    if(array->length == array->capacity) {
        JSON_Array_resize(array, array->capacity * 2);
    }

    JSON *const ret = array->data + array->length;
    array->length++;

    return ret;
}

JSON *JSON_Array_get(const JSON_Array *const array, const unsigned int index) {
    assert(array != NULL);

    return index >= array->length ? NULL : array->data + index;
}

void JSON_Array_set(JSON_Array *const array, const unsigned int index, const JSON *const value) {
    assert(array != NULL);
    assert(value != NULL);

    if(index >= array->capacity) {
        unsigned int capacity = array->capacity;
        do {
            capacity *= 2U;
        } while(index >= capacity);
        JSON_Array_resize(array, capacity);
    } else if(index >= array->length) {
        array->length = index + 1U;
    }

    _JSON_free(array->data + index);
    array->data[index] = *value;
}

void JSON_Array_push(JSON_Array *const array, const JSON *const value) {
    assert(array != NULL);
    assert(value != NULL);

    JSON *const next = JSON_Array_next(array);

    *next = *value;
}

#define JSON_ARRAY_GET(JSON_TYPE)\
    assert(array != NULL);\
    assert(success != NULL);\
                            \
    JSON *const ret = JSON_Array_get(array, index);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        return 0;\
    }\
    *success = true;\

#define JSON_ARRAY_GET_VALUE(JSON_TYPE, MEMBER)\
    JSON_ARRAY_GET(JSON_TYPE)\
    return ret->value.MEMBER;

#define JSON_ARRAY_GET_PTR(JSON_TYPE, MEMBER)\
    JSON_ARRAY_GET(JSON_TYPE)\
    return &ret->value.MEMBER;

char *JSON_Array_get_string(const JSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(JSON_STRING, string)
}

double JSON_Array_get_float64(const JSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(JSON_FLOAT64, float64)
}

int64_t JSON_Array_get_int64(const JSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(JSON_INT64, int64)    
}

uint64_t JSON_Array_get_uint64(const JSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(JSON_UINT64, uint64)
}

JSON_Array *JSON_Array_get_array(const JSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_PTR(JSON_ARRAY, array)
}

JSON_Object *JSON_Array_get_object(const JSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_PTR(JSON_OBJECT, object)    
}

void *JSON_Array_get_null(const JSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(JSON_NULL, null)
}

bool JSON_Array_get_bool(const JSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(JSON_BOOL, boolean)
}

void JSON_Array_set_string (JSON_Array *const array, const unsigned int index, const char *const value) {
    assert(array != NULL);

    char *copy = value != NULL ? CJSON_STRDUP(value) : NULL;
    assert(value != NULL && copy != NULL);

    JSON_Array_set(array, index, &(JSON){
        .type = JSON_STRING,
        .value = {.string = copy}
    });
}

void JSON_Array_set_float64(JSON_Array *const array, const unsigned int index, const double value) {
    assert(array != NULL);

    JSON_Array_set(array, index, &(JSON){
        .type = JSON_FLOAT64,
        .value = {.float64 = value}
    });
}

void JSON_Array_set_int64  (JSON_Array *const array, const unsigned int index, const int64_t value) {
    assert(array != NULL);

    JSON_Array_set(array, index, &(JSON){
        .type = JSON_INT64,
        .value = {.int64 = value}
    });
}

void JSON_Array_set_uint64 (JSON_Array *const array, const unsigned int index, const uint64_t value) {
    assert(array != NULL);

    JSON_Array_set(array, index, &(JSON){
        .type = JSON_UINT64,
        .value = {.uint64 = value}
    });
}
void JSON_Array_set_array(JSON_Array *const array, const unsigned int index, const JSON_Array *const value) {
    assert(array != NULL);
    assert(value != NULL);

    JSON_Array_set(array, index, &(JSON){
        .type = JSON_ARRAY,
        .value = {.array = *value}
    });
}
void JSON_Array_set_object(JSON_Array *const array, const unsigned int index, const JSON_Object *const value) {
    assert(array != NULL);
    assert(value != NULL);

    JSON_Array_set(array, index, &(JSON){
        .type = JSON_OBJECT,
        .value = {.object = *value}
    });
}

void JSON_Array_set_null(JSON_Array *const array, const unsigned int index) {
    assert(array != NULL);

    array->data[index] = (JSON){
        .type = JSON_NULL
    };
}

void JSON_Array_set_bool(JSON_Array *const array, const unsigned int index, const bool value) {
    assert(array != NULL);

    JSON_Array_set(array, index, &(JSON){
        .type = JSON_BOOL,
        .value = {.boolean = value}
    });
}
