#define _POSIX_C_SOURCE 200809L

#include "benchmark_config.h"
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

static int make_fixture(char *root)
{
    char *suites = runner_path_join(root, "suites");
    char *example = runner_path_join(root, "paths.example.env");
    char *local = runner_path_join(root, "paths.env");
    char *profiles = runner_path_join(root, "profiles.json");
    char *zeta = runner_path_join(suites, "zeta.json");
    char *alpha = runner_path_join(suites, "alpha.json");
    int result = -1;

    if (!suites || !example || !local || !profiles || !zeta || !alpha ||
        runner_mkdir_recursive(suites, 0755) != 0 ||
        write_text(example,
                   "CETUS=/example/cetus\nCPP=example-cpp\n"
                   "CUSTOM_ROOT=/example/root\n") != 0 ||
        write_text(local, "CETUS=/local/cetus\nGCC=local-gcc\n") != 0 ||
        write_text(profiles,
                   "{\"parallel\":{\"description\":\"OpenMP\","
                   "\"flags\":[\"-ompGen=1\"]},"
                   "\"tile\":{\"flags\":[\"-paw_tiling=1\"]}}") != 0 ||
        write_text(zeta,
                   "{\"id\":\"zeta\",\"adapter\":\"npb\",\"default\":false,"
                   "\"root_env\":\"NPB_ROOT\",\"profiles\":[\"parallel\"],"
                   "\"kernels\":[{\"name\":\"cg\",\"bench\":\"CG\","
                   "\"binary\":\"cg\",\"headers\":[],\"sources\":[\"cg.c\"],"
                   "\"xfail_profiles\":{\"parallel\":\"known\"}}]}") != 0 ||
        write_text(alpha,
                   "{\"id\":\"alpha\",\"adapter\":\"generic\",\"default\":true,"
                   "\"root_env\":\"CUSTOM_ROOT\",\"dataset\":\"MINI\","
                   "\"profiles\":[\"parallel\",\"tile\"],"
                   "\"xfail_profiles\":{\"tile\":\"suite known\"},"
                   "\"generic\":{\"preprocessor\":\"{cpp} -D{dataset}\","
                   "\"compile\":[\"{gcc}\",\"{name}.c\"],"
                   "\"inject_map\":{\"demo.c\":\"suite/demo.c\"},"
                   "\"verify\":{\"type\":\"exit_code\",\"expect\":0,"
                   "\"run\":[\"./{name}\"]}},"
                   "\"kernels\":[{\"name\":\"demo\",\"copy\":[\"demo.c\"],"
                   "\"cetus_inputs\":[\"demo.c\"],"
                   "\"inject_map\":{\"demo.c\":\"src/demo.c\"},"
                   "\"compile\":[\"override-cc\",\"{work}\"],"
                   "\"xfail\":\"pending\"}]}") != 0) {
        goto cleanup;
    }
    result = 0;
cleanup:
    free(alpha);
    free(zeta);
    free(profiles);
    free(local);
    free(example);
    free(suites);
    return result;
}

static int test_loads_owned_models_and_precedence(void)
{
    char root[] = "/tmp/cetus-benchmark-config-XXXXXX";
    BenchmarkConfig config;
    BenchmarkGenericView generic;
    char error[512];

    CHECK(mkdtemp(root) != NULL);
    CHECK(make_fixture(root) == 0);
    CHECK(setenv("CETUS", "/env/cetus", 1) == 0);
    CHECK(setenv("CUSTOM_ROOT", "/env/custom", 1) == 0);
    CHECK(setenv("NPB_ROOT", "/env/npb", 1) == 0);

    benchmark_config_init(&config);
    CHECK(benchmark_config_load(root, &config, error, sizeof(error)) == 0);
    CHECK(config.profile_count == 2);
    CHECK(strcmp(config.profiles[0].name, "parallel") == 0);
    CHECK(config.profiles[0].flags.length == 1);
    CHECK(config.suite_count == 2);
    CHECK(strcmp(config.suites[0].id, "alpha") == 0);
    CHECK(strcmp(config.suites[1].id, "zeta") == 0);
    CHECK(strcmp(benchmark_config_path(&config, "CETUS"), "/env/cetus") == 0);
    CHECK(strcmp(benchmark_config_path(&config, "GCC"), "local-gcc") == 0);
    CHECK(strcmp(benchmark_config_path(&config, "CUSTOM_ROOT"), "/env/custom") == 0);
    CHECK(strcmp(benchmark_config_path(&config, "NPB_ROOT"), "/env/npb") == 0);

    CHECK(config.suites[0].adapter == BENCHMARK_ADAPTER_GENERIC);
    CHECK(config.suites[0].kernel_count == 1);
    CHECK(strcmp(config.suites[0].kernels[0].inject_map.items[0].value,
                 "src/demo.c") == 0);
    CHECK(strcmp(config.suites[0].kernels[0].xfail, "pending") == 0);
    CHECK(strcmp(config.suites[0].xfail_profiles.items[0].value,
                 "suite known") == 0);
    CHECK(strcmp(config.suites[1].kernels[0].xfail_profiles.items[0].value,
                 "known") == 0);
    benchmark_generic_resolve(
        &config.suites[0], &config.suites[0].kernels[0], &generic);
    CHECK(strcmp(generic.preprocessor, "{cpp} -D{dataset}") == 0);
    CHECK(generic.compile == &config.suites[0].kernels[0].generic.compile);
    CHECK(generic.verify == &config.suites[0].generic.verify);
    CHECK(strcmp(generic.inject_map->items[0].value, "src/demo.c") == 0);
    CHECK(generic.copy->length == 1 && generic.cetus_inputs->length == 1);

    benchmark_config_free(&config);
    unsetenv("CETUS");
    unsetenv("CUSTOM_ROOT");
    unsetenv("NPB_ROOT");
    CHECK(runner_remove_recursive(root) == 0);
    return 0;
}

