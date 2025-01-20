#include <sys/time.h>
#include <stdio.h>
#include "benchmark.h"

#ifndef MAXIMUM_FUNCTION_COUNT
#define MAXIMUM_FUNCTION_COUNT 1 << 11
#endif

static Benchmark benchmarks[MAXIMUM_FUNCTION_COUNT] = {0};
static unsigned int current_index = 0;

static long usec_timestamp(void) {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    return current_time.tv_sec * 1000000L + current_time.tv_usec;
}

unsigned int Benchmark_start(const char *const function_name) {
    benchmarks[current_index] = (Benchmark) {
        .function_name = function_name,
        .us_start = usec_timestamp()
    };

    return current_index++;
}

void Benchmark_end(const unsigned int index) {
    benchmarks[index].us_end = usec_timestamp();
}

Benchmark *Benchmark_get(const unsigned int index) {
    return benchmarks + index;
}

void Benchmark_print(const unsigned int index) {
    printf("Function %s took %li microseconds.\n", benchmarks[index].function_name, benchmarks[index].us_end - benchmarks[index].us_start);
}

void Benchmark_print_all(void) {
    for(unsigned int i = 0U; i < current_index; i++) {
        Benchmark_print(i);
    }
}

