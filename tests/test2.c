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

    const long start = usec_timestamp();
    struct CJSON_Root root;
    const bool success = CJSON_parse(&root, (const char*)buffer.data, buffer.size);
    if(!success) {
        fputs(CJSON_get_error(&root.json), stderr);
        CJSON_Buffer_free(&buffer);
        return EXIT_FAILURE;
    }

    const long end = usec_timestamp();
    printf("Execution time: %li\n", end - start);

    CJSON_Buffer_free(&buffer);
    CJSON_free(&root);

#ifndef NDEBUG
    const struct CJSON_AllocationStats *allocation_stats = CJSON_get_allocation_stats();
    printf("allocated   times: %i\n", allocation_stats->allocated);
    printf("deallocated times: %i\n", allocation_stats->deallocated);
#endif
    return EXIT_SUCCESS;
}
