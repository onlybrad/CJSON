#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "benchmark.h"

typedef struct Benchmark {
    const char *function_name;
    long us_start;
    long us_end;
} Benchmark;
typedef struct BenchmarkStats {
    const char *function_name;
    unsigned int count;
    long total_time;
    long max_time;
    long min_time;
} BenchmarkStats;

static Benchmark *benchmark_timestamps = NULL;
static unsigned int length = 0;
static unsigned int capacity = 0;

static long usec_timestamp(void) {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    return current_time.tv_sec * 1000000L + current_time.tv_usec;
}

void Benchmark_init(void) {
    capacity = 1 << 12;
    benchmark_timestamps = malloc((size_t)capacity * sizeof(Benchmark));
    assert(benchmark_timestamps != NULL);
}

void Benchmark_free(void) {
    free(benchmark_timestamps);
    length = 0;
    capacity = 0;
}

static void Benchmark_resize(const unsigned int new_capacity) {
    Benchmark *new_benchmark_timestamps = realloc(benchmark_timestamps, (size_t)new_capacity * sizeof(Benchmark));
    assert(new_benchmark_timestamps);
    benchmark_timestamps = new_benchmark_timestamps;
    capacity = new_capacity;
}

unsigned int Benchmark_start(const char *const function_name) {
    if(length == capacity) {
        Benchmark_resize(capacity * 2);
    }

    benchmark_timestamps[length] = (Benchmark) {
        .function_name = function_name,
        .us_start = usec_timestamp()
    };

    return length++;
}

void Benchmark_end(const unsigned int index) {
    benchmark_timestamps[index].us_end = usec_timestamp();
}

void Benchmark_print(const unsigned int index) {
    printf("Function %s took %li microseconds.\n", benchmark_timestamps[index].function_name, benchmark_timestamps[index].us_end - benchmark_timestamps[index].us_start);
}

static int compare_benchmark_times(const void *a, const void *b) {
    const BenchmarkStats *const benchmark_time_a = a;
    const BenchmarkStats *const benchmark_time_b = b;

    return benchmark_time_b->max_time - benchmark_time_a->max_time;
}

void Benchmark_print_all(void) {
    Benchmark *benchmarks_copy = malloc(length * sizeof(Benchmark));
    memcpy(benchmarks_copy, benchmark_timestamps, length * sizeof(Benchmark));
    unsigned int length_copy = length;
    BenchmarkStats *stats = malloc(length_copy * sizeof(BenchmarkStats));
    
    unsigned int i;
    for(i = 0U; i < length_copy; i++) {
        stats[i] = (BenchmarkStats) {
            .function_name = benchmarks_copy[i].function_name,
            .total_time = benchmarks_copy[i].us_end - benchmarks_copy[i].us_start,
            .count = 1
        };
        stats[i].max_time = stats[i].min_time = stats[i].total_time;

        for(unsigned int j = i + 1U; j < length_copy; j++) {
            if(strcmp(benchmarks_copy[j].function_name, stats[i].function_name) == 0) {
                const long current_time = benchmarks_copy[j].us_end - benchmarks_copy[j].us_start;

                stats[i].total_time += current_time;
                stats[i].count++;

                if(current_time > stats[i].max_time) {
                    stats[i].max_time = current_time;
                } else if(current_time < stats[i].min_time) {
                    stats[i].min_time = current_time;
                }

                benchmarks_copy[j] = benchmarks_copy[--length_copy];
                j--;
            }
        }

    }

    qsort(stats, i, sizeof(BenchmarkStats), compare_benchmark_times);

    for(unsigned int j = 0U; j < i; j++) {
        printf("Function %s \n" 
                "\tCall count: %u\n"
                "\tTotal time: %lius\n"
                "\tMinimum time: %lius \n"
                "\tMaximum time: %lius \n"
                "\tAverage time: %.2fus \n\n",
            stats[j].function_name,
            stats[j].count,
            stats[j].total_time,
            stats[j].min_time,
            stats[j].max_time,
            (double)stats[j].total_time / (double)stats[j].count
        );
    }

    free(stats);
    free(benchmarks_copy);
}

