#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_OBJECT_H
#define CJSON_OBJECT_H

#define CJSON_OBJECT_MINIMUM_CAPACITY 8U

struct CJSON_Root;
struct CJSON_KV;
struct CJSON_Array;

struct CJSON_Object {
    struct CJSON_KV *entries;
    unsigned         capacity;
};

bool                 CJSON_Object_init       (struct CJSON_Object*, struct CJSON_Root*, unsigned capacity);
struct CJSON_KV     *CJSON_Object_get_entry  (struct CJSON_Object*, struct CJSON_Root*, const char *key);
struct CJSON_KV     *CJSON_Object_find_entry (const struct CJSON_Object*, const char *key);
struct CJSON        *CJSON_Object_get        (const struct CJSON_Object*, const char *key);
bool                 CJSON_Object_set        (struct CJSON_Object*, struct CJSON_Root*, const char *key, const struct CJSON*);
void                 CJSON_Object_delete     (struct CJSON_Object*, const char *key);
const char          *CJSON_Object_get_string (const struct CJSON_Object*, const char *key, bool *success);
double               CJSON_Object_get_float64(const struct CJSON_Object*, const char *key, bool *success);
int64_t              CJSON_Object_get_int64  (const struct CJSON_Object*, const char *key, bool *success);
uint64_t             CJSON_Object_get_uint64 (const struct CJSON_Object*, const char *key, bool *success);
struct CJSON_Object *CJSON_Object_get_object (const struct CJSON_Object*, const char *key, bool *success);
struct CJSON_Array  *CJSON_Object_get_array  (const struct CJSON_Object*, const char *key, bool *success);
void                *CJSON_Object_get_null   (const struct CJSON_Object*, const char *key, bool *success);
bool                 CJSON_Object_get_bool   (const struct CJSON_Object*, const char *key, bool *success);
bool                 CJSON_Object_set_string (struct CJSON_Object*, struct CJSON_Root*, const char *key, const char*);
bool                 CJSON_Object_set_float64(struct CJSON_Object*, struct CJSON_Root*, const char *key, double);
bool                 CJSON_Object_set_int64  (struct CJSON_Object*, struct CJSON_Root*, const char *key, int64_t);
bool                 CJSON_Object_set_uint64 (struct CJSON_Object*, struct CJSON_Root*, const char *key, uint64_t);
bool                 CJSON_Object_set_object (struct CJSON_Object*, struct CJSON_Root*, const char *key, const struct CJSON_Object*);
bool                 CJSON_Object_set_array  (struct CJSON_Object*, struct CJSON_Root*, const char *key, const struct CJSON_Array*);
bool                 CJSON_Object_set_null   (struct CJSON_Object*, struct CJSON_Root*, const char *key);
bool                 CJSON_Object_set_bool   (struct CJSON_Object*, struct CJSON_Root*, const char *key, bool);

#endif

#ifdef __cplusplus
}
#endif