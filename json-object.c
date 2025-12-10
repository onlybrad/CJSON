#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "cjson.h"
#include "util.h"

#define INITIAL_JSON_OBJECT_CAPACITY (1 << 3)

static char DELETED_ENTRY[] = {0};

static unsigned CJSON_hash(const char *const key) {
    assert(key != NULL);

    const size_t length = strlen(key);
    assert(length > 0 && length <= UINT_MAX);
    unsigned i = 0U;
    unsigned hash = 0U;

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

static bool CJSON_Object_resize(struct CJSON_Object *const object, struct CJSON_Root *const root, const unsigned capacity) {
    assert(object != NULL);
    assert(root != NULL);
    assert(capacity > object->capacity);

    struct CJSON_KV *const old_entries  = object->entries;
    const unsigned         old_capacity = object->capacity;
    struct CJSON_KV *const entries      = CJSON_ARENA_ALLOC(&root->object_arena, capacity, struct CJSON_KV);
    if(entries == NULL) {
        return false;
    }

    object->entries = entries;
    object->capacity = capacity;

    for(unsigned i = 0U; i < old_capacity; i++) {
        struct CJSON_KV *const old_entry = old_entries + i;
        if(old_entry->key == NULL || old_entry->key == DELETED_ENTRY) {
            continue;
        }
        struct CJSON_KV *const entry = CJSON_Object_get_entry(object, root, old_entry->key);
        entry->key   = old_entry->key;
        entry->value = old_entry->value;
    }

    return true;
}

EXTERN_C bool CJSON_Object_init(struct CJSON_Object *const object, struct CJSON_Root *const root, const unsigned capacity) {
    assert(object != NULL);
    assert(root != NULL);
    
    struct CJSON_KV *entries = CJSON_ARENA_ALLOC(
        &root->object_arena,
        capacity < INITIAL_JSON_OBJECT_CAPACITY ? INITIAL_JSON_OBJECT_CAPACITY : capacity,
        struct CJSON_KV
    );
    if(entries == NULL) {
        return false;
    }

    object->entries  = entries;
    object->capacity = INITIAL_JSON_OBJECT_CAPACITY;
    
    return true;
}

EXTERN_C struct CJSON_KV *CJSON_Object_get_entry(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    const unsigned start = CJSON_hash(key) % object->capacity;
    unsigned i = start; 
    while(
        object->entries[i].key != NULL 
        && object->entries[i].key != DELETED_ENTRY
        && strcmp(object->entries[i].key, key
    ) != 0) {
        i = (i + 1U) % object->capacity;
        if(i == start) {
            if(!CJSON_Object_resize(object, root, object->capacity * 2)) {
                return NULL;
            }
            return CJSON_Object_get_entry(object, root, key);
        }
    }

    return object->entries + i;
}

EXTERN_C struct CJSON_KV *CJSON_Object_find_entry(const struct CJSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    const unsigned start = CJSON_hash(key) % object->capacity;
    unsigned i = start; 
    do {
        if(object->entries[i].key == DELETED_ENTRY) {
            i = (i + 1U) % object->capacity;
            continue;
        }

        if(object->entries[i].key == NULL) {
            return NULL;
        } 

        if(strcmp(object->entries[i].key, key) == 0) {
            return object->entries + i;
        }

        i = (i + 1U) % object->capacity;
    } while(i != start);

    return NULL;
}

EXTERN_C struct CJSON *CJSON_Object_get(const struct CJSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    struct CJSON_KV *const entry = CJSON_Object_find_entry(object, key);

    return entry == NULL ? NULL : &entry->value;
}

EXTERN_C bool CJSON_Object_set(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key, const struct CJSON *const value) {
    assert(object != NULL);
    assert(root != NULL);
    assert(key != NULL);
    assert(value != NULL);
    
    struct CJSON_KV *const entry = CJSON_Object_get_entry(object, root, key);
    if(entry == NULL) {
        return false;
    }

    if(entry->key == NULL || entry->key == DELETED_ENTRY) {
        entry->key = (char*)CJSON_Arena_strdup(&root->string_arena, key, NULL);
        if(entry->key == NULL) {
            return false;
        }
    }
    
    entry->value = *value;

    return true;
}

EXTERN_C void CJSON_Object_delete(struct CJSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    struct CJSON_KV *const entry = CJSON_Object_find_entry(object, key);
    if(entry != NULL) {
        entry->key             = DELETED_ENTRY;
        entry->value.type      = CJSON_NULL;
        entry->value.data.null = NULL;
    }
}

#define CJSON_OBJECT_GET(JSON_TYPE)\
    assert(object != NULL);\
    assert(key != NULL);\
    assert(success != NULL);\
                            \
    struct CJSON *const ret = CJSON_Object_get(object, key);\
    if(ret == NULL || ret->type != JSON_TYPE) {\
        *success = false;\
        return 0;\
    }\
    *success = true;\

#define CJSON_OBJECT_GET_VALUE(JSON_TYPE, MEMBER)\
    CJSON_OBJECT_GET(JSON_TYPE)\
    return ret->data.MEMBER;

#define CJSON_OBJECT_GET_PTR(JSON_TYPE, MEMBER)\
    CJSON_OBJECT_GET(JSON_TYPE)\
    return &ret->data.MEMBER;

EXTERN_C const char *CJSON_Object_get_string(const struct CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_STRING, string.chars)
}

