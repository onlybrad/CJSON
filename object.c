#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "cjson.h"
#include "util.h"

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

static bool CJSON_Object_resize(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const unsigned capacity) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(capacity > object->capacity);

    struct CJSON_KV *const old_entries  = object->entries;
    const unsigned         old_capacity = object->capacity;
    struct CJSON_KV *const entries      = CJSON_ARENA_ALLOC(&parser->object_arena, capacity, struct CJSON_KV);
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
        struct CJSON_KV *const entry = CJSON_Object_get_entry(object, parser, old_entry->key);
        entry->key   = old_entry->key;
        entry->value = old_entry->value;
    }

    return true;
}

EXTERN_C void CJSON_Object_init(struct CJSON_Object *const object) {
    assert(object != NULL);

    object->entries  = NULL;
    object->capacity = 0U;
}

EXTERN_C bool CJSON_Object_reserve(struct CJSON_Object *const object, struct CJSON_Parser *const parser, unsigned capacity) {
    assert(object != NULL);
    assert(parser != NULL);

    if(capacity < CJSON_OBJECT_MINIMUM_CAPACITY) {
        capacity = CJSON_OBJECT_MINIMUM_CAPACITY;
    }

    if(capacity <= object->capacity) {
        return true;
    }
    
    struct CJSON_KV *entries = CJSON_ARENA_ALLOC(&parser->object_arena, capacity, struct CJSON_KV);
    if(entries == NULL) {
        return false;
    }

    object->entries  = entries;
    object->capacity = capacity;
    
    return true;
}

EXTERN_C struct CJSON_KV *CJSON_Object_get_entry(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    if(object->capacity == 0) {
        if(!CJSON_Object_reserve(object, parser, 0U)) {
            return NULL;
        }
    }

    const unsigned start = CJSON_hash(key) % object->capacity;
    unsigned i = start; 
    while(
        object->entries[i].key != NULL 
        && object->entries[i].key != DELETED_ENTRY
        && strcmp(object->entries[i].key, key
    ) != 0) {
        i = (i + 1U) % object->capacity;
        if(i == start) {
            if(!CJSON_Object_resize(object, parser, object->capacity * 2U)) {
                return NULL;
            }
            return CJSON_Object_get_entry(object, parser, key);
        }
    }

    return object->entries + i;
}

EXTERN_C struct CJSON_KV *CJSON_Object_find_entry(const struct CJSON_Object *const object, const char *const key) {
    assert(object != NULL);
    assert(key != NULL);

    if(object->capacity == 0) {
        return NULL;
    }

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

EXTERN_C bool CJSON_Object_set(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key, const struct CJSON *const value) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(key != NULL);
    assert(value != NULL);
    
    struct CJSON_KV *const entry = CJSON_Object_get_entry(object, parser, key);
    if(entry == NULL) {
        return false;
    }

    if(entry->key == NULL || entry->key == DELETED_ENTRY) {
        entry->key = (char*)CJSON_Arena_strdup(&parser->string_arena, key, NULL);
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
        entry->value.value.null = NULL;
    }
}

#define CJSON_GET_TYPE        CJSON_STRING
#define CJSON_GET_MEMBER      string.chars
#define CJSON_GET_SUFFIX      string
#define CJSON_GET_RETURN_TYPE const char*
#include "cjson-object-get-template.h"

#define CJSON_GET_TYPE        CJSON_FLOAT64
#define CJSON_GET_MEMBER      float64
#define CJSON_GET_SUFFIX      float64
#define CJSON_GET_RETURN_TYPE double
#include "cjson-object-get-template.h"

#define CJSON_GET_TYPE        CJSON_INT64
#define CJSON_GET_MEMBER      int64
#define CJSON_GET_SUFFIX      int64
#define CJSON_GET_RETURN_TYPE int64_t
#include "cjson-object-get-template.h"

#define CJSON_GET_TYPE        CJSON_UINT64
#define CJSON_GET_MEMBER      uint64
#define CJSON_GET_SUFFIX      uint64
#define CJSON_GET_RETURN_TYPE uint64_t
#include "cjson-object-get-template.h"

#define CJSON_GET_TYPE        CJSON_OBJECT
#define CJSON_GET_MEMBER      object
#define CJSON_GET_SUFFIX      object
#define CJSON_GET_RETURN_TYPE struct CJSON_Object*
#define CJSON_GET_RETURN_PTR
#include "cjson-object-get-template.h"

#define CJSON_GET_TYPE        CJSON_ARRAY
#define CJSON_GET_MEMBER      array
#define CJSON_GET_SUFFIX      array
#define CJSON_GET_RETURN_TYPE struct CJSON_Array*
#define CJSON_GET_RETURN_PTR
#include "cjson-object-get-template.h"

#define CJSON_GET_TYPE        CJSON_NULL
#define CJSON_GET_MEMBER      null
#define CJSON_GET_SUFFIX      null
#define CJSON_GET_RETURN_TYPE void*
#include "cjson-object-get-template.h"

#define CJSON_GET_TYPE        CJSON_BOOL
#define CJSON_GET_MEMBER      boolean
#define CJSON_GET_RETURN_TYPE bool
#define CJSON_GET_SUFFIX_BOOL
#include "cjson-object-get-template.h"

EXTERN_C bool CJSON_Object_set_string(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key, const char *const value) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(key != NULL);
    assert(value != NULL);

    struct CJSON json;
    char *const copy = CJSON_Arena_strdup(&parser->string_arena, value, &json.value.string.length);
    if(copy == NULL) {
        return false;
    }

    json.type               = CJSON_STRING;
    json.value.string.chars = copy;
    
    return CJSON_Object_set(object, parser, key, &json);
}

EXTERN_C bool CJSON_Object_set_float64(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key, const double value) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type          = CJSON_FLOAT64;
    json.value.float64 = value;

    return CJSON_Object_set(object, parser, key, &json);
}

EXTERN_C bool CJSON_Object_set_int64(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key, const int64_t value) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type        = CJSON_INT64;
    json.value.int64 = value;

    return CJSON_Object_set(object, parser, key, &json);
}

EXTERN_C bool CJSON_Object_set_uint64(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key, const uint64_t value) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type         = CJSON_UINT64;
    json.value.uint64 = value;

    return CJSON_Object_set(object, parser, key, &json);
}

EXTERN_C bool CJSON_Object_set_object(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key, const struct CJSON_Object *const value) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(key != NULL);
    assert(value != NULL);

    if(object == value) {
        return true;
    }

    struct CJSON json;
    json.type         = CJSON_OBJECT;
    json.value.object = *value;

    return CJSON_Object_set(object, parser, key, &json);
}

EXTERN_C bool CJSON_Object_set_array(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key, const struct CJSON_Array *const value) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(key != NULL);
    assert(value != NULL);

    struct CJSON json;
    json.type        = CJSON_ARRAY;
    json.value.array = *value;

    return CJSON_Object_set(object, parser, key, &json);
}

EXTERN_C bool CJSON_Object_set_null(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type       = CJSON_NULL;
    json.value.null = NULL;

    return CJSON_Object_set(object, parser, key, &json);
}

EXTERN_C bool CJSON_Object_set_bool(struct CJSON_Object *const object, struct CJSON_Parser *const parser, const char *const key, const bool value) {
    assert(object != NULL);
    assert(parser != NULL);
    assert(key != NULL);

    struct CJSON json;
    json.type          = CJSON_BOOL;
    json.value.boolean = value;

    return CJSON_Object_set(object, parser, key, &json);
}