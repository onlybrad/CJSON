#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_QUERY_BUILDER_H
#define CJSON_QUERY_BUILDER_H

struct CJSON;

struct CJSON_QueryBuilder {
    struct CJSON *json;
};

struct CJSON_QueryBuilder CJSON_get_query_builder(struct CJSON*);
void CJSON_QueryBuilder_key  (struct CJSON_QueryBuilder*, const char *key);
void CJSON_QueryBuilder_index(struct CJSON_QueryBuilder*, unsigned index);

#endif

#ifdef __cplusplus
}
#endif
