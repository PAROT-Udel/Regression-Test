#ifndef BENCHMARK_CONFIG_H
#define BENCHMARK_CONFIG_H

#include "runner_common.h"

#include <stddef.h>

typedef enum {
    BENCHMARK_ADAPTER_POLYBENCH,
    BENCHMARK_ADAPTER_NPB,
    BENCHMARK_ADAPTER_GENERIC
} BenchmarkAdapter;

typedef enum {
    BENCHMARK_COMMAND_NONE,
    BENCHMARK_COMMAND_SHELL,
    BENCHMARK_COMMAND_ARGV
} BenchmarkCommandType;

typedef struct {
    BenchmarkCommandType type;
    char *shell;
    RunnerStringVector argv;
} BenchmarkCommand;

typedef enum {
    BENCHMARK_VERIFY_NONE,
    BENCHMARK_VERIFY_EXIT_CODE,
    BENCHMARK_VERIFY_REGEX,
    BENCHMARK_VERIFY_DUMP_DIFF
} BenchmarkVerifyType;

typedef enum {
    BENCHMARK_DUMP_STREAM_STDERR,
    BENCHMARK_DUMP_STREAM_STDOUT
} BenchmarkDumpStream;

typedef struct {
    int present;
    BenchmarkVerifyType type;
    int expect;
    char *pattern;
    BenchmarkCommand run;
    BenchmarkCommand baseline;
    BenchmarkDumpStream dump_stream;
} BenchmarkVerify;

typedef struct {
    int has_preprocessor;
    char *preprocessor;
    int has_copy;
    RunnerStringVector copy;
    int has_cetus_inputs;
    RunnerStringVector cetus_inputs;
    int has_inject_map;
    RunnerKeyValueMap inject_map;
    int has_compile;
    BenchmarkCommand compile;
    int has_run;
    BenchmarkCommand run;
    int has_verify;
    BenchmarkVerify verify;
} BenchmarkGenericData;

typedef struct {
    char *name;
    char *description;
    RunnerStringVector flags;
} BenchmarkProfile;

typedef struct {
    char *name;
    char *rel_path;
    char *bench;
    char *binary;
    RunnerStringVector headers;
    RunnerStringVector sources;
    RunnerKeyValueMap inject_map;
    char *xfail;
    RunnerKeyValueMap xfail_profiles;
    char *root_env;
    char *src_file;
    char *header_file;
    RunnerStringVector cetus_sources;
    BenchmarkGenericData generic;
} BenchmarkKernel;

typedef struct {
    char *id;
    BenchmarkAdapter adapter;
    char *adapter_name;
    char *description;
    int is_default;
    char *root_env;
    char *dataset;
    char *npb_class;
    RunnerStringVector profiles;
    char *xfail;
    RunnerKeyValueMap xfail_profiles;
    BenchmarkGenericData generic;
    BenchmarkKernel *kernels;
    size_t kernel_count;
} BenchmarkSuite;

typedef struct {
    RunnerKeyValueMap paths;
    BenchmarkProfile *profiles;
    size_t profile_count;
    BenchmarkSuite *suites;
    size_t suite_count;
} BenchmarkConfig;

typedef struct {
    const char *preprocessor;
    const RunnerStringVector *copy;
    const RunnerStringVector *cetus_inputs;
    const RunnerKeyValueMap *inject_map;
    const BenchmarkCommand *compile;
    const BenchmarkCommand *run;
    const BenchmarkVerify *verify;
} BenchmarkGenericView;

typedef struct {
    const char *root;
    const char *sandbox;
    const char *cpp;
    const char *dataset;
    const char *gcc;
    const char *name;
    const char *work;
} BenchmarkTemplateValues;

void benchmark_config_init(BenchmarkConfig *config);
int benchmark_config_load(const char *config_directory,
                          BenchmarkConfig *config,
                          char *error,
                          size_t error_size);
void benchmark_config_free(BenchmarkConfig *config);
const char *benchmark_config_path(const BenchmarkConfig *config,
                                  const char *name);

void benchmark_generic_resolve(const BenchmarkSuite *suite,
                               const BenchmarkKernel *kernel,
                               BenchmarkGenericView *view);

int benchmark_expand_template(const char *input,
                              const BenchmarkTemplateValues *values,
                              char **output,
                              char *error,
                              size_t error_size);

int benchmark_safe_relative_path(const char *path);

#endif
