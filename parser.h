#ifndef CJSON_JSON_H
#define CJSON_JSON_H

#include <stdbool.h>
#include <stdint.h>
#include "allocator.h"
#include "token.h"
#include "tokens.h"

typedef union CJSON_Data CJSON_Data;
typedef struct CJSON_Array CJSON_Array;
typedef struct CJSON_Object CJSON_Object;
typedef struct CJSON_Node CJSON_Node;
typedef struct CJSON CJSON;
typedef struct CJSON_Key_Value CJSON_Key_Value;

typedef enum CJSON_Type {
    CJSON_ERROR,
    CJSON_STRING,
    CJSON_FLOAT64,
    CJSON_INT64,
    CJSON_UINT64,
    CJSON_ARRAY,
    CJSON_OBJECT,
    CJSON_NULL,
    CJSON_BOOL
} CJSON_Type;

typedef enum CJSON_Error {
    CJSON_TOKEN_ERROR,
    CJSON_STRING_FAILED_TO_PARSE,
    CJSON_FLOAT64_FAILED_TO_PARSE,
    CJSON_INT64_FAILED_TO_PARSE,
    CJSON_UINT64_FAILED_TO_PARSE,
    CJSON_OBJECT_FAILED_TO_PARSE,
    CJSON_OBJECT_INVALID_VALUE,
    CJSON_OBJECT_INVALID_KEY,
    CJSON_OBJECT_MISSING_COLON,
    CJSON_OBJECT_MISSING_COMMA_OR_RCURLY,
    CJSON_ARRAY_FAILED_TO_PARSE,
    CJSON_ARRAY_MISSING_COMMA_OR_RBRACKET,
    CJSON_ARRAY_INVALID_VALUE,
    CJSON_FAILED_TO_OPEN_FILE
} CJSON_Error;

struct CJSON_Array {
    CJSON_Node *nodes;
    unsigned int length;
    unsigned int capacity;
};

struct CJSON_Object {
    CJSON_Key_Value *nodes;
    unsigned int capacity;
};

union CJSON_Data {
    char        *string;
    double       float64;
    int64_t      int64;
    uint64_t     uint64;
    CJSON_Array  array;
    CJSON_Object object;
    void         *null;
    bool         boolean;
    CJSON_Error  error;
};

struct CJSON_Node {
    CJSON_Data value;
    CJSON_Type type;
};

struct CJSON {
    CJSON_Node node;
    CJSON_Tokens tokens;
};

struct CJSON_Key_Value {
    char *key;
    CJSON_Node value;
};

void             CJSON_Object_init       (CJSON_Object *const object);
void             CJSON_Object_free       (CJSON_Object *const object);
CJSON_Key_Value *CJSON_Object_get_entry  (CJSON_Object *const object, const char *const key);
CJSON_Key_Value *CJSON_Object_find_entry (const CJSON_Object *const object, const char *const key);
CJSON_Node      *CJSON_Object_get        (const CJSON_Object *const object, const char *const key);
void             CJSON_Object_set        (CJSON_Object *const object, const char *const key, const CJSON_Node *const value);
void             CJSON_Object_delete     (CJSON_Object *const object, const char *const key);
char            *CJSON_Object_get_string (const CJSON_Object *const object, const char *const key, bool *const success);
double           CJSON_Object_get_float64(const CJSON_Object *const object, const char *const key, bool *const success);
int64_t          CJSON_Object_get_int64  (const CJSON_Object *const object, const char *const key, bool *const success);
uint64_t         CJSON_Object_get_uint64 (const CJSON_Object *const object, const char *const key, bool *const success);
CJSON_Object    *CJSON_Object_get_object (const CJSON_Object *const object, const char *const key, bool *const success);
CJSON_Array     *CJSON_Object_get_array  (const CJSON_Object *const object, const char *const key, bool *const success);
void            *CJSON_Object_get_null   (const CJSON_Object *const object, const char *const key, bool *const success);
bool             CJSON_Object_get_bool   (const CJSON_Object *const object, const char *const key, bool *const success);
void             CJSON_Object_set_string (CJSON_Object *const object, const char *const key, const char *const value);
void             CJSON_Object_set_float64(CJSON_Object *const object, const char *const key, const double value);
void             CJSON_Object_set_int64  (CJSON_Object *const object, const char *const key, const int64_t value);
void             CJSON_Object_set_uint64 (CJSON_Object *const object, const char *const key, const uint64_t value);
void             CJSON_Object_set_object (CJSON_Object *const object, const char *const key, const CJSON_Object *const value);
void             CJSON_Object_set_array  (CJSON_Object *const object, const char *const key, const CJSON_Array *const value);
void             CJSON_Object_set_null   (CJSON_Object *const object, const char *const key);
void             CJSON_Object_set_bool   (CJSON_Object *const object, const char *const key, const bool value);

