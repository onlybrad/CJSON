#ifndef BENCHMARK_H
#define BENCHMARK_H

typedef struct Benchmark {
    const char *function_name;
    long us_start;
    long us_end;
} Benchmark;

unsigned int Benchmark_start    (const char *const function_name);
void         Benchmark_end      (const unsigned int index);
Benchmark   *Benchmark_get      (const unsigned int index);
void         Benchmark_print    (const unsigned int index);
void         Benchmark_print_all(void);

#ifdef BENCHMARK
#define BENCHMARK_START() const unsigned int benchmark_index = Benchmark_start(__func__)
#define BENCHMARK_END() Benchmark_end(benchmark_index)

#else
#define BENCHMARK_START()
#define BENCHMARK_END(INDEX)
#endif

#endif