static int expect_invalid(const char *profiles_json,
                          const char *first_suite,
                          const char *second_suite,
                          const char *needle)
{
    char root[] = "/tmp/cetus-benchmark-invalid-XXXXXX";
    char *suites;
    char *profiles;
    char *one;
    char *two;
    BenchmarkConfig config;
    char error[512];
    int result = 1;

    CHECK(mkdtemp(root) != NULL);
    suites = runner_path_join(root, "suites");
    profiles = runner_path_join(root, "profiles.json");
    one = runner_path_join(suites, "one.json");
    two = runner_path_join(suites, "two.json");
    CHECK(suites && profiles && one && two);
    CHECK(runner_mkdir_recursive(suites, 0755) == 0);
    CHECK(write_text(profiles, profiles_json) == 0);
    CHECK(write_text(one, first_suite) == 0);
    if (second_suite != NULL) {
        CHECK(write_text(two, second_suite) == 0);
    }
    benchmark_config_init(&config);
    if (benchmark_config_load(root, &config, error, sizeof(error)) != 0 &&
        strstr(error, needle) != NULL && strstr(error, ".json") != NULL) {
        result = 0;
    } else {
        fprintf(stderr, "expected error containing %s, got: %s\n", needle, error);
    }
    benchmark_config_free(&config);
    runner_remove_recursive(root);
    free(two);
    free(one);
    free(profiles);
    free(suites);
    return result;
}

