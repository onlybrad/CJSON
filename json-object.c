#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "parser.h"

#define INITIAL_JSON_OBJECT_CAPACITY (1 << 3)

static char *const DELETED_ENTRY = {0};

static unsigned int hash(const char *const key) {
    assert(key != NULL);

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

    return hash;
}

static void JSON_Object_resize(JSON_Object *const object, const unsigned int capacity) {
    assert(object != NULL);
    assert(capacity > object->capacity); //new size must be larger than current size

    JSON_Key_Value *const old_data = object->data;
    const unsigned int old_capacity = object->capacity;
    JSON_Key_Value *data = CJSON_CALLOC((size_t)capacity, sizeof(JSON_Key_Value));
    assert(data != NULL);

    object->data = data;
    object->capacity = capacity;

    for(unsigned int i = 0U; i < old_capacity; i++) {
        if(old_data[i].key == NULL || old_data[i].key == DELETED_ENTRY) {
            continue;
        }
        JSON_Key_Value *const entry = JSON_Object_get_entry(object, old_data[i].key);
        entry->key = old_data[i].key;
        entry->value = old_data[i].value;
    }

    CJSON_FREE(old_data);
}

void JSON_Object_init(JSON_Object *const object) {
    assert(object != NULL);
    
    JSON_Key_Value *data = CJSON_CALLOC(INITIAL_JSON_OBJECT_CAPACITY, sizeof(JSON_Key_Value));
    assert(data != NULL);

    *object = (JSON_Object) {
        .data = data,
        .capacity = INITIAL_JSON_OBJECT_CAPACITY
    };
}

JSON_Key_Value *JSON_Object_get_entry(JSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    const unsigned int start = hash(key) % object->capacity;
    unsigned int i = start; 
    while(object->data[i].key != NULL && object->data[i].key != DELETED_ENTRY && strcmp(object->data[i].key, key) != 0) {
        i = (i + 1U) % object->capacity;
        if(i == start) {
            JSON_Object_resize(object, object->capacity * 2);
            return JSON_Object_get_entry(object, key);
        }
    }

    return object->data + i;
}

JSON_Key_Value *JSON_Object_find_entry(const JSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    const unsigned int start = hash(key) % object->capacity;
    unsigned int i = start; 
    do {
        if(object->data[i].key == DELETED_ENTRY) {
            i = (i + 1U) % object->capacity;
            continue;
        }

        if(object->data[i].key == NULL) {
            return NULL;
        } 

        if(strcmp(object->data[i].key, key) == 0) {
            return object->data + i;
        }

        i = (i + 1U) % object->capacity;
    } while(i != start);

    return NULL;
}

JSON *JSON_Object_get(const JSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    JSON_Key_Value *const entry = JSON_Object_find_entry(object, key);

    return entry == NULL ? NULL : &entry->value;
}

void JSON_Object_set(JSON_Object *const object, const char *const key, const JSON *const value) {
    assert(object != NULL);
    assert(key != NULL);
    assert(value != NULL);
    
    JSON_Key_Value *const entry = JSON_Object_get_entry(object, key);

    _JSON_free(&entry->value);
    if(entry->key == NULL || entry->key == DELETED_ENTRY) {
        entry->key = CJSON_STRDUP(key);
    }
    entry->value = *value;
}

void JSON_Object_delete(JSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    JSON_Key_Value *const entry = JSON_Object_find_entry(object, key);

    if(entry != NULL) {
        CJSON_FREE(entry->key);
        _JSON_free(&entry->value);
        entry->key = DELETED_ENTRY;
    }
}

void JSON_Object_free(JSON_Object *const object) {
    assert(object != NULL);

    for(unsigned int i = 0U; i < object->capacity; i++) {
        JSON_Key_Value *const data = object->data + i;
        if(data->key == NULL || data->key == DELETED_ENTRY) {
            continue;
        }
        CJSON_FREE(data->key);
        _JSON_free(&data->value);
    }
    CJSON_FREE(object->data);
    *object = (JSON_Object){0};
}

#define JSON_OBJECT_GET(JSON_TYPE)\
    assert(object != NULL);\
    assert(key != NULL);\
    assert(success != NULL);\
                            \
    JSON *const ret = JSON_Object_get(object, key);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        return 0;\
    }\
    *success = true;\

