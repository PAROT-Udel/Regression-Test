#ifndef BENCHMARK_RUNNER_H
#define BENCHMARK_RUNNER_H

#include "benchmark_config.h"
#include "process_runner.h"

#include <stddef.h>

typedef enum {
    BENCHMARK_STATUS_PASS,
    BENCHMARK_STATUS_FAIL,
    BENCHMARK_STATUS_XFAIL,
    BENCHMARK_STATUS_XPASS,
    BENCHMARK_STATUS_SKIP
} BenchmarkStatus;

typedef enum {
    BENCHMARK_STAGE_SELECTION,
    BENCHMARK_STAGE_SETUP,
    BENCHMARK_STAGE_GROUND_TRUTH,
    BENCHMARK_STAGE_PREPARE,
    BENCHMARK_STAGE_CETUS,
    BENCHMARK_STAGE_FORMAT,
    BENCHMARK_STAGE_INJECT,
    BENCHMARK_STAGE_BUILD,
    BENCHMARK_STAGE_VERIFY,
    BENCHMARK_STAGE_DONE
} BenchmarkStage;

typedef enum {
    BENCHMARK_ADAPTER_OK,
    BENCHMARK_ADAPTER_FUNCTIONAL_FAILURE,
    BENCHMARK_ADAPTER_INFRA_FAILURE,
    /* A hook may explicitly decline an otherwise valid case. */
    BENCHMARK_ADAPTER_SKIP
} BenchmarkAdapterOutcome;

typedef int (*BenchmarkProcessRun)(const ProcessSpec *spec,
                                   ProcessResult *result,
                                   void *context);

typedef struct {
    const BenchmarkSuite *suite;
    const BenchmarkKernel *kernel;
    const BenchmarkProfile *profile;
} BenchmarkCase;

typedef struct {
    const BenchmarkConfig *config;
    const BenchmarkCase *test_case;
    const char *project_root;
    const char *work_dir;
    const char *ground_truth_dir;
    const char *log_path;
    const char *cetus;
    int timeout_seconds;
    BenchmarkProcessRun process_run;
    void *process_context;
} BenchmarkCaseContext;

typedef struct {
    int (*expected_gt_names)(void *context,
                             const BenchmarkCaseContext *case_context,
                             RunnerStringVector *names,
                             char *error,
                             size_t error_size);
    BenchmarkAdapterOutcome (*prepare)(void *context,
                                       const BenchmarkCaseContext *case_context,
                                       char *error,
                                       size_t error_size);
    BenchmarkAdapterOutcome (*cetus_transform)(
        void *context,
        const BenchmarkCaseContext *case_context,
        const RunnerStringVector *flags,
        RunnerStringVector *outputs,
        char *error,
        size_t error_size);
    BenchmarkAdapterOutcome (*inject)(void *context,
                                      const BenchmarkCaseContext *case_context,
                                      const RunnerStringVector *outputs,
                                      char *error,
                                      size_t error_size);
    BenchmarkAdapterOutcome (*build)(void *context,
                                     const BenchmarkCaseContext *case_context,
                                     char *error,
                                     size_t error_size);
    BenchmarkAdapterOutcome (*verify)(void *context,
                                      const BenchmarkCaseContext *case_context,
                                      char *error,
                                      size_t error_size);
    void (*cleanup)(void *context, const BenchmarkCaseContext *case_context);
} BenchmarkAdapterVTable;

typedef struct {
    const BenchmarkAdapterVTable *vtable;
    void *context;
} BenchmarkAdapterInstance;

typedef struct {
    const char *project_root;
    const RunnerStringVector *override_flags;
    int timeout_seconds;
    int generate;
    int skip_ground_truth;
    int skip_run;
    BenchmarkProcessRun process_run;
    void *process_context;
} BenchmarkRunnerOptions;

typedef struct {
    BenchmarkStatus status;
    BenchmarkStage stage;
    char *message;
    char *log_path;
} BenchmarkCaseResult;

typedef struct {
    const BenchmarkSuite *suite;
    const BenchmarkKernel *kernel;
    const BenchmarkProfile *profile;
    BenchmarkCaseResult result;
} BenchmarkSelectionItem;

typedef struct {
    BenchmarkSelectionItem *items;
    size_t length;
    size_t capacity;
} BenchmarkSelection;

typedef struct {
    size_t pass;
    size_t fail;
    size_t xfail;
    size_t xpass;
    size_t skip;
    size_t total;
} BenchmarkSummary;

void benchmark_runner_options_init(BenchmarkRunnerOptions *options);
int benchmark_runner_parse_timeout(const char *text,
                                   int *timeout_seconds,
                                   char *error,
                                   size_t error_size);
void benchmark_selection_init(BenchmarkSelection *selection);
void benchmark_selection_free(BenchmarkSelection *selection);
int benchmark_runner_select(const BenchmarkConfig *config,
                            const RunnerStringVector *suite_ids,
                            const char *kernel_filter,
                            const char *profile_filter,
                            BenchmarkSelection *selection,
                            char *error,
                            size_t error_size);
int benchmark_runner_run_case(const BenchmarkConfig *config,
                              const BenchmarkCase *test_case,
                              const BenchmarkAdapterInstance *adapter,
                              const BenchmarkRunnerOptions *options,
                              BenchmarkCaseResult *result,
                              char *error,
                              size_t error_size);
void benchmark_case_result_free(BenchmarkCaseResult *result);
void benchmark_summary_add(BenchmarkSummary *summary,
                           const BenchmarkCaseResult *result);
int benchmark_summary_exit_code(const BenchmarkSummary *summary);
const char *benchmark_status_name(BenchmarkStatus status);
const char *benchmark_stage_name(BenchmarkStage stage);

void benchmark_log_process(const char *log_path,
                           const char *label,
                           const char *const *argv,
                           const ProcessResult *result);

#endif
