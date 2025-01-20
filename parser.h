#ifndef JSON_JSON_H
#define JSON_JSON_H

#include <stdbool.h>
#include <stdint.h>
#include "allocator.h"
#include "token.h"
#include "tokens.h"

typedef union JSON_Data JSON_Data;
typedef struct JSON_Array JSON_Array;
typedef struct JSON_Object JSON_Object;
typedef struct JSON JSON;
typedef struct JSON_Key_Value JSON_Key_Value;

typedef enum JSON_Type {
    JSON_ERROR,
    JSON_STRING,
    JSON_FLOAT64,
    JSON_INT64,
    JSON_UINT64,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_NULL,
    JSON_BOOL
} JSON_Type;

typedef enum JSON_Error {
    JSON_TOKEN_ERROR,
    JSON_STRING_FAILED_TO_PARSE,
    JSON_FLOAT64_FAILED_TO_PARSE,
    JSON_INT64_FAILED_TO_PARSE,
    JSON_UINT64_FAILED_TO_PARSE,
    JSON_OBJECT_FAILED_TO_PARSE,
    JSON_OBJECT_INVALID_VALUE,
    JSON_OBJECT_INVALID_KEY,
    JSON_OBJECT_MISSING_COLON,
    JSON_OBJECT_MISSING_COMMA_OR_RCURLY,
    JSON_ARRAY_FAILED_TO_PARSE,
    JSON_ARRAY_MISSING_COMMA_OR_RBRACKET,
    JSON_ARRAY_INVALID_VALUE,
    JSON_FAILED_TO_OPEN_FILE
} JSON_Error;

struct JSON_Array {
    JSON *data;
    unsigned int length;
    unsigned int capacity;
};

struct JSON_Object {
    JSON_Key_Value *data;
    unsigned int capacity;
};

union JSON_Data {
    char        *string;
    double       float64;
    int64_t      int64;
    uint64_t     uint64;
    JSON_Array   array;
    JSON_Object  object;
    void        *null;
    bool         boolean;
    JSON_Error   error;
};

struct JSON {
    JSON_Data value;
    JSON_Type type;
};

struct JSON_Key_Value {
    char *key;
    JSON value;
};

void            JSON_Object_init       (JSON_Object *const object);
void            JSON_Object_free       (JSON_Object *const object);
JSON_Key_Value *JSON_Object_get_entry  (JSON_Object *const object, const char *const key);
JSON_Key_Value *JSON_Object_find_entry (const JSON_Object *const object, const char *const key);
JSON           *JSON_Object_get        (const JSON_Object *const object, const char *const key);
void            JSON_Object_set        (JSON_Object *const object, const char *const key, const JSON *const value);
void            JSON_Object_delete     (JSON_Object *const object, const char *const key);
char           *JSON_Object_get_string (const JSON_Object *const object, const char *const key, bool *const success);
double          JSON_Object_get_float64(const JSON_Object *const object, const char *const key, bool *const success);
int64_t         JSON_Object_get_int64  (const JSON_Object *const object, const char *const key, bool *const success);
uint64_t        JSON_Object_get_uint64 (const JSON_Object *const object, const char *const key, bool *const success);
JSON_Object    *JSON_Object_get_object (const JSON_Object *const object, const char *const key, bool *const success);
JSON_Array     *JSON_Object_get_array  (const JSON_Object *const object, const char *const key, bool *const success);
void           *JSON_Object_get_null   (const JSON_Object *const object, const char *const key, bool *const success);
bool            JSON_Object_get_bool   (const JSON_Object *const object, const char *const key, bool *const success);
void            JSON_Object_set_string (JSON_Object *const object, const char *const key, const char *const value);
void            JSON_Object_set_float64(JSON_Object *const object, const char *const key, const double value);
void            JSON_Object_set_int64  (JSON_Object *const object, const char *const key, const int64_t value);
void            JSON_Object_set_uint64 (JSON_Object *const object, const char *const key, const uint64_t value);
void            JSON_Object_set_object (JSON_Object *const object, const char *const key, const JSON_Object *const value);
void            JSON_Object_set_array  (JSON_Object *const object, const char *const key, const JSON_Array *const value);
void            JSON_Object_set_null   (JSON_Object *const object, const char *const key);
void            JSON_Object_set_bool   (JSON_Object *const object, const char *const key, const bool value);

