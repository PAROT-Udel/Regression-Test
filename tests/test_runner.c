#define _POSIX_C_SOURCE 200809L

#include "process_runner.h"
#include "runner_common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return 1;                                                           \
        }                                                                       \
    } while (0)

static int accept_txt(const char *relative_path, int is_directory, void *context)
{
    size_t length;
    (void)context;
    if (is_directory) {
        return 1;
    }
    length = strlen(relative_path);
    return length >= 4 && strcmp(relative_path + length - 4, ".txt") == 0;
}

static int test_strings_and_paths(void)
{
    RunnerString string;
    RunnerStringVector vector;
    char *joined;
    char *root_joined;
    char *rendered;

    runner_string_init(&string);
    CHECK(runner_string_append(&string, "alpha") == 0);
    CHECK(runner_string_append_n(&string, "-beta-extra", 5) == 0);
    CHECK(strcmp(string.data, "alpha-beta") == 0);

    runner_string_vector_init(&vector);
    CHECK(runner_string_vector_push(&vector, "one") == 0);
    CHECK(runner_string_vector_push(&vector, "two") == 0);
    CHECK(vector.length == 2);
    CHECK(strcmp(vector.items[1], "two") == 0);

    joined = runner_path_join("/tmp/base/", "/child/file.txt");
    CHECK(joined != NULL);
    CHECK(strcmp(joined, "/tmp/base/child/file.txt") == 0);
    CHECK(strcmp(runner_path_basename(joined), "file.txt") == 0);
    root_joined = runner_path_join("/", "tmp");
    CHECK(root_joined != NULL);
    CHECK(strcmp(root_joined, "/tmp") == 0);

    rendered = runner_template_replace(
        "cc {{INPUT}} -o {{OUTPUT}}", "{{INPUT}}", "source file.c");
    CHECK(rendered != NULL);
    CHECK(strcmp(rendered, "cc source file.c -o {{OUTPUT}}") == 0);

    free(rendered);
    free(root_joined);
    free(joined);
    runner_string_vector_free(&vector);
    runner_string_free(&string);
    return 0;
}

static int test_files_and_environment(void)
{
    char root_template[] = "/tmp/cetus-runner-common-XXXXXX";
    RunnerKeyValueMap map;
    char *nested;
    char *source;
    char *copy;
    char *tree;
    char *tree_txt;
    char *tree_bin;
    char *filtered_bin;
    char *contents = NULL;
    size_t length = 0;
    const char binary_data[] = {'a', '\0', 'b'};

    CHECK(mkdtemp(root_template) != NULL);
    nested = runner_path_join(root_template, "a/b");
    source = runner_path_join(nested, "source.txt");
    copy = runner_path_join(root_template, "copy.txt");
    tree = runner_path_join(root_template, "tree");
    tree_txt = runner_path_join(tree, "source.txt");
    tree_bin = runner_path_join(nested, "skip.bin");
    filtered_bin = runner_path_join(tree, "skip.bin");
    CHECK(nested && source && copy && tree && tree_txt && tree_bin && filtered_bin);

    CHECK(runner_mkdir_recursive(nested, 0755) == 0);
    CHECK(runner_write_file(source, "hello\n", 6) == 0);
    CHECK(runner_write_file(tree_bin, "skip", 4) == 0);
    CHECK(runner_copy_file(source, copy) == 0);
    CHECK(runner_mkdir_recursive(copy, 0755) != 0);
    CHECK(runner_read_file(copy, &contents, &length) == 0);
    CHECK(length == 6 && strcmp(contents, "hello\n") == 0);
    free(contents);
    contents = NULL;
    CHECK(runner_write_file(copy, binary_data, sizeof(binary_data)) == 0);
    CHECK(runner_read_file(copy, &contents, &length) == 0);
    CHECK(length == sizeof(binary_data));
    CHECK(memcmp(contents, binary_data, sizeof(binary_data)) == 0);
    free(contents);
    contents = NULL;

    CHECK(runner_copy_tree(nested, tree, accept_txt, NULL) == 0);
    CHECK(runner_file_exists(tree_txt));
    CHECK(!runner_file_exists(filtered_bin));

    runner_key_value_map_init(&map);
    CHECK(runner_parse_env_text(
              "# comment\n NAME = value \nQUOTED=\"two words\"\nEMPTY=\n", &map) == 0);
    CHECK(strcmp(runner_key_value_map_get(&map, "NAME"), "value") == 0);
    CHECK(strcmp(runner_key_value_map_get(&map, "QUOTED"), "two words") == 0);
    CHECK(strcmp(runner_key_value_map_get(&map, "EMPTY"), "") == 0);
    CHECK(runner_key_value_map_set(&map, "NAME", "override") == 0);
    CHECK(strcmp(runner_key_value_map_get(&map, "NAME"), "override") == 0);

    runner_key_value_map_free(&map);
    CHECK(runner_remove_recursive(root_template) == 0);
    CHECK(!runner_file_exists(root_template));
    free(filtered_bin);
    free(tree_bin);
    free(tree_txt);
    free(tree);
    free(copy);
    free(source);
    free(nested);
    return 0;
}

