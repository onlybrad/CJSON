#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_JSON_H
#define CJSON_JSON_H

#include <stdbool.h>
#include <stdint.h>
#include "allocator.h"
#include "token.h"
#include "tokens.h"
#include "object.h"
#include "array.h"

struct CJSON_Parser;

enum CJSON_Error {
    CJSON_ERROR_NONE,
    CJSON_ERROR_TOKEN,
    CJSON_ERROR_STRING,
    CJSON_ERROR_FLOAT64,
    CJSON_ERROR_INT64,
    CJSON_ERROR_UINT64,
    CJSON_ERROR_OBJECT,
    CJSON_ERROR_OBJECT_VALUE,
    CJSON_ERROR_OBJECT_KEY,
    CJSON_ERROR_ARRAY,
    CJSON_ERROR_ARRAY_VALUE,
    CJSON_ERROR_MISSING_COLON,
    CJSON_ERROR_MISSING_COMMA_OR_RCURLY,
    CJSON_ERROR_FILE,
    CJSON_ERROR_MEMORY
};

struct CJSON_String {
    char    *chars;
    unsigned length;
};

enum CJSON_Type {
    CJSON_ERROR,
    CJSON_STRING,
    CJSON_FLOAT64,
    CJSON_INT64,
    CJSON_UINT64,
    CJSON_ARRAY,
    CJSON_OBJECT,
    CJSON_NULL,
    CJSON_BOOL
};

union CJSON_Value {
    struct CJSON_String string;
    double              float64;
    int64_t             int64;
    uint64_t            uint64;
    struct CJSON_Array  array;
    struct CJSON_Object object;
    void               *null;
    bool                boolean;
    enum CJSON_Error    error;
};

struct CJSON {
    enum CJSON_Type   type;
    union CJSON_Value value;
};

struct CJSON_KV {
    char        *key;
    struct CJSON value;
};

struct CJSON_Array  *CJSON_make_array (struct CJSON*, struct CJSON_Parser*);
struct CJSON_Object *CJSON_make_object(struct CJSON*, struct CJSON_Parser*);
struct CJSON        *CJSON_get        (struct CJSON*, const char *query);
const char          *CJSON_get_string (struct CJSON*, const char *query, bool *success);
double               CJSON_get_float64(struct CJSON*, const char *query, bool *success);
int64_t              CJSON_get_int64  (struct CJSON*, const char *query, bool *success);
uint64_t             CJSON_get_uint64 (struct CJSON*, const char *query, bool *success);
struct CJSON_Object *CJSON_get_object (struct CJSON*, const char *query, bool *success);
struct CJSON_Array  *CJSON_get_array  (struct CJSON*, const char *query, bool *success);
void                *CJSON_get_null   (struct CJSON*, const char *query, bool *success);
bool                 CJSON_get_bool   (struct CJSON*, const char *query, bool *success);
const char          *CJSON_as_string  (struct CJSON*, bool *success);
double               CJSON_as_float64 (struct CJSON*, bool *success);
int64_t              CJSON_as_int64   (struct CJSON*, bool *success);
uint64_t             CJSON_as_uint64  (struct CJSON*, bool *success);
struct CJSON_Object *CJSON_as_object  (struct CJSON*, bool *success);
struct CJSON_Array  *CJSON_as_array   (struct CJSON*, bool *success);
void                *CJSON_as_null    (struct CJSON*, bool *success);
bool                 CJSON_as_bool    (struct CJSON*, bool *success);
bool                 CJSON_set_string (struct CJSON*, struct CJSON_Parser*, const char*);
void                 CJSON_set_float64(struct CJSON*, double);
void                 CJSON_set_int64  (struct CJSON*, int64_t);
void                 CJSON_set_uint64 (struct CJSON*, uint64_t);
void                 CJSON_set_object (struct CJSON*, const struct CJSON_Object*);
void                 CJSON_set_array  (struct CJSON*, const struct CJSON_Array*);
void                 CJSON_set_null   (struct CJSON*);
void                 CJSON_set_bool   (struct CJSON*, bool);

#endif

#ifdef __cplusplus
}
#endif