#define JSON_OBJECT_GET_VALUE(JSON_TYPE, MEMBER)\
    JSON_OBJECT_GET(JSON_TYPE)\
    return ret->value.MEMBER;

#define JSON_OBJECT_GET_PTR(JSON_TYPE, MEMBER)\
    JSON_OBJECT_GET(JSON_TYPE)\
    return &ret->value.MEMBER;

char *JSON_Object_get_string(const JSON_Object *const object, const char *const key, bool *const success) {
    JSON_OBJECT_GET_VALUE(JSON_STRING, string)
}

double JSON_Object_get_float64(const JSON_Object *const object, const char *const key, bool *const success) {
    JSON_OBJECT_GET_VALUE(JSON_FLOAT64, float64)
}

int64_t JSON_Object_get_int64(const JSON_Object *const object, const char *const key, bool *const success) {
    JSON_OBJECT_GET_VALUE(JSON_INT64, int64)    
}

uint64_t JSON_Object_get_uint64(const JSON_Object *const object, const char *const key, bool *const success) {
    JSON_OBJECT_GET_VALUE(JSON_UINT64, uint64)
}

JSON_Object *JSON_Object_get_object(const JSON_Object *const object, const char *const key, bool *const success) {
    JSON_OBJECT_GET_PTR(JSON_OBJECT, object)
}

JSON_Array *JSON_Object_get_array(const JSON_Object *const object, const char *const key, bool *const success) {
    JSON_OBJECT_GET_PTR(JSON_ARRAY, array)    
}

void *JSON_Object_get_null(const JSON_Object *const object, const char *const key, bool *const success) {
    JSON_OBJECT_GET_VALUE(JSON_NULL, null)
}

bool JSON_Object_get_bool(const JSON_Object *const object, const char *const key, bool *const success) {
    JSON_OBJECT_GET_VALUE(JSON_BOOL, boolean)
}

void JSON_Object_set_string(JSON_Object *const object, const char *const key, const char *const value) {
    assert(object != NULL);

    char *copy = value != NULL ? CJSON_STRDUP(value) : NULL;
    assert(value != NULL && copy != NULL);

    JSON_Object_set(object, key, &(JSON){
        .type = JSON_STRING,
        .value = {.string = copy}
    });
}
void JSON_Object_set_float64(JSON_Object *const object, const char *const key, const double value) {
    assert(object != NULL);

    JSON_Object_set(object, key, &(JSON){
        .type = JSON_FLOAT64,
        .value = {.float64 = value}
    });
}

void JSON_Object_set_int64(JSON_Object *const object, const char *const key, const int64_t value) {
    assert(object != NULL);

    JSON_Object_set(object, key, &(JSON){
        .type = JSON_INT64,
        .value = {.int64 = value}
    });
}

void JSON_Object_set_uint64(JSON_Object *const object, const char *const key, const uint64_t value) {
    assert(object != NULL);

    JSON_Object_set(object, key, &(JSON){
        .type = JSON_UINT64,
        .value = {.uint64 = value}
    });
}

void JSON_Object_set_object(JSON_Object *const object, const char *const key, const JSON_Object *const value) {
    assert(object != NULL);
    assert(value != NULL);

    JSON_Object_set(object, key, &(JSON){
        .type = JSON_OBJECT,
        .value = {.object = *value}
    });
}

void JSON_Object_set_array(JSON_Object *const object, const char *const key, const JSON_Array *const value) {
    assert(object != NULL);
    assert(value != NULL);

    JSON_Object_set(object, key, &(JSON){
        .type = JSON_ARRAY,
        .value = {.array = *value}
    });
}

void JSON_Object_set_null(JSON_Object *const object, const char *const key) {
    assert(object != NULL);

    JSON_Object_set(object, key, &(JSON){
        .type = JSON_NULL,
    });
}

void JSON_Object_set_bool(JSON_Object *const object, const char *const key, const bool value) {
    assert(object != NULL);

    JSON_Object_set(object, key, &(JSON){
        .type = JSON_BOOL,
        .value = {.boolean = value}
    });  
}
