#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "benchmark.h"

static Benchmark *benchmarks = NULL;
static unsigned int length = 0;
static unsigned int capacity = 0;

typedef struct BenchmarkTime{
    const char *function_name;
    unsigned int count;
    long time;
} BenchmarkTime;

static long usec_timestamp(void) {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    return current_time.tv_sec * 1000000L + current_time.tv_usec;
}

void Benchmark_init(void) {
    capacity = 1 << 12;
    benchmarks = malloc(capacity * sizeof(Benchmark));
    assert(benchmarks != NULL);
}

void Benchmark_free(void) {
    free(benchmarks);
    length = 0;
    capacity = 0;
}

static void Benchmark_resize(const unsigned int new_capacity) {
    Benchmark *new_benchmarks = realloc(benchmarks, (size_t)new_capacity * sizeof(Benchmark));
    assert(new_benchmarks);
    benchmarks = new_benchmarks;
    capacity = new_capacity;
}

unsigned int Benchmark_start(const char *const function_name) {
    if(length == capacity) {
        Benchmark_resize(capacity * 2);
    }

    benchmarks[length] = (Benchmark) {
        .function_name = function_name,
        .us_start = usec_timestamp()
    };

    return length++;
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

static int compareBenchmarkTimes(const void *a, const void *b) {
    const BenchmarkTime *const benchmark_time_a = a;
    const BenchmarkTime *const benchmark_time_b = b;

    return benchmark_time_b->time - benchmark_time_a->time;
}

void Benchmark_print_all(void) {
    Benchmark *benchmarks_copy = malloc(length * sizeof(Benchmark));
    memcpy(benchmarks_copy, benchmarks, length * sizeof(Benchmark));
    unsigned int length_copy = length;
    BenchmarkTime *benchmark_times = malloc(length_copy * sizeof(BenchmarkTime));
    
    unsigned int i;
    for(i = 0U; i < length_copy; i++) {
        benchmark_times[i] = (BenchmarkTime) {
            .function_name = benchmarks_copy[i].function_name,
            .time = benchmarks_copy[i].us_end - benchmarks_copy[i].us_start,
            .count = 1
        };

        for(unsigned int j = i + 1U; j < length_copy; j++) {
            if(strcmp(benchmarks_copy[j].function_name, benchmark_times[i].function_name) == 0) {
                benchmark_times[i].time += benchmarks_copy[j].us_end - benchmarks_copy[j].us_start;
                benchmark_times[i].count++;

                benchmarks_copy[j] = benchmarks_copy[--length_copy];
            }
        }

    }

    qsort(benchmark_times, i + 1, sizeof(BenchmarkTime), compareBenchmarkTimes);

    for(unsigned int j = 0U; j < i; j++) {
        printf("Function %s \n\tCalled %u times\n\tTotal Time (microseconds) %li \n\n", benchmark_times[j].function_name, benchmark_times[j].count, benchmark_times[j].time);
    }

    free(benchmark_times);
    free(benchmarks_copy);
}

