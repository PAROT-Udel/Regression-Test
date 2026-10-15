#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#include "helper_tests.h"
#include "runner_common.h"

#include <stddef.h>

typedef struct {
    char *id;
    char *description;
    RunnerStringVector aliases;
} KernelSuiteInfo;

typedef struct {
    char *suite;
    char *name;
    char *input;
    TransformationType transform;
    ExpectedOutcome expected;
    char *flags;
} KernelCase;

typedef struct {
    KernelSuiteInfo *suites;
    size_t suite_count;
    KernelCase *cases;
    size_t case_count;
} KernelConfig;

void kernel_config_init(KernelConfig *config);
int kernel_config_load(const char *directory,
                       KernelConfig *config,
                       char *error,
                       size_t error_size);
void kernel_config_free(KernelConfig *config);
int kernel_config_matches_suite(const KernelConfig *config,
                                const KernelCase *kernel,
                                const char *suite_name);

#endif