EXTERN_C double CJSON_Object_get_float64(const struct CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_FLOAT64, float64)
}

EXTERN_C int64_t CJSON_Object_get_int64(const struct CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_INT64, int64)    
}

EXTERN_C uint64_t CJSON_Object_get_uint64(const struct CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_UINT64, uint64)
}

EXTERN_C struct CJSON_Object *CJSON_Object_get_object(const struct CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_PTR(CJSON_OBJECT, object)
}

EXTERN_C struct CJSON_Array *CJSON_Object_get_array(const struct CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_PTR(CJSON_ARRAY, array)    
}

EXTERN_C void *CJSON_Object_get_null(const struct CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_NULL, null)
}

EXTERN_C bool CJSON_Object_get_bool(const struct CJSON_Object *const object, const char *const key, bool *const success) {
    CJSON_OBJECT_GET_VALUE(CJSON_BOOL, boolean)
}

EXTERN_C bool CJSON_Object_set_string(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key, const char *const value) {
    assert(object != NULL);
    assert(root != NULL);
    assert(key != NULL);
    assert(value != NULL);

    struct CJSON json;
    char *const copy = CJSON_Arena_strdup(&root->string_arena, value, &json.data.string.length);
    if(copy == NULL) {
        return false;
    }

    json.type              = CJSON_STRING;
    json.data.string.chars = copy;
    
    return CJSON_Object_set(object, root, key, &json);
}

EXTERN_C bool CJSON_Object_set_float64(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key, const double value) {
    assert(object != NULL);
    assert(root != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type         = CJSON_FLOAT64;
    json.data.float64 = value;

    return CJSON_Object_set(object, root, key, &json);
}

EXTERN_C bool CJSON_Object_set_int64(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key, const int64_t value) {
    assert(object != NULL);
    assert(root != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type       = CJSON_INT64;
    json.data.int64 = value;

    return CJSON_Object_set(object, root, key, &json);
}

EXTERN_C bool CJSON_Object_set_uint64(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key, const uint64_t value) {
    assert(object != NULL);
    assert(root != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type        = CJSON_UINT64;
    json.data.uint64 = value;

    return CJSON_Object_set(object, root, key, &json);
}

EXTERN_C bool CJSON_Object_set_object(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key, const struct CJSON_Object *const value) {
    assert(object != NULL);
    assert(root != NULL);
    assert(key != NULL);
    assert(value != NULL);

    struct CJSON json;
    json.type        = CJSON_OBJECT;
    json.data.object = *value;

    return CJSON_Object_set(object, root, key, &json);
}

EXTERN_C bool CJSON_Object_set_array(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key, const struct CJSON_Array *const value) {
    assert(object != NULL);
    assert(root != NULL);
    assert(key != NULL);
    assert(value != NULL);

    struct CJSON json;
    json.type       = CJSON_ARRAY;
    json.data.array = *value;

    return CJSON_Object_set(object, root, key, &json);
}

EXTERN_C bool CJSON_Object_set_null(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key) {
    assert(object != NULL);
    assert(root != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type      = CJSON_NULL;
    json.data.null = NULL;

    return CJSON_Object_set(object, root, key, &json);
}

EXTERN_C bool CJSON_Object_set_bool(struct CJSON_Object *const object, struct CJSON_Root *const root, const char *const key, const bool value) {
    assert(object != NULL);
    assert(root != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type         = CJSON_BOOL;
    json.data.boolean = value;

    return CJSON_Object_set(object, root, key, &json);
}