static int test_process_capture_and_environment(void)
{
    const char *argv[] = {"sh", "-c", "printf '%s' \"$GREETING\"; printf err >&2; exit 7", NULL};
    RunnerEnvOverride environment[] = {{"GREETING", "hello"}};
    ProcessSpec spec = {0};
    ProcessResult result;

    spec.argv = argv;
    spec.environment = environment;
    spec.environment_count = 1;
    spec.timeout_seconds = 5;
    CHECK(process_run(&spec, &result) == 0);
    CHECK(result.exited);
    CHECK(result.exit_code == 7);
    CHECK(!result.signaled);
    CHECK(!result.timed_out);
    CHECK(strcmp(result.stdout_data, "hello") == 0);
    CHECK(strcmp(result.stderr_data, "err") == 0);
    process_result_free(&result);
    return 0;
}

static int test_process_cwd_signal_and_timeout(void)
{
    const char *pwd_argv[] = {"pwd", NULL};
    const char *signal_argv[] = {"sh", "-c", "kill -TERM $$", NULL};
    const char *timeout_argv[] = {
        "sh", "-c", "trap '' TERM; sleep 30 & wait", NULL};
    ProcessSpec spec = {0};
    ProcessResult result;

    spec.argv = pwd_argv;
    spec.cwd = "/tmp";
    spec.timeout_seconds = 5;
    CHECK(process_run(&spec, &result) == 0);
    CHECK(result.exited && result.exit_code == 0);
    CHECK(strcmp(result.stdout_data, "/tmp\n") == 0);
    process_result_free(&result);

    spec.argv = signal_argv;
    spec.cwd = NULL;
    CHECK(process_run(&spec, &result) == 0);
    CHECK(result.signaled && result.term_signal == 15);
    process_result_free(&result);

    spec.argv = timeout_argv;
    spec.timeout_seconds = 1;
    CHECK(process_run(&spec, &result) == 0);
    CHECK(result.timed_out);
    CHECK(result.signaled);
    process_result_free(&result);
    return 0;
}

static int test_process_times_out_descendant_after_child_exit(void)
{
    const char *argv[] = {
        "sh", "-c", "sleep 30 & printf descendant-started", NULL};
    ProcessSpec spec = {0};
    ProcessResult result;

    spec.argv = argv;
    spec.timeout_seconds = 1;
    CHECK(process_run(&spec, &result) == 0);
    CHECK(result.exited && result.exit_code == 0);
    CHECK(result.timed_out);
    CHECK(strcmp(result.stdout_data, "descendant-started") == 0);
    process_result_free(&result);
    return 0;
}

int main(void)
{
    CHECK(test_strings_and_paths() == 0);
    CHECK(test_files_and_environment() == 0);
    CHECK(test_process_capture_and_environment() == 0);
    CHECK(test_process_cwd_signal_and_timeout() == 0);
    CHECK(test_process_times_out_descendant_after_child_exit() == 0);
    puts("all native runner tests passed");
    return 0;
}
