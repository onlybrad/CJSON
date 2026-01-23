#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../cjson.h"
#include "../util.h"
#include "../file.h"
#include "../allocator.h"

int main(void) {
    struct CJSON_FileContents file_contents;
    CJSON_FileContents_init(&file_contents);

    const enum CJSON_FileContents_Error error = CJSON_FileContents_get(&file_contents, "tests/really-big-json-file.json");
    if(error != CJSON_FILECONTENTS_ERROR_NONE) {
        return EXIT_FAILURE;
    }

    const uint64_t start = CJSON_usec_timestamp();
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    if(CJSON_parse(&parser, (const char*)file_contents.data, file_contents.size) == NULL) {
        fputs(CJSON_get_error(&parser), stderr);
        CJSON_FileContents_free(&file_contents);
        return EXIT_FAILURE;
    }
    const uint64_t end = CJSON_usec_timestamp();
    printf("Parsing time: %" PRIu64 " microseconds\n", end - start);

    CJSON_FileContents_free(&file_contents);
    CJSON_Parser_free(&parser);
    
#ifndef NDEBUG
    const struct CJSON_AllocationStats *allocation_stats = CJSON_get_allocation_stats();
    printf("times allocated on the heap     : %i\n", allocation_stats->allocated);
    printf("times deallocated from the heap : %i\n", allocation_stats->deallocated);
#endif

    return EXIT_SUCCESS;
}