void         JSON_Array_init       (JSON_Array *const array);
void         JSON_Array_free       (JSON_Array *const array);
JSON        *JSON_Array_next       (JSON_Array *const array);
JSON        *JSON_Array_get        (const JSON_Array *const array, const unsigned int index);
void         JSON_Array_set        (JSON_Array *const array, const unsigned int index, const JSON *const value);
void         JSON_Array_push       (JSON_Array *const array, const JSON *const value);
char        *JSON_Array_get_string (const JSON_Array *const array, const unsigned int index, bool *const success);
double       JSON_Array_get_float64(const JSON_Array *const array, const unsigned int index, bool *const success);
int64_t      JSON_Array_get_int64  (const JSON_Array *const array, const unsigned int index, bool *const success);
uint64_t     JSON_Array_get_uint64 (const JSON_Array *const array, const unsigned int index, bool *const success);
JSON_Array  *JSON_Array_get_array  (const JSON_Array *const array, const unsigned int index, bool *const success);
JSON_Object *JSON_Array_get_object (const JSON_Array *const array, const unsigned int index, bool *const success);
void        *JSON_Array_get_null   (const JSON_Array *const array, const unsigned int index, bool *const success);
bool         JSON_Array_get_bool   (const JSON_Array *const array, const unsigned int index, bool *const success);
void         JSON_Array_set_string (JSON_Array *const array, const unsigned int index, const char *const value);
void         JSON_Array_set_float64(JSON_Array *const array, const unsigned int index, const double value);
void         JSON_Array_set_int64  (JSON_Array *const array, const unsigned int index, const int64_t value);
void         JSON_Array_set_uint64 (JSON_Array *const array, const unsigned int index, const uint64_t value);
void         JSON_Array_set_array  (JSON_Array *const array, const unsigned int index, const JSON_Array *const value);
void         JSON_Array_set_object (JSON_Array *const array, const unsigned int index, const JSON_Object *const value);
void         JSON_Array_set_null   (JSON_Array *const array, const unsigned int index);
void         JSON_Array_set_bool   (JSON_Array *const array, const unsigned int index, const bool value);

JSON        *JSON_init       (void);
JSON_Array  *JSON_make_array (JSON *const json);
JSON_Object *JSON_make_object(JSON *const json);
JSON        *JSON_parse      (const char *const data, const unsigned int length);
JSON        *JSON_parse_file (const char *const path);
void         JSON_free       (JSON *const json);
const char  *JSON_get_error  (const JSON *const json);
JSON        *JSON_get        (JSON *json, const char *query);
char        *JSON_get_string (JSON *const json, const char *query, bool *const success);
double       JSON_get_float64(JSON *const json, const char *query, bool *const success);
int64_t      JSON_get_int64  (JSON *const json, const char *query, bool *const success);
uint64_t     JSON_get_uint64 (JSON *const json, const char *query, bool *const success);
JSON_Object *JSON_get_object (JSON *const json, const char *query, bool *const success);
JSON_Array  *JSON_get_array  (JSON *const json, const char *query, bool *const success);
void        *JSON_get_null   (JSON *const json, const char *query, bool *const success);
bool         JSON_get_bool   (JSON *const json, const char *query, bool *const success);
void         JSON_set_string (JSON *const json, const char *const value);
void         JSON_set_float64(JSON *const json, const double value);
void         JSON_set_int64  (JSON *const json, const int64_t value);
void         JSON_set_uint64 (JSON *const json, const uint64_t value);
void         JSON_set_object (JSON *const json, const JSON_Object *const value);
void         JSON_set_array  (JSON *const json, const JSON_Array *const value);
void         JSON_set_null   (JSON *const json);
void         JSON_set_bool   (JSON *const json, const bool value);

void _JSON_free(JSON *const json);
#endif
