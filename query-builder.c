#include <assert.h>
#include <stdlib.h>

#include "query-builder.h"
#include "json.h"
#include "object.h"
#include "array.h"
#include "util.h"

EXTERN_C struct CJSON_QueryBuilder CJSON_get_query_builder(struct CJSON *json) {
    struct CJSON_QueryBuilder query_builder = {json};

    return query_builder;
}

EXTERN_C void CJSON_QueryBuilder_key(struct CJSON_QueryBuilder *const query_builder, const char *key) {
    assert(query_builder != NULL);

    if(query_builder->json == NULL) {
        return;
    }

    if(query_builder->json->type != CJSON_OBJECT) {
        query_builder->json = NULL;
        return;
    }

    query_builder->json = CJSON_Object_get(&query_builder->json->value.object, key);
}

EXTERN_C void CJSON_QueryBuilder_index(struct CJSON_QueryBuilder *const query_builder, unsigned index) {
    assert(query_builder != NULL);

    if(query_builder->json == NULL) {
        return;
    }

    if(query_builder->json->type != CJSON_ARRAY) {
        query_builder->json = NULL;
        return;
    }

    query_builder->json = CJSON_Array_get(&query_builder->json->value.array, index);
}