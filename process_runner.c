#define _POSIX_C_SOURCE 200809L

#include "process_runner.h"
#include "runner_common.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void initialize_result(ProcessResult *result)
{
    memset(result, 0, sizeof(*result));
    result->exit_code = -1;
}

void process_result_free(ProcessResult *result)
{
    free(result->stdout_data);
    free(result->stderr_data);
    initialize_result(result);
}

static double monotonic_seconds(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        return 0.0;
    }
    return (double)value.tv_sec + (double)value.tv_nsec / 1000000000.0;
}

static int make_pipe(int descriptors[2])
{
    if (pipe(descriptors) != 0) {
        return -1;
    }
    if (fcntl(descriptors[0], F_SETFD, FD_CLOEXEC) != 0 ||
        fcntl(descriptors[1], F_SETFD, FD_CLOEXEC) != 0) {
        int saved_errno = errno;
        close(descriptors[0]);
        close(descriptors[1]);
        errno = saved_errno;
        return -1;
    }
    return 0;
}

static void child_execute(const ProcessSpec *spec,
                          int stdout_pipe[2],
                          int stderr_pipe[2])
{
    size_t index;

    setpgid(0, 0);
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);
    if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0 ||
        dup2(stderr_pipe[1], STDERR_FILENO) < 0) {
        _exit(126);
    }
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    if (spec->cwd != NULL && chdir(spec->cwd) != 0) {
        _exit(126);
    }
    for (index = 0; index < spec->environment_count; ++index) {
        const RunnerEnvOverride *override = &spec->environment[index];
        int status = override->value == NULL
                         ? unsetenv(override->name)
                         : setenv(override->name, override->value, 1);
        if (status != 0) {
            _exit(126);
        }
    }
    execvp(spec->argv[0], (char *const *)spec->argv);
    _exit(errno == ENOENT ? 127 : 126);
}

static int set_nonblocking(int descriptor)
{
    int flags = fcntl(descriptor, F_GETFL);
    return flags < 0 ? -1 : fcntl(descriptor, F_SETFL, flags | O_NONBLOCK);
}

static int drain_descriptor(int *descriptor, RunnerString *destination)
{
    char buffer[8192];
    ssize_t count;

    for (;;) {
        count = read(*descriptor, buffer, sizeof(buffer));
        if (count > 0) {
            if (runner_string_append_n(
                    destination, buffer, (size_t)count) != 0) {
                return -1;
            }
        } else if (count == 0) {
            close(*descriptor);
            *descriptor = -1;
            return 0;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else if (errno != EINTR) {
            return -1;
        }
    }
}

static void close_if_open(int *descriptor)
{
    if (*descriptor >= 0) {
        close(*descriptor);
        *descriptor = -1;
    }
}

static int collect_process(pid_t child,
                           int stdout_descriptor,
                           int stderr_descriptor,
                           unsigned int timeout_seconds,
                           ProcessResult *result)
{
    RunnerString stdout_buffer;
    RunnerString stderr_buffer;
    double start = monotonic_seconds();
    int status = 0;
    int child_finished = 0;
    int failed = 0;
    int kill_sent = 0;
    double termination_started = 0.0;

    runner_string_init(&stdout_buffer);
    runner_string_init(&stderr_buffer);
    if (set_nonblocking(stdout_descriptor) != 0 ||
        set_nonblocking(stderr_descriptor) != 0) {
        failed = 1;
    }
    while (!failed &&
           (!child_finished ||
            stdout_descriptor >= 0 || stderr_descriptor >= 0)) {
        struct pollfd descriptors[2];
        nfds_t count = 0;
        int poll_result;
        pid_t wait_result;

        if (stdout_descriptor >= 0) {
            descriptors[count].fd = stdout_descriptor;
            descriptors[count].events = POLLIN | POLLHUP;
            descriptors[count].revents = 0;
            ++count;
        }
        if (stderr_descriptor >= 0) {
            descriptors[count].fd = stderr_descriptor;
            descriptors[count].events = POLLIN | POLLHUP;
            descriptors[count].revents = 0;
            ++count;
        }
        poll_result = poll(descriptors, count, 50);
        if (poll_result < 0 && errno != EINTR) {
            failed = 1;
            break;
        }
        if (stdout_descriptor >= 0 &&
            drain_descriptor(&stdout_descriptor, &stdout_buffer) != 0) {
            failed = 1;
            break;
        }
        if (stderr_descriptor >= 0 &&
            drain_descriptor(&stderr_descriptor, &stderr_buffer) != 0) {
            failed = 1;
            break;
        }
        if (!child_finished) {
            wait_result = waitpid(child, &status, WNOHANG);
            if (wait_result == child) {
                child_finished = 1;
            } else if (wait_result < 0 && errno != EINTR) {
                failed = 1;
                break;
            }
        }
        if (!result->timed_out && timeout_seconds > 0 &&
            monotonic_seconds() - start >= (double)timeout_seconds) {
            result->timed_out = 1;
            termination_started = monotonic_seconds();
            if (kill(-child, SIGTERM) != 0 && errno != ESRCH) {
                failed = 1;
                break;
            }
        }
        if (result->timed_out && !kill_sent &&
            monotonic_seconds() - termination_started >= 0.2) {
            if (kill(-child, SIGKILL) != 0 && errno != ESRCH) {
                failed = 1;
                break;
            }
            kill_sent = 1;
        }
    }
    close_if_open(&stdout_descriptor);
    close_if_open(&stderr_descriptor);
    if (!child_finished) {
        kill(-child, SIGKILL);
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
        }
    }
    if (failed) {
        runner_string_free(&stdout_buffer);
        runner_string_free(&stderr_buffer);
        return -1;
    }
    result->stdout_data = runner_string_take(&stdout_buffer);
    result->stderr_data = runner_string_take(&stderr_buffer);
    if (result->stdout_data == NULL || result->stderr_data == NULL) {
        process_result_free(result);
        return -1;
    }
    if (WIFEXITED(status)) {
        result->exited = 1;
        result->exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result->signaled = 1;
        result->term_signal = WTERMSIG(status);
    }
    return 0;
}

int process_run(const ProcessSpec *spec, ProcessResult *result)
{
    int stdout_pipe[2] = {-1, -1};
    int stderr_pipe[2] = {-1, -1};
    pid_t child;
    int saved_errno;

    if (result == NULL || spec == NULL || spec->argv == NULL ||
        spec->argv[0] == NULL) {
        errno = EINVAL;
        return -1;
    }
    initialize_result(result);
    if (make_pipe(stdout_pipe) != 0 || make_pipe(stderr_pipe) != 0) {
        saved_errno = errno;
        close_if_open(&stdout_pipe[0]);
        close_if_open(&stdout_pipe[1]);
        close_if_open(&stderr_pipe[0]);
        close_if_open(&stderr_pipe[1]);
        errno = saved_errno;
        return -1;
    }
    child = fork();
    if (child < 0) {
        saved_errno = errno;
        close_if_open(&stdout_pipe[0]);
        close_if_open(&stdout_pipe[1]);
        close_if_open(&stderr_pipe[0]);
        close_if_open(&stderr_pipe[1]);
        errno = saved_errno;
        return -1;
    }
    if (child == 0) {
        child_execute(spec, stdout_pipe, stderr_pipe);
    }
    setpgid(child, child);
    close_if_open(&stdout_pipe[1]);
    close_if_open(&stderr_pipe[1]);
    return collect_process(child,
                           stdout_pipe[0],
                           stderr_pipe[0],
                           spec->timeout_seconds,
                           result);
}
