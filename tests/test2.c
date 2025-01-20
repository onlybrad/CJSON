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
    Benchmark_init();

    size_t filesize;
    char *const data = file_get_contents("E:\\code\\c\\json\\tests\\really-big-json-file.json", &filesize);

    JSON *const json = JSON_parse(data, (unsigned int)filesize);

    free(data);
    JSON_free(json);

    Benchmark_print_all();

    #ifdef COUNT_ALLOCATIONS
    printf("malloced = %i\n", malloced);
    printf("freed = %i\n", freed);
    #endif

    Benchmark_free();
    return 0;
}
