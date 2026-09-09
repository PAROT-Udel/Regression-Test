#ifndef BENCHMARK_ADAPTERS_H
#define BENCHMARK_ADAPTERS_H

#include "benchmark_runner.h"

int benchmark_adapter_create(BenchmarkAdapter type,
                             BenchmarkAdapterInstance *instance,
                             char *error,
                             size_t error_size);

void benchmark_adapter_destroy(BenchmarkAdapterInstance *instance);

extern const BenchmarkAdapterVTable polybench_adapter_vtable;
extern const BenchmarkAdapterVTable npb_adapter_vtable;
extern const BenchmarkAdapterVTable generic_adapter_vtable;

#endif
