#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../cjson.h"
#include "../util.h"
#include "../allocator.h"

int main(void) {
    struct CJSON_Buffer buffer;
    const enum CJSON_UtilError error = file_get_contents("tests\\really-big-json-file.json", &buffer);
    if(error != CJSON_UTIL_ERROR_NONE) {
        return EXIT_FAILURE;
    }

    const unsigned long long start = usec_timestamp();
    struct CJSON_Parser parser;
    const bool success = CJSON_parse(&parser, (const char*)buffer.data, buffer.size);
    if(!success) {
        fputs(CJSON_get_error(&parser), stderr);
        CJSON_Buffer_free(&buffer);
        return EXIT_FAILURE;
    }

    const unsigned long long end = usec_timestamp();
    printf("Execution time: %llu\n", end - start);

    CJSON_Buffer_free(&buffer);
    CJSON_Parser_free(&parser);
    
#ifndef NDEBUG
    const struct CJSON_AllocationStats *allocation_stats = CJSON_get_allocation_stats();
    printf("times allocated on the heap    : %i\n", allocation_stats->allocated);
    printf("times deallocated from the heap: %i\n", allocation_stats->deallocated);
#endif

    return EXIT_SUCCESS;
}
