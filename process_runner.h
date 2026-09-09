#ifndef PROCESS_RUNNER_H
#define PROCESS_RUNNER_H

#include <stddef.h>

typedef struct {
    const char *name;
    const char *value;
} RunnerEnvOverride;

typedef struct {
    const char *const *argv;
    const char *cwd;
    const RunnerEnvOverride *environment;
    size_t environment_count;
    unsigned int timeout_seconds;
} ProcessSpec;

typedef struct {
    int exited;
    int exit_code;
    int signaled;
    int term_signal;
    int timed_out;
    char *stdout_data;
    char *stderr_data;
} ProcessResult;

int process_run(const ProcessSpec *spec, ProcessResult *result);
void process_result_free(ProcessResult *result);

#endif
