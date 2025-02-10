#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "parser.h"
#include "benchmark.h"

#define INITIAL_JSON_OBJECT_CAPACITY (1 << 3)

static char *const DELETED_ENTRY = {0};

static unsigned int CJSON_hash(const char *const key) {
    assert(key != NULL);

    BENCHMARK_START();

    const size_t length = strlen(key);
    assert(length > 0 && length <= UINT_MAX);
    unsigned int i = 0U;
    unsigned int hash = 0U;

    while (i != (unsigned int)length) {
        hash += (unsigned int)key[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    BENCHMARK_END();

    return hash;
}

static void CJSON_Object_resize(CJSON_Object *const object, const unsigned int capacity) {
    assert(object != NULL);
    assert(capacity > object->capacity); //new size must be larger than current size

    BENCHMARK_START();

    CJSON_Key_Value *const old_data = object->nodes;
    const unsigned int old_capacity = object->capacity;
    CJSON_Key_Value *data = CJSON_CALLOC((size_t)capacity, sizeof(CJSON_Key_Value));
    assert(data != NULL);

    object->nodes = data;
    object->capacity = capacity;

    for(unsigned int i = 0U; i < old_capacity; i++) {
        if(old_data[i].key == NULL || old_data[i].key == DELETED_ENTRY) {
            continue;
        }
        CJSON_Key_Value *const entry = CJSON_Object_get_entry(object, old_data[i].key);
        entry->key = old_data[i].key;
        entry->value = old_data[i].value;
    }

    CJSON_FREE(old_data);

    BENCHMARK_END();
}

void CJSON_Object_init(CJSON_Object *const object) {
    assert(object != NULL);
    
    CJSON_Key_Value *data = CJSON_CALLOC(INITIAL_JSON_OBJECT_CAPACITY, sizeof(CJSON_Key_Value));
    assert(data != NULL);

    *object = (CJSON_Object) {
        .nodes = data,
        .capacity = INITIAL_JSON_OBJECT_CAPACITY
    };
}

CJSON_Key_Value *CJSON_Object_get_entry(CJSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    BENCHMARK_START();

    const unsigned int start = CJSON_hash(key) % object->capacity;
    unsigned int i = start; 
    while(object->nodes[i].key != NULL && object->nodes[i].key != DELETED_ENTRY && strcmp(object->nodes[i].key, key) != 0) {
        i = (i + 1U) % object->capacity;
        if(i == start) {
            CJSON_Object_resize(object, object->capacity * 2);
            CJSON_Key_Value *const ret = CJSON_Object_get_entry(object, key);
            BENCHMARK_END();
            return ret;
        }
    }

    CJSON_Key_Value *const ret = object->nodes + i;
    BENCHMARK_END();
    return ret;
}

CJSON_Key_Value *CJSON_Object_find_entry(const CJSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    BENCHMARK_START();

    const unsigned int start = CJSON_hash(key) % object->capacity;
    unsigned int i = start; 
    do {
        if(object->nodes[i].key == DELETED_ENTRY) {
            i = (i + 1U) % object->capacity;
            continue;
        }

        if(object->nodes[i].key == NULL) {
            BENCHMARK_END();

            return NULL;
        } 

        if(strcmp(object->nodes[i].key, key) == 0) {
            CJSON_Key_Value *const ret = object->nodes + i;
            BENCHMARK_END();
            
            return ret;
        }

        i = (i + 1U) % object->capacity;
    } while(i != start);

    BENCHMARK_END();

    return NULL;
}

CJSON_JSON *CJSON_Object_get(const CJSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    BENCHMARK_START();

    CJSON_Key_Value *const entry = CJSON_Object_find_entry(object, key);

    BENCHMARK_END();

    return entry == NULL ? NULL : &entry->value;
}

void CJSON_Object_set(CJSON_Object *const object, const char *const key, const CJSON_JSON *const value) {
    assert(object != NULL);
    assert(key != NULL);
    assert(value != NULL);

    BENCHMARK_START();
    
    CJSON_Key_Value *const entry = CJSON_Object_get_entry(object, key);

    CJSON_internal_free(&entry->value);
    if(entry->key == NULL || entry->key == DELETED_ENTRY) {
        entry->key = CJSON_STRDUP(key);
    }
    entry->value = *value;

    BENCHMARK_END();
}

void CJSON_Object_delete(CJSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    BENCHMARK_START();

    CJSON_Key_Value *const entry = CJSON_Object_find_entry(object, key);

    if(entry != NULL) {
        CJSON_FREE(entry->key);
        CJSON_internal_free(&entry->value);
        entry->key = DELETED_ENTRY;
    }

    BENCHMARK_END();
}

void CJSON_Object_free(CJSON_Object *const object) {
    assert(object != NULL);

    BENCHMARK_START();

    for(unsigned int i = 0U; i < object->capacity; i++) {
        CJSON_Key_Value *const data = object->nodes + i;
        if(data->key == NULL || data->key == DELETED_ENTRY) {
            continue;
        }
        CJSON_FREE(data->key);
        CJSON_internal_free(&data->value);
    }
    CJSON_FREE(object->nodes);
    *object = (CJSON_Object){0};

    BENCHMARK_END();
}

#define CJSON_OBJECT_GET(JSON_TYPE)\
    assert(object != NULL);\
    assert(key != NULL);\
    assert(success != NULL);\
                            \
    BENCHMARK_START();\
    CJSON_JSON *const ret = CJSON_Object_get(object, key);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        BENCHMARK_END();\
        return 0;\
    }\
    *success = true;\

#define CJSON_OBJECT_GET_VALUE(JSON_TYPE, MEMBER)\
    CJSON_OBJECT_GET(JSON_TYPE)\
    BENCHMARK_END();\
    return ret->value.MEMBER;

#define CJSON_OBJECT_GET_PTR(JSON_TYPE, MEMBER)\
    CJSON_OBJECT_GET(JSON_TYPE)\
    BENCHMARK_END();\
    return &ret->value.MEMBER;

char *CJSON_Object_get_string(const CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_STRING, string)
}

