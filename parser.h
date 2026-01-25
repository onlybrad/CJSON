#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_PARSER_H
#define CJSON_PARSER_H

#include "counters.h"
#include "json.h"
#include "tokens.h"
#include "allocator.h"
struct CJSON_Parser {
    enum   CJSON_Error error;
    struct CJSON_Arena array_arena,
                       object_arena,
                       string_arena,
                       json_arena;
};

void          CJSON_Parser_init(struct CJSON_Parser*);
void          CJSON_Parser_free(struct CJSON_Parser*);
struct CJSON *CJSON_new        (struct CJSON_Parser*);
struct CJSON *CJSON_parse      (struct CJSON_Parser*, const char *data, unsigned length);
struct CJSON *CJSON_parse_file (struct CJSON_Parser*, const char *path);

const char *CJSON_get_error(const struct CJSON_Parser*);

#endif

#ifdef __cplusplus
}
#endif