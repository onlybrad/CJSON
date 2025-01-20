#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../parser.h"
#include "../util.h"

#define BENCHMARK
#include "../benchmark.h"

#ifdef COUNT_ALLOCATIONS
int malloced = 0;
int freed = 0;
#endif

int main(void) {
    size_t filesize;
    char *const data = file_get_contents("E:\\code\\c\\json\\tests\\really-big-json-file.json", &filesize);

    BENCHMARK_START();
    JSON *const json = JSON_parse(data, (unsigned int)filesize);
    BENCHMARK_END();

    free(data);
    JSON_free(json);

    Benchmark_print_all();

    #ifdef COUNT_ALLOCATIONS
    printf("malloced = %i\n", malloced);
    printf("freed = %i\n", freed);
    #endif

    return 0;
}
