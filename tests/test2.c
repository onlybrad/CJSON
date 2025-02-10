#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../parser.h"
#include "../util.h"

#include "../benchmark.h"

#ifdef COUNT_ALLOCATIONS
int malloced = 0;
int freed = 0;
#endif

int main(void) {
#ifdef BENCHMARK
    Benchmark_init();
#endif

    size_t filesize;
    char *const data = file_get_contents("tests\\really-big-json-file.json", &filesize);

#ifdef BENCHMARK
    BENCHMARK_START();
    CJSON_JSON *const json = CJSON_parse(data, (unsigned int)filesize);
    BENCHMARK_END();
#else
    const long start = usec_timestamp();
    CJSON_JSON *const json = CJSON_parse(data, (unsigned int)filesize);
    const long end = usec_timestamp();
    printf("Execution time: %li", end - start);
#endif

    free(data);
    CJSON_free(json);

#ifdef COUNT_ALLOCATIONS
    printf("malloced = %i\n", malloced);
    printf("freed = %i\n", freed);
#endif

#ifdef BENCHMARK
    Benchmark_print_all();
    Benchmark_free();
#endif

    return 0;
}