void               CJSON_Array_init       (CJSON_Array *const array);
void               CJSON_Array_free       (CJSON_Array *const array);
CJSON_Node        *CJSON_Array_next       (CJSON_Array *const array);
CJSON_Node        *CJSON_Array_get        (const CJSON_Array *const array, const unsigned int index);
void               CJSON_Array_set        (CJSON_Array *const array, const unsigned int index, const CJSON_Node *const value);
void               CJSON_Array_push       (CJSON_Array *const array, const CJSON_Node *const value);
char              *CJSON_Array_get_string (const CJSON_Array *const array, const unsigned int index, bool *const success);
double             CJSON_Array_get_float64(const CJSON_Array *const array, const unsigned int index, bool *const success);
int64_t            CJSON_Array_get_int64  (const CJSON_Array *const array, const unsigned int index, bool *const success);
uint64_t           CJSON_Array_get_uint64 (const CJSON_Array *const array, const unsigned int index, bool *const success);
CJSON_Array       *CJSON_Array_get_array  (const CJSON_Array *const array, const unsigned int index, bool *const success);
CJSON_Object      *CJSON_Array_get_object (const CJSON_Array *const array, const unsigned int index, bool *const success);
void              *CJSON_Array_get_null   (const CJSON_Array *const array, const unsigned int index, bool *const success);
bool               CJSON_Array_get_bool   (const CJSON_Array *const array, const unsigned int index, bool *const success);
void               CJSON_Array_set_string (CJSON_Array *const array, const unsigned int index, const char *const value);
void               CJSON_Array_set_float64(CJSON_Array *const array, const unsigned int index, const double value);
void               CJSON_Array_set_int64  (CJSON_Array *const array, const unsigned int index, const int64_t value);
void               CJSON_Array_set_uint64 (CJSON_Array *const array, const unsigned int index, const uint64_t value);
void               CJSON_Array_set_array  (CJSON_Array *const array, const unsigned int index, const CJSON_Array *const value);
void               CJSON_Array_set_object (CJSON_Array *const array, const unsigned int index, const CJSON_Object *const value);
void               CJSON_Array_set_null   (CJSON_Array *const array, const unsigned int index);
void               CJSON_Array_set_bool   (CJSON_Array *const array, const unsigned int index, const bool value);

CJSON *CJSON_init      (void);
CJSON *CJSON_parse     (const char *const data, const unsigned int length);
CJSON *CJSON_parse_file(const char *const path);
void   CJSON_free      (CJSON *const json);

void          CJSON_Node_free  (CJSON_Node *const node);
CJSON_Array  *CJSON_make_array (CJSON_Node *const node);
CJSON_Object *CJSON_make_object(CJSON_Node *const node);
const char   *CJSON_get_error  (const CJSON_Node *const node);
CJSON_Node   *CJSON_get        (CJSON_Node *node, const char *query);
char         *CJSON_get_string (CJSON_Node *const node, const char *query, bool *const success);
double        CJSON_get_float64(CJSON_Node *const node, const char *query, bool *const success);
int64_t       CJSON_get_int64  (CJSON_Node *const node, const char *query, bool *const success);
uint64_t      CJSON_get_uint64 (CJSON_Node *const node, const char *query, bool *const success);
CJSON_Object *CJSON_get_object (CJSON_Node *const node, const char *query, bool *const success);
CJSON_Array  *CJSON_get_array  (CJSON_Node *const node, const char *query, bool *const success);
void         *CJSON_get_null   (CJSON_Node *const node, const char *query, bool *const success);
bool          CJSON_get_bool   (CJSON_Node *const node, const char *query, bool *const success);
void          CJSON_set_string (CJSON_Node *const node, const char *const value);
void          CJSON_set_float64(CJSON_Node *const node, const double value);
void          CJSON_set_int64  (CJSON_Node *const node, const int64_t value);
void          CJSON_set_uint64 (CJSON_Node *const node, const uint64_t value);
void          CJSON_set_object (CJSON_Node *const node, const CJSON_Object *const value);
void          CJSON_set_array  (CJSON_Node *const node, const CJSON_Array *const value);
void          CJSON_set_null   (CJSON_Node *const node);
void          CJSON_set_bool   (CJSON_Node *const node, const bool value);

#endif
