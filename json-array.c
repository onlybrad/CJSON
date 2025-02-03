#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "parser.h"
#include "benchmark.h"

#define INITIAL_JSON_ARRAY_CAPACITY (1 << 3)

static void CJSON_Array_resize(CJSON_Array *const array, const unsigned int capacity) {
    assert(array != NULL);
    assert(capacity > array->capacity); //new size must be larger than current size

    BENCHMARK_START();

    CJSON_Node *data = CJSON_REALLOC(array->nodes, (size_t)capacity * sizeof(CJSON_Node), (size_t)array->capacity * sizeof(CJSON_Node));
    
    assert(data != NULL);

    array->nodes = data;
    array->capacity = capacity;

    BENCHMARK_END();
}

void CJSON_Array_init(CJSON_Array *const array) {
    assert(array != NULL);

    CJSON_Node *data = CJSON_MALLOC(INITIAL_JSON_ARRAY_CAPACITY * sizeof(CJSON_Node));
    assert(data != NULL);

    *array = (CJSON_Array) {
        .nodes = data,
        .capacity = INITIAL_JSON_ARRAY_CAPACITY
    };
}

void CJSON_Array_free(CJSON_Array *const array) {
    for(unsigned int i = 0U; i < array->length; i++) {
        CJSON_Node_free(array->nodes + i);
    }
    CJSON_FREE(array->nodes);
    *array = (CJSON_Array){0};
}

CJSON_Node *CJSON_Array_next(CJSON_Array *const array) {
    assert(array != NULL);

    BENCHMARK_START();

    if(array->length == array->capacity) {
        CJSON_Array_resize(array, array->capacity * 2);
    }

    CJSON_Node *const ret = array->nodes + array->length;
    array->length++;

    BENCHMARK_END();

    return ret;
}

CJSON_Node *CJSON_Array_get(const CJSON_Array *const array, const unsigned int index) {
    assert(array != NULL);

    return index >= array->length ? NULL : array->nodes + index;
}

void CJSON_Array_set(CJSON_Array *const array, const unsigned int index, const CJSON_Node *const value) {
    assert(array != NULL);
    assert(value != NULL);

    BENCHMARK_START();

    if(index >= array->capacity) {
        unsigned int capacity = array->capacity;
        do {
            capacity *= 2U;
        } while(index >= capacity);
        CJSON_Array_resize(array, capacity);
    } else if(index >= array->length) {
        array->length = index + 1U;
    }

    CJSON_Node_free(array->nodes + index);
    array->nodes[index] = *value;

    BENCHMARK_END();
}

void CJSON_Array_push(CJSON_Array *const array, const CJSON_Node *const value) {
    assert(array != NULL);
    assert(value != NULL);

    BENCHMARK_START();

    CJSON_Node *const next = CJSON_Array_next(array);

    *next = *value;

    BENCHMARK_END();
}

#define JSON_ARRAY_GET(JSON_TYPE)\
    assert(array != NULL);\
    assert(success != NULL);\
                            \
    BENCHMARK_START();\
    CJSON_Node *const ret = CJSON_Array_get(array, index);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        BENCHMARK_END();\
        return 0;\
    }\
    *success = true;\

#define JSON_ARRAY_GET_VALUE(JSON_TYPE, MEMBER)\
    JSON_ARRAY_GET(JSON_TYPE)\
    BENCHMARK_END();\
    return ret->value.MEMBER;

#define JSON_ARRAY_GET_PTR(JSON_TYPE, MEMBER)\
    JSON_ARRAY_GET(JSON_TYPE)\
    BENCHMARK_END();\
    return &ret->value.MEMBER;

char *CJSON_Array_get_string(const CJSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_STRING, string)
}

double CJSON_Array_get_float64(const CJSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_FLOAT64, float64)
}

int64_t CJSON_Array_get_int64(const CJSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_INT64, int64)    
}

uint64_t CJSON_Array_get_uint64(const CJSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_UINT64, uint64)
}

CJSON_Array *CJSON_Array_get_array(const CJSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_PTR(CJSON_ARRAY, array)
}

CJSON_Object *CJSON_Array_get_object(const CJSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_PTR(CJSON_OBJECT, object)    
}

void *CJSON_Array_get_null(const CJSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_NULL, null)
}

bool CJSON_Array_get_bool(const CJSON_Array *const array, const unsigned int index, bool *const success) {
    JSON_ARRAY_GET_VALUE(CJSON_BOOL, boolean)
}

void CJSON_Array_set_string (CJSON_Array *const array, const unsigned int index, const char *const value) {
    assert(array != NULL);

    BENCHMARK_START();

    char *copy = value != NULL ? CJSON_STRDUP(value) : NULL;
    assert(value != NULL && copy != NULL);

    CJSON_Array_set(array, index, &(CJSON_Node){
        .type = CJSON_STRING,
        .value = {.string = copy}
    });

    BENCHMARK_END();
}

void CJSON_Array_set_float64(CJSON_Array *const array, const unsigned int index, const double value) {
    assert(array != NULL);
    
    BENCHMARK_START();

    CJSON_Array_set(array, index, &(CJSON_Node){
        .type = CJSON_FLOAT64,
        .value = {.float64 = value}
    });

    BENCHMARK_END();
}

void CJSON_Array_set_int64(CJSON_Array *const array, const unsigned int index, const int64_t value) {
    assert(array != NULL);

    BENCHMARK_START();

    CJSON_Array_set(array, index, &(CJSON_Node){
        .type = CJSON_INT64,
        .value = {.int64 = value}
    });

    BENCHMARK_END();
}

void CJSON_Array_set_uint64(CJSON_Array *const array, const unsigned int index, const uint64_t value) {
    assert(array != NULL);

    BENCHMARK_START();

    CJSON_Array_set(array, index, &(CJSON_Node){
        .type = CJSON_UINT64,
        .value = {.uint64 = value}
    });

    BENCHMARK_END();
}
void CJSON_Array_set_array(CJSON_Array *const array, const unsigned int index, const CJSON_Array *const value) {
    assert(array != NULL);
    assert(value != NULL);

    BENCHMARK_START();

    CJSON_Array_set(array, index, &(CJSON_Node){
        .type = CJSON_ARRAY,
        .value = {.array = *value}
    });

    BENCHMARK_END();
}
void CJSON_Array_set_object(CJSON_Array *const array, const unsigned int index, const CJSON_Object *const value) {
    assert(array != NULL);
    assert(value != NULL);

    BENCHMARK_START();

    CJSON_Array_set(array, index, &(CJSON_Node){
        .type = CJSON_OBJECT,
        .value = {.object = *value}
    });

    BENCHMARK_END();
}

void CJSON_Array_set_null(CJSON_Array *const array, const unsigned int index) {
    assert(array != NULL);

    BENCHMARK_START();

    array->nodes[index] = (CJSON_Node){
        .type = CJSON_NULL
    };

    BENCHMARK_END();
}

void CJSON_Array_set_bool(CJSON_Array *const array, const unsigned int index, const bool value) {
    assert(array != NULL);

    BENCHMARK_START();

    CJSON_Array_set(array, index, &(CJSON_Node){
        .type = CJSON_BOOL,
        .value = {.boolean = value}
    });

    BENCHMARK_END();
}