static int test_validation_errors(void)
{
    const char *profiles = "{\"parallel\":{\"flags\":[]}}";
    const char *valid =
        "{\"id\":\"one\",\"adapter\":\"polybench\",\"root_env\":\"ROOT\","
        "\"profiles\":[\"parallel\"],\"kernels\":[{\"name\":\"gemm\","
        "\"rel_path\":\"blas/gemm\"}]}";
    CHECK(expect_invalid("{}", valid, NULL, "profiles") == 0);
    CHECK(expect_invalid(profiles,
          "{\"id\":\"one\",\"adapter\":\"mystery\",\"root_env\":\"ROOT\","
          "\"profiles\":[\"parallel\"],\"kernels\":[{\"name\":\"x\"}]}",
          NULL, "adapter") == 0);
    CHECK(expect_invalid(profiles,
          "{\"id\":\"one\",\"adapter\":\"polybench\",\"root_env\":\"ROOT\","
          "\"profiles\":[\"missing\"],\"kernels\":[{\"name\":\"x\","
          "\"rel_path\":\"x\"}]}", NULL, "missing") == 0);
    CHECK(expect_invalid(profiles,
          "{\"id\":\"one\",\"adapter\":\"polybench\",\"root_env\":\"ROOT\","
          "\"profiles\":[\"parallel\"],\"kernels\":[]}",
          NULL, "kernels") == 0);
    CHECK(expect_invalid(profiles,
          "{\"id\":\"one\",\"adapter\":\"polybench\",\"root_env\":\"ROOT\","
          "\"profiles\":[\"parallel\"],\"kernels\":[{\"name\":\"x\","
          "\"rel_path\":\"x\"},{\"name\":\"x\",\"rel_path\":\"y\"}]}",
          NULL, "duplicate kernel") == 0);
    CHECK(expect_invalid(profiles, valid,
          "{\"id\":\"one\",\"adapter\":\"npb\",\"root_env\":\"ROOT\","
          "\"profiles\":[\"parallel\"],\"kernels\":[{\"name\":\"cg\","
          "\"bench\":\"CG\",\"binary\":\"cg\",\"sources\":[\"cg.c\"]}]}",
          "duplicate suite") == 0);
    CHECK(expect_invalid(profiles,
          "{\"id\":\"one\",\"adapter\":\"generic\",\"root_env\":\"ROOT\","
          "\"profiles\":[\"parallel\"],\"kernels\":[{\"name\":\"x\","
          "\"cetus_inputs\":[]}]}",
          NULL, "cetus_inputs") == 0);
    CHECK(expect_invalid(profiles,
          "{\"id\":\"one\",\"adapter\":\"generic\",\"root_env\":\"ROOT\","
          "\"profiles\":[\"parallel\"],\"generic\":{\"verify\":{"
          "\"type\":\"bogus\"}},\"kernels\":[{\"name\":\"x\","
          "\"cetus_inputs\":[\"x.c\"]}]}",
          NULL, "verify.type") == 0);
    CHECK(expect_invalid(profiles,
          "{\"id\":\"one\",\"adapter\":\"polybench\",\"root_env\":\"ROOT\","
          "\"profiles\":\"parallel\",\"kernels\":[{\"name\":\"x\","
          "\"rel_path\":\"x\"}]}",
          NULL, "profiles") == 0);
    CHECK(expect_invalid(profiles,
          "{\"id\":\"one\",\"adapter\":\"polybench\",\"root_env\":\"ROOT\","
          "\"profiles\":[\"parallel\"],"
          "\"xfail_profiles\":{\"missing\":\"known issue\"},"
          "\"kernels\":[{\"name\":\"x\",\"rel_path\":\"x\"}]}",
          NULL, "xfail_profiles") == 0);
    CHECK(expect_invalid(profiles,
          "{\"id\":\"one\",\"adapter\":\"polybench\",\"root_env\":\"ROOT\","
          "\"profiles\":[\"parallel\"],\"kernels\":[{\"name\":\"x\","
          "\"rel_path\":\"x\",\"xfail_profiles\":{"
          "\"missing\":\"known issue\"}}]}",
          NULL, "xfail_profiles") == 0);
    return 0;
}

static int test_template_expansion(void)
{
    BenchmarkTemplateValues values = {
        "/root", "/work/sandbox", "cpp-tool", "SMALL", "gcc-tool", "gemm",
        "/work"};
    char *expanded = NULL;
    char error[256];

    CHECK(benchmark_expand_template(
              "{gcc} {sandbox}/{name}.c -D{dataset} -E{cpp} "
              "{root} {work} {{literal}}",
              &values, &expanded, error, sizeof(error)) == 0);
    CHECK(strcmp(expanded,
                 "gcc-tool /work/sandbox/gemm.c -DSMALL -Ecpp-tool "
                 "/root /work {literal}") == 0);
    free(expanded);
    expanded = NULL;
    CHECK(benchmark_expand_template(
              "{unknown}", &values, &expanded, error, sizeof(error)) != 0);
    CHECK(expanded == NULL);
    CHECK(strstr(error, "unknown placeholder") != NULL);
    CHECK(strstr(error, "unknown") != NULL);
    CHECK(benchmark_expand_template(
              "right }}", &values, &expanded, error, sizeof(error)) == 0);
    CHECK(strcmp(expanded, "right }") == 0);
    free(expanded);
    expanded = NULL;
    CHECK(benchmark_expand_template(
              "unmatched }", &values, &expanded, error, sizeof(error)) != 0);
    CHECK(expanded == NULL);
    CHECK(strstr(error, "invalid template") != NULL);
    return 0;
}

static int test_repository_configuration(void)
{
    BenchmarkConfig config;
    char error[512];

    benchmark_config_init(&config);
    CHECK(benchmark_config_load(
              "benchmarks/config", &config, error, sizeof(error)) == 0);
    CHECK(config.profile_count == 2);
    CHECK(config.suite_count == 4);
    CHECK(strcmp(config.suites[0].id, "generic-example") == 0);
    CHECK(strcmp(config.suites[1].id, "npb-omp") == 0);
    CHECK(strcmp(config.suites[2].id, "npb-ser") == 0);
    CHECK(strcmp(config.suites[3].id, "polybench") == 0);
    CHECK(config.suites[3].kernel_count == 30);
    benchmark_config_free(&config);
    return 0;
}

int main(void)
{
    CHECK(test_loads_owned_models_and_precedence() == 0);
    CHECK(test_template_expansion() == 0);
    CHECK(test_validation_errors() == 0);
    CHECK(test_repository_configuration() == 0);
    puts("all benchmark config tests passed");
    return 0;
}
