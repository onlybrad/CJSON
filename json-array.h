#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_ARRAY_H
#define CJSON_ARRAY_H

#define CJSON_ARRAY_MINIMUM_CAPACITY  8U

struct CJSON_Root;
struct CJSON;
struct CJSON_Object;

struct CJSON_Array {
    struct CJSON *values;
    unsigned      count,
                  capacity;
};

bool                 CJSON_Array_init       (struct CJSON_Array*, struct CJSON_Root*, unsigned capacity);
struct CJSON        *CJSON_Array_next       (struct CJSON_Array*, struct CJSON_Root*);
struct CJSON        *CJSON_Array_get        (const struct CJSON_Array*, unsigned index);
bool                 CJSON_Array_set        (struct CJSON_Array*, struct CJSON_Root*, unsigned index, const struct CJSON *);
bool                 CJSON_Array_push       (struct CJSON_Array*, struct CJSON_Root*, const struct CJSON*);
const char          *CJSON_Array_get_string (const struct CJSON_Array*, unsigned index, bool *success);
double               CJSON_Array_get_float64(const struct CJSON_Array*, unsigned index, bool *success);
int64_t              CJSON_Array_get_int64  (const struct CJSON_Array*, unsigned index, bool *success);
uint64_t             CJSON_Array_get_uint64 (const struct CJSON_Array*, unsigned index, bool *success);
struct CJSON_Array  *CJSON_Array_get_array  (const struct CJSON_Array*, unsigned index, bool *success);
struct CJSON_Object *CJSON_Array_get_object (const struct CJSON_Array*, unsigned index, bool *success);
void                *CJSON_Array_get_null   (const struct CJSON_Array*, unsigned index, bool *success);
bool                 CJSON_Array_get_bool   (const struct CJSON_Array*, unsigned index, bool *success);
bool                 CJSON_Array_set_string (struct CJSON_Array*, struct CJSON_Root*, unsigned index, const char*);
bool                 CJSON_Array_set_float64(struct CJSON_Array*, struct CJSON_Root*, unsigned index, double);
bool                 CJSON_Array_set_int64  (struct CJSON_Array*, struct CJSON_Root*, unsigned index, int64_t);
bool                 CJSON_Array_set_uint64 (struct CJSON_Array*, struct CJSON_Root*, unsigned index, uint64_t);
bool                 CJSON_Array_set_array  (struct CJSON_Array*, struct CJSON_Root*, unsigned index, const struct CJSON_Array*);
bool                 CJSON_Array_set_object (struct CJSON_Array*, struct CJSON_Root*, unsigned index, const struct CJSON_Object*);
bool                 CJSON_Array_set_null   (struct CJSON_Array*, struct CJSON_Root*, unsigned index);
bool                 CJSON_Array_set_bool   (struct CJSON_Array*, struct CJSON_Root*, unsigned index, bool);

#endif

#ifdef __cplusplus
}
#endif