#include "benchmark_adapters.h"

#include <stdio.h>
#include <string.h>

int benchmark_adapter_create(BenchmarkAdapter type,
                             BenchmarkAdapterInstance *instance,
                             char *error,
                             size_t error_size)
{
    memset(instance, 0, sizeof(*instance));
    switch (type) {
    case BENCHMARK_ADAPTER_POLYBENCH:
        instance->vtable = &polybench_adapter_vtable;
        return 0;
    case BENCHMARK_ADAPTER_NPB:
        instance->vtable = &npb_adapter_vtable;
        return 0;
    case BENCHMARK_ADAPTER_GENERIC:
        instance->vtable = &generic_adapter_vtable;
        return 0;
    default:
        if (error != NULL && error_size != 0)
            snprintf(error, error_size, "unknown adapter type %d", (int)type);
        return -1;
    }
}

void benchmark_adapter_destroy(BenchmarkAdapterInstance *instance)
{
    instance->vtable = NULL;
    instance->context = NULL;
}
