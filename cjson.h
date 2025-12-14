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
#include "json-object.h"
#include "json-array.h"

enum CJSON_Error {
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

struct CJSON {
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
    }  type;
    union CJSON_Data {
        struct CJSON_String string;
        double              float64;
        int64_t             int64;
        uint64_t            uint64;
        struct CJSON_Array  array;
        struct CJSON_Object object;
        void               *null;
        bool                boolean;
        enum CJSON_Error    error;
    } data;
};

struct CJSON_Root {
    struct CJSON        json;
    struct CJSON_Tokens tokens;
    struct CJSON_Arena  array_arena,
                        object_arena,
                        string_arena;
};

struct CJSON_KV {
    char        *key;
    struct CJSON value;
};

bool CJSON_init      (struct CJSON_Root*);
bool CJSON_parse     (struct CJSON_Root*, const char *data, unsigned length);
bool CJSON_parse_file(struct CJSON_Root*, const char *path);
void CJSON_free      (struct CJSON_Root*);

struct CJSON_Array  *CJSON_make_array   (struct CJSON*, struct CJSON_Root*);
struct CJSON_Object *CJSON_make_object  (struct CJSON*, struct CJSON_Root*);
const char          *CJSON_get_error    (const struct CJSON*);
struct CJSON        *CJSON_get          (struct CJSON*, const char *query);
const char          *CJSON_get_string   (struct CJSON*, const char *query, bool *success);
double               CJSON_get_float64  (struct CJSON*, const char *query, bool *success);
int64_t              CJSON_get_int64    (struct CJSON*, const char *query, bool *success);
uint64_t             CJSON_get_uint64   (struct CJSON*, const char *query, bool *success);
struct CJSON_Object *CJSON_get_object   (struct CJSON*, const char *query, bool *success);
struct CJSON_Array  *CJSON_get_array    (struct CJSON*, const char *query, bool *success);
void                *CJSON_get_null     (struct CJSON*, const char *query, bool *success);
bool                 CJSON_get_bool     (struct CJSON*, const char *query, bool *success);
bool                 CJSON_set_string   (struct CJSON*, struct CJSON_Root*, const char*);
void                 CJSON_set_float64  (struct CJSON*, double);
void                 CJSON_set_int64    (struct CJSON*, int64_t);
void                 CJSON_set_uint64   (struct CJSON*, uint64_t);
void                 CJSON_set_object   (struct CJSON*, const struct CJSON_Object*);
void                 CJSON_set_array    (struct CJSON*, const struct CJSON_Array*);
void                 CJSON_set_null     (struct CJSON*);
void                 CJSON_set_bool     (struct CJSON*, bool);

#endif

#ifdef __cplusplus
}
#endif
