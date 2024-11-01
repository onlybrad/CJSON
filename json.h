#ifndef JSON_JSON_H
#define JSON_JSON_H

#include <stdbool.h>
#include <stdint.h>
#include "allocator.h"
#include "token.h"
#include "token-list.h"

typedef union JSON_Data JSON_Data;
typedef struct JSON_Array JSON_Array;
typedef struct JSON_Object JSON_Object;
typedef struct JSON JSON;
typedef struct JSON_Root JSON_Root;
typedef struct JSON_Key_Value JSON_Key_Value;

typedef enum JSON_Type {
    JSON_STRING,
    JSON_FLOAT64,
    JSON_INT64,
    JSON_UINT64,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_NULL,
    JSON_BOOL,
    JSON_ERROR
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
    unsigned length;
    unsigned capacity;
};

struct JSON_Object {
    JSON_Key_Value *data;
    unsigned capacity;
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

//this is the same thing as JSON but contains the tokens. This is the actual type JSON_parse returns. For convienance, JSON_parse returns a JSON*.
struct JSON_Root {
    JSON json;
    JSON_Token_List tokens;
};

struct JSON_Key_Value {
    char *key;
    JSON value;
};

void            JSON_Object_init      (JSON_Object *const object);
void            JSON_Object_free      (JSON_Object *const object);
JSON_Key_Value *JSON_Object_get_entry (JSON_Object *const object, const char *const key);
JSON_Key_Value *JSON_Object_find_entry(const JSON_Object *const object, const char *const key);
JSON           *JSON_Object_get       (const JSON_Object *const object, const char *const key);
void            JSON_Object_set       (JSON_Object *const object, const char *const key, const JSON *const value);
void            JSON_Object_delete    (JSON_Object *const object, const char *const key);

char        *JSON_Object_get_string (const JSON_Object *const object, const char *const key, bool *const success);
double       JSON_Object_get_float64(const JSON_Object *const object, const char *const key, bool *const success);
int64_t      JSON_Object_get_int64  (const JSON_Object *const object, const char *const key, bool *const success);
uint64_t     JSON_Object_get_uint64 (const JSON_Object *const object, const char *const key, bool *const success);
JSON_Object *JSON_Object_get_object (const JSON_Object *const object, const char *const key, bool *const success);
JSON_Array  *JSON_Object_get_array  (const JSON_Object *const object, const char *const key, bool *const success);
void        *JSON_Object_get_null   (const JSON_Object *const object, const char *const key, bool *const success);
bool         JSON_Object_get_bool   (const JSON_Object *const object, const char *const key, bool *const success);

void  JSON_Array_init(JSON_Array *const array);
void  JSON_Array_free(JSON_Array *const array);
JSON *JSON_Array_next(JSON_Array *const array);
JSON *JSON_Array_get (const JSON_Array *const array, const unsigned index);
void JSON_Array_set  (JSON_Array *const array, const unsigned index, const JSON *const value);
void JSON_Array_push (JSON_Array *const array, const JSON *const value);

char         *JSON_Array_get_string(const JSON_Array *const array, const unsigned index, bool *const success);
double       JSON_Array_get_float64(const JSON_Array *const array, const unsigned index, bool *const success);
int64_t      JSON_Array_get_int64  (const JSON_Array *const array, const unsigned index, bool *const success);
uint64_t     JSON_Array_get_uint64 (const JSON_Array *const array, const unsigned index, bool *const success);
JSON_Array   *JSON_Array_get_array (const JSON_Array *const array, const unsigned index, bool *const success);
JSON_Object  *JSON_Array_get_object(const JSON_Array *const array, const unsigned index, bool *const success);
void        *JSON_Array_get_null   (const JSON_Array *const array, const unsigned index, bool *const success);
bool         JSON_Array_get_bool   (const JSON_Array *const array, const unsigned index, bool *const success);

JSON        *JSON_parse         (const char *const data, const unsigned length);
JSON        *JSON_parse_file    (const char *const path);
void         JSON_free          (JSON *const json);
const char  *JSON_get_error     (const JSON *const json);
JSON        *JSON_get           (JSON *json, const char *query);
char        *JSON_get_string    (JSON *const json, const char *query, bool *const success);
double       JSON_get_float64   (JSON *const json, const char *query, bool *const success);
int64_t      JSON_get_int64     (JSON *const json, const char *query, bool *const success);
uint64_t     JSON_get_uint64    (JSON *const json, const char *query, bool *const success);
JSON_Object *JSON_get_object    (JSON *const json, const char *query, bool *const success);
JSON_Array  *JSON_get_array     (JSON *const json, const char *query, bool *const success);
void        *JSON_get_null      (JSON *const json, const char *query, bool *const success);
bool         JSON_get_bool      (JSON *const json, const char *query, bool *const success);

void _JSON_free(JSON *const json);
#endif