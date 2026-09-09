CC ?= cc
CPPFLAGS ?=
CPPFLAGS += -D_GNU_SOURCE -I. -Ithird_party/cjson
CFLAGS ?=
CFLAGS += -std=c11 -Wall -Wextra
LDFLAGS ?=
LDLIBS ?=

ADAPTER_SOURCES := benchmark_adapters.c polybench_adapter.c npb_adapter.c \
	generic_adapter.c

TARGET := cetus_regression_test
TARGET_SOURCES := cetus_regression_test.c cli.c runner_common.c process_runner.c \
	benchmark_config.c benchmark_runner.c $(ADAPTER_SOURCES) \
	third_party/cjson/cJSON.c
TARGET_OBJECTS := $(TARGET_SOURCES:.c=.o)

TEST_TARGET := tests/test_runner
TEST_SOURCES := tests/test_runner.c runner_common.c process_runner.c \
	third_party/cjson/cJSON.c
CONFIG_TEST_TARGET := tests/test_benchmark_config
CONFIG_TEST_SOURCES := tests/test_benchmark_config.c benchmark_config.c \
	runner_common.c third_party/cjson/cJSON.c
BENCHMARK_TEST_TARGET := tests/test_benchmark_runner
BENCHMARK_TEST_SOURCES := tests/test_benchmark_runner.c benchmark_runner.c \
	benchmark_config.c runner_common.c process_runner.c third_party/cjson/cJSON.c
ADAPTER_TEST_TARGET := tests/test_benchmark_adapters
ADAPTER_TEST_SOURCES := tests/test_benchmark_adapters.c $(ADAPTER_SOURCES) \
	benchmark_config.c benchmark_runner.c runner_common.c process_runner.c \
	third_party/cjson/cJSON.c
CLI_TEST_TARGET := tests/test_cli
CLI_TEST_SOURCES := tests/test_cli.c cli.c runner_common.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(TARGET_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_TARGET): $(TEST_SOURCES) runner_common.h process_runner.h \
	third_party/cjson/cJSON.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(TEST_SOURCES) $(LDLIBS)

$(CONFIG_TEST_TARGET): $(CONFIG_TEST_SOURCES) benchmark_config.h runner_common.h \
	third_party/cjson/cJSON.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(CONFIG_TEST_SOURCES) $(LDLIBS)

$(BENCHMARK_TEST_TARGET): $(BENCHMARK_TEST_SOURCES) benchmark_runner.h \
	benchmark_config.h runner_common.h process_runner.h third_party/cjson/cJSON.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(BENCHMARK_TEST_SOURCES) $(LDLIBS)

$(ADAPTER_TEST_TARGET): $(ADAPTER_TEST_SOURCES) benchmark_adapters.h \
	benchmark_runner.h benchmark_config.h runner_common.h process_runner.h \
	third_party/cjson/cJSON.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(ADAPTER_TEST_SOURCES) $(LDLIBS)

$(CLI_TEST_TARGET): $(CLI_TEST_SOURCES) cli.h runner_common.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(CLI_TEST_SOURCES) $(LDLIBS)

test: $(TEST_TARGET) $(CONFIG_TEST_TARGET) $(BENCHMARK_TEST_TARGET) \
	$(ADAPTER_TEST_TARGET) $(CLI_TEST_TARGET)
	./$(TEST_TARGET)
	./$(CONFIG_TEST_TARGET)
	./$(BENCHMARK_TEST_TARGET)
	./$(ADAPTER_TEST_TARGET)
	./$(CLI_TEST_TARGET)

clean:
	rm -f $(TARGET) $(TARGET_OBJECTS) $(TEST_TARGET) $(CONFIG_TEST_TARGET) \
		$(BENCHMARK_TEST_TARGET) $(ADAPTER_TEST_TARGET) $(CLI_TEST_TARGET)
