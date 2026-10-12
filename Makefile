CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Isrc
# consteval.c uses fmod/fmax: on Linux the math library is separate.
LDLIBS = -lm
RELEASE_FLAGS = -O2
DEBUG_FLAGS = -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all

COMMON_SRC = src/common/arena.c src/common/util.c src/common/diag.c src/common/ast.c src/common/type.c src/common/library.c
FRONT_SRC = src/front/lexer.c src/front/parse_util.c src/front/parse_expr.c src/front/parse_value.c src/front/parse_stmt.c src/front/parse_if.c src/front/parse_loop.c src/front/parse_func.c src/front/parse_list.c src/front/parse_kind.c src/front/parse_match.c src/front/parse_text.c src/front/parse_names.c src/front/parse_record.c src/front/parse_map.c src/front/parse_func_value.c src/front/parse_apply.c src/front/parse_iface.c src/front/parse_module.c src/front/parse_library.c src/front/parse_library_stmt.c src/front/parse_system.c src/front/consteval.c src/front/check.c src/front/check_names.c src/front/check_expr.c src/front/check_types.c src/front/check_narrow.c src/front/check_kind.c src/front/check_const.c src/front/check_list.c src/front/check_text.c src/front/check_record.c src/front/check_map.c src/front/check_func_value.c src/front/check_apply.c src/front/check_iface.c src/front/check_module.c src/front/check_library.c src/front/check_system.c
BACK_SRC = src/back/codegen_c.c src/back/codegen_program.c src/back/codegen_expr.c src/back/codegen_list.c src/back/codegen_text.c src/back/codegen_record.c src/back/codegen_map.c src/back/codegen_kind.c src/back/codegen_closure.c src/back/codegen_apply.c src/back/codegen_iface.c src/back/codegen_names.c src/back/codegen_library.c src/back/codegen_system.c src/back/source_table.c
# The runtime (its files in order), embedded into the compiler as a byte
# array by tools/embed.c.
RUNTIME = runtime/es_memory.h runtime/es_program.h runtime/es_runtime.h runtime/es_list.h runtime/es_sort.h runtime/es_text.h runtime/es_record.h runtime/es_map.h runtime/es_kind.h runtime/es_closure.h runtime/es_math.h runtime/es_time.h runtime/es_files.h runtime/es_csv.h runtime/es_keys.h runtime/es_system.h
GEN_RUNTIME = build/gen/es_runtime_embed.c
EMBED = build/tools/embed
COMPILER_SRC = src/main.c src/modules.c src/shell.c $(FRONT_SRC) $(BACK_SRC) $(COMMON_SRC) $(GEN_RUNTIME)
UNIT_SRC = $(wildcard tests/unit/*.c) $(FRONT_SRC) $(COMMON_SRC)

RELEASE_DIR = build/release
DEBUG_DIR = build/debug
GCC_DIR = build/gcc

release_objs = $(patsubst %.c,$(RELEASE_DIR)/%.o,$(1))
debug_objs = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(1))
gcc_objs = $(patsubst %.c,$(GCC_DIR)/%.o,$(1))

# GCC for `make test-gcc`: the newest Homebrew gcc-N on macOS (where `cc` is
# Clang), plain gcc elsewhere. Override with `make test-gcc GCC=gcc-15`.
GCC ?= $(or $(shell ls /opt/homebrew/bin/gcc-[0-9]* /usr/local/bin/gcc-[0-9]* 2>/dev/null | sort -t- -k2 -n | tail -1),gcc)

all: easyscript $(RELEASE_DIR)/unit_tests

$(EMBED): tools/embed.c
	@mkdir -p $(dir $@)
	$(CC) -std=c11 -Wall -Wextra -O2 -o $@ $<

$(GEN_RUNTIME): $(RUNTIME) $(EMBED)
	@mkdir -p $(dir $@)
	$(EMBED) $@ es_runtime_source $(RUNTIME)

easyscript: $(call release_objs,$(COMPILER_SRC))
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS)

$(RELEASE_DIR)/unit_tests: $(call release_objs,$(UNIT_SRC))
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS)

$(RELEASE_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -MMD -MP -c $< -o $@

# Sanitizer build of the compiler and unit tests, in build/debug/.
debug: $(DEBUG_DIR)/easyscript $(DEBUG_DIR)/unit_tests

$(DEBUG_DIR)/easyscript: $(call debug_objs,$(COMPILER_SRC))
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^ $(LDLIBS)

$(DEBUG_DIR)/unit_tests: $(call debug_objs,$(UNIT_SRC))
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^ $(LDLIBS)

$(DEBUG_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -MMD -MP -c $< -o $@

test: all
	sh tests/run.sh

# The compiler built with GCC and -Werror (in build/gcc/), then the full suite
# against it, with the strict-C check on generated programs also using GCC.
# Catches GCC-only warnings before CI's Linux run does. (On macOS, GCC uses
# Apple's headers, so a missing #include is caught by audit/includes instead.)
$(GCC_DIR)/easyscript: $(call gcc_objs,$(COMPILER_SRC))
	$(GCC) $(CFLAGS) $(RELEASE_FLAGS) -Werror -o $@ $^ $(LDLIBS)

$(GCC_DIR)/unit_tests: $(call gcc_objs,$(UNIT_SRC))
	$(GCC) $(CFLAGS) $(RELEASE_FLAGS) -Werror -o $@ $^ $(LDLIBS)

$(GCC_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) $(RELEASE_FLAGS) -Werror -MMD -MP -c $< -o $@

test-gcc: all $(GCC_DIR)/easyscript $(GCC_DIR)/unit_tests
	STRICT_CC=$(GCC) ES=$(CURDIR)/$(GCC_DIR)/easyscript UNIT=$(CURDIR)/$(GCC_DIR)/unit_tests sh tests/run.sh

# Full suite against the sanitizer build, with every generated program also
# built with sanitizers (ES_SANITIZE=1), then a leak check. On Linux,
# LeakSanitizer (on by default) checks every process in the suite, compiler
# and programs; macOS's AddressSanitizer can't, so there tests/leaks.sh runs
# every test input and generated program under the system `leaks` tool.
test-debug: debug all
	ES_SANITIZE=1 ES=$(CURDIR)/$(DEBUG_DIR)/easyscript UNIT=$(CURDIR)/$(DEBUG_DIR)/unit_tests sh tests/run.sh
	@if [ "$$(uname)" = Darwin ]; then sh tests/leaks.sh; fi

# Rewrites every golden file from the current output and shows git diff --stat.
# Only use it after confirming the new output is intended.
bless: all
	sh tests/bless.sh

# Builds and times the programs in benchmarks/ (EasyScript, C, Go, Python).
bench: all
	sh benchmarks/run.sh

clean:
	rm -rf build easyscript

-include $(shell find build -name '*.d' 2>/dev/null)

.PHONY: all debug test test-debug test-gcc bless bench clean
