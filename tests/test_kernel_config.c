#define _POSIX_C_SOURCE 200809L

#include "kernel_config.h"
#include "runner_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return 1;                                                           \
        }                                                                       \
    } while (0)

static int write_text(const char *path, const char *text)
{
    return runner_write_file(path, text, strlen(text));
}

static int test_loads_suites_and_joins_flags(void)
{
    char root[] = "/tmp/cetus-kernel-config-XXXXXX";
    char *subsub;
    char *tiling;
    KernelConfig config;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    subsub = runner_path_join(root, "subsub.json");
    tiling = runner_path_join(root, "tiling.json");
    CHECK(subsub && tiling);
    CHECK(write_text(subsub,
                     "{\"id\":\"subsub\",\"transform\":\"subsub_analysis\","
                     "\"aliases\":[\"subsub_analysis\"],"
                     "\"flags\":[\"-subsub_analysis\",\"-ompGen=0\"],"
                     "\"kernels\":[{\"name\":\"Amg\","
                     "\"input\":\"subsub/subsub_test_amgmk.c\"}]}") == 0);
    CHECK(write_text(tiling,
                     "{\"id\":\"tiling\",\"description\":\"PAW snippets\","
                     "\"transform\":\"tiling\","
                     "\"expected\":\"success_transformed\","
                     "\"aliases\":[\"paw\",\"paw_tiling\"],"
                     "\"flags\":[\"-paw_tiling=1\",\"-selection=NT\"],"
                     "\"kernels\":["
                     "{\"name\":\"Gemm\",\"input\":\"tiling/tiling_gemm.c\","
                     "\"flags\":[\"-tileSizes=64\"]},"
                     "{\"name\":\"Small\",\"input\":\"tiling/small.c\"}"
                     "]}") == 0);

    kernel_config_init(&config);
    CHECK(kernel_config_load(root, &config, error, sizeof(error)) == 0);
    CHECK(config.suite_count == 2);
    CHECK(strcmp(config.suites[0].id, "subsub") == 0);
    CHECK(strcmp(config.suites[1].id, "tiling") == 0);
    CHECK(strcmp(config.suites[1].description, "PAW snippets") == 0);
    CHECK(config.case_count == 3);
    CHECK(strcmp(config.cases[0].name, "Amg") == 0);
    CHECK(config.cases[0].transform == TRANSFORM_SUBSUB_ANALYSIS);
    CHECK(strcmp(config.cases[0].flags, "-subsub_analysis -ompGen=0") == 0);
    CHECK(strcmp(config.cases[1].name, "Gemm") == 0);
    CHECK(config.cases[1].expected == EXPECT_SUCCESS_TRANSFORMED);
    CHECK(strcmp(config.cases[1].flags,
                 "-paw_tiling=1 -selection=NT -tileSizes=64") == 0);
    CHECK(strcmp(config.cases[2].flags, "-paw_tiling=1 -selection=NT") == 0);
    CHECK(kernel_config_matches_suite(&config, &config.cases[1], "paw") == 1);
    CHECK(kernel_config_matches_suite(&config, &config.cases[1], "Tiling") == 1);
    CHECK(kernel_config_matches_suite(&config, &config.cases[0], "paw") == 0);
    CHECK(kernel_config_matches_suite(
              &config, &config.cases[0], "subsub_analysis") == 1);

    kernel_config_free(&config);
    CHECK(runner_remove_recursive(root) == 0);
    free(tiling);
    free(subsub);
    return 0;
}

static int expect_invalid(const char *json, const char *needle)
{
    char root[] = "/tmp/cetus-kernel-invalid-XXXXXX";
    char *path;
    KernelConfig config;
    char error[512];
    int result = 1;

    CHECK(mkdtemp(root) != NULL);
    path = runner_path_join(root, "one.json");
    CHECK(path != NULL);
    CHECK(write_text(path, json) == 0);
    kernel_config_init(&config);
    if (kernel_config_load(root, &config, error, sizeof(error)) != 0 &&
        strstr(error, needle) != NULL && strstr(error, ".json") != NULL) {
        result = 0;
    } else {
        fprintf(stderr, "expected error containing %s, got: %s\n", needle, error);
    }
    kernel_config_free(&config);
    runner_remove_recursive(root);
    free(path);
    return result;
}