double CJSON_Object_get_float64(const CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_FLOAT64, float64)
}

int64_t CJSON_Object_get_int64(const CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_INT64, int64)    
}

uint64_t CJSON_Object_get_uint64(const CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_UINT64, uint64)
}

CJSON_Object *CJSON_Object_get_object(const CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_PTR(CJSON_OBJECT, object)
}

CJSON_Array *CJSON_Object_get_array(const CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_PTR(CJSON_ARRAY, array)    
}

void *CJSON_Object_get_null(const CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_NULL, null)
}

bool CJSON_Object_get_bool(const CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_BOOL, boolean)
}

void CJSON_Object_set_string(CJSON_Object *const object, const char *const key, const char *const value) {
    assert(object != NULL);

    BENCHMARK_START();

    char *copy = value != NULL ? CJSON_STRDUP(value) : NULL;
    assert(value != NULL && copy != NULL);

    CJSON_Object_set(object, key, &(CJSON_JSON){
        .type = CJSON_STRING,
        .value = {.string = copy}
    });

    BENCHMARK_END();
}
void CJSON_Object_set_float64(CJSON_Object *const object, const char *const key, const double value) {
    assert(object != NULL);

    BENCHMARK_START();

    CJSON_Object_set(object, key, &(CJSON_JSON){
        .type = CJSON_FLOAT64,
        .value = {.float64 = value}
    });

    BENCHMARK_END();
}

void CJSON_Object_set_int64(CJSON_Object *const object, const char *const key, const int64_t value) {
    assert(object != NULL);

    BENCHMARK_START();

    CJSON_Object_set(object, key, &(CJSON_JSON){
        .type = CJSON_INT64,
        .value = {.int64 = value}
    });

    BENCHMARK_END();
}

void CJSON_Object_set_uint64(CJSON_Object *const object, const char *const key, const uint64_t value) {
    assert(object != NULL);

    BENCHMARK_START();

    CJSON_Object_set(object, key, &(CJSON_JSON){
        .type = CJSON_UINT64,
        .value = {.uint64 = value}
    });

    BENCHMARK_END();
}

void CJSON_Object_set_object(CJSON_Object *const object, const char *const key, const CJSON_Object *const value) {
    assert(object != NULL);
    assert(value != NULL);

    BENCHMARK_START();

    CJSON_Object_set(object, key, &(CJSON_JSON){
        .type = CJSON_OBJECT,
        .value = {.object = *value}
    });

    BENCHMARK_END();
}

void CJSON_Object_set_array(CJSON_Object *const object, const char *const key, const CJSON_Array *const value) {
    assert(object != NULL);
    assert(value != NULL);

    BENCHMARK_START();

    CJSON_Object_set(object, key, &(CJSON_JSON){
        .type = CJSON_ARRAY,
        .value = {.array = *value}
    });

    BENCHMARK_END();
}

void CJSON_Object_set_null(CJSON_Object *const object, const char *const key) {
    assert(object != NULL);

    BENCHMARK_START();

    CJSON_Object_set(object, key, &(CJSON_JSON){
        .type = CJSON_NULL,
    });

    BENCHMARK_END();
}

void CJSON_Object_set_bool(CJSON_Object *const object, const char *const key, const bool value) {
    assert(object != NULL);

    BENCHMARK_START();

    CJSON_Object_set(object, key, &(CJSON_JSON){
        .type = CJSON_BOOL,
        .value = {.boolean = value}
    });

    BENCHMARK_END();
}
