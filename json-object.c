#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "json.h"

#define INITIAL_JSON_OBJECT_CAPACITY (1 << 3)

static unsigned hash(const char *const key) {
    assert(key != NULL);

    const size_t length = strlen(key);
    assert(length > 0 && length <= UINT_MAX);
    unsigned i = 0u;
    unsigned hash = 0u;

    while (i != (unsigned)length) {
        hash += (unsigned)key[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    return hash;
}

static void JSON_Object_resize(JSON_Object *const object, const double multiplier) {
    assert(object != NULL);
    assert(multiplier > 1.0); //multiplier must actually increase the size
    assert(multiplier <= UINT_MAX / object->capacity); //check overflow

    const unsigned capacity = (unsigned)((double)object->capacity * multiplier);
    JSON_Key_Value *const old_data = object->data;
    const unsigned old_capacity = object->capacity;
    JSON_Key_Value *data = JSON_CALLOC((size_t)capacity, sizeof(JSON_Key_Value));
    assert(data != NULL);

    object->data = data;
    object->capacity = capacity;

    for(unsigned i = 0U; i < old_capacity; i++) {
        if(old_data[i].key == NULL) {
            continue;
        }
        JSON_Object_set(object, old_data[i].key, &old_data[i].value);
    }

    JSON_FREE(old_data);
}

void JSON_Object_init(JSON_Object *const object) {
    assert(object != NULL);
    
    JSON_Key_Value *data = JSON_CALLOC(INITIAL_JSON_OBJECT_CAPACITY, sizeof(JSON_Key_Value));
    assert(data != NULL);

    *object = (JSON_Object) {
        .data = data,
        .capacity = INITIAL_JSON_OBJECT_CAPACITY
    };
}

JSON_Key_Value *JSON_Object_get_entry(JSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    const unsigned start = hash(key) % object->capacity;
    unsigned i = start; 
    while(object->data[i].key != NULL && strcmp(object->data[i].key, key) != 0) {
        i = (i + 1) % object->capacity;
        if(i == start) {
            JSON_Object_resize(object, 2.0);
            return JSON_Object_get_entry(object, key);
        }
    }

    return object->data + i;
}

JSON_Key_Value *JSON_Object_find_entry(const JSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    const unsigned start = hash(key) % object->capacity;
    unsigned i = start; 
    do {
        if(object->data[i].key == NULL) {
            return NULL;
        } 

        if(strcmp(object->data[i].key, key) == 0) {
            return object->data + i;
        }

        i = (i + 1) % object->capacity;
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

    entry->value = *value;
}

void JSON_Object_delete(JSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    JSON_Key_Value *const entry = JSON_Object_find_entry(object, key);

    if(entry != NULL) {
        JSON_FREE(entry->key);
        entry->key = NULL;
        _JSON_free(&entry->value);
    }
}

void JSON_Object_free(JSON_Object *const object) {
    assert(object != NULL);

    for(unsigned i = 0U; i < object->capacity; i++) {
        JSON_Key_Value *const data = object->data + i;
        if(data->key == NULL) {
            continue;
        }
        JSON_FREE(data->key);
        _JSON_free(&data->value);
    }
    JSON_FREE(object->data);
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