static int test_validation_errors(void)
{
    CHECK(expect_invalid(
              "{\"id\":\"tiling\",\"transform\":\"tiling\","
              "\"flags\":[\"-paw_tiling=1\"],\"kernels\":["
              "{\"name\":\"A\",\"input\":\"a.c\"},"
              "{\"name\":\"A\",\"input\":\"b.c\"}]}",
              "duplicate kernel") == 0);
    CHECK(expect_invalid(
              "{\"id\":\"tiling\",\"transform\":\"tiling\","
              "\"flags\":[\"-paw_tiling=1\"],\"kernels\":["
              "{\"name\":\"A\"}]}",
              "input") == 0);
    CHECK(expect_invalid(
              "{\"id\":\"tiling\",\"transform\":\"tiling\","
              "\"flags\":[\"-paw_tiling=1\"],\"kernels\":[]}",
              "kernels") == 0);
    CHECK(expect_invalid(
              "{\"id\":\"tiling\",\"transform\":\"nope\","
              "\"flags\":[\"-paw_tiling=1\"],"
              "\"kernels\":[{\"name\":\"A\",\"input\":\"a.c\"}]}",
              "transform") == 0);
    CHECK(expect_invalid(
              "{\"id\":\"tiling\",\"transform\":\"tiling\","
              "\"flags\":[\"-paw_tiling=1\"],"
              "\"kernels\":[{\"name\":\"A\",\"input\":\"../a.c\"}]}",
              "input") == 0);
    return 0;
}

static const KernelCase *find_case(const KernelConfig *config, const char *name)
{
    size_t index;
    for (index = 0; index < config->case_count; ++index) {
        if (strcmp(config->cases[index].name, name) == 0)
            return &config->cases[index];
    }
    return NULL;
}

static int test_checked_in_kernel_suites(void)
{
    KernelConfig config;
    const KernelCase *gemm;
    const KernelCase *amg;
    char error[512];
    size_t tiling = 0;
    size_t subsub = 0;
    size_t index;

    kernel_config_init(&config);
    CHECK(kernel_config_load(
              "benchmarks/config/kernels", &config, error, sizeof(error)) == 0);
    CHECK(config.case_count == 15);
    CHECK(config.suite_count == 2);
    for (index = 0; index < config.case_count; ++index) {
        if (strcmp(config.cases[index].suite, "tiling") == 0)
            tiling++;
        if (strcmp(config.cases[index].suite, "subsub") == 0)
            subsub++;
    }
    CHECK(tiling == 6);
    CHECK(subsub == 9);
    gemm = find_case(&config, "Tiling_PAW_GEMM_Fixed64");
    amg = find_case(&config, "SubSub_Amgmk");
    CHECK(gemm != NULL && amg != NULL);
    CHECK(strcmp(gemm->input, "tiling/tiling_gemm.c") == 0);
    CHECK(strstr(gemm->flags, "-paw_tiling=1") != NULL);
    CHECK(strstr(gemm->flags, "-tileSizes=64") != NULL);
    CHECK(strstr(amg->flags, "-subsub_analysis") != NULL);
    CHECK(strstr(amg->flags, "-tileSizes") == NULL);
    CHECK(kernel_config_matches_suite(&config, gemm, "paw_tiling") == 1);
    CHECK(kernel_config_matches_suite(&config, amg, "subscripted-subscript") == 1);
    kernel_config_free(&config);
    return 0;
}

int main(void)
{
    CHECK(test_loads_suites_and_joins_flags() == 0);
    CHECK(test_validation_errors() == 0);
    CHECK(test_checked_in_kernel_suites() == 0);
    printf("kernel config tests passed\n");
    return 0;
}
