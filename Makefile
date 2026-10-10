CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Isrc
RELEASE_FLAGS = -O2
DEBUG_FLAGS = -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all

COMMON_SRC = src/common/arena.c src/common/util.c src/common/diag.c src/common/ast.c
FRONT_SRC = src/front/lexer.c src/front/parse_util.c src/front/parse_expr.c src/front/parse_stmt.c src/front/parse_if.c src/front/parse_loop.c src/front/parse_func.c src/front/consteval.c src/front/check.c src/front/check_const.c
BACK_SRC = src/back/codegen_c.c src/back/codegen_expr.c
# The runtime, embedded into the compiler as a byte array by tools/embed.c.
GEN_RUNTIME = build/gen/es_runtime_embed.c
EMBED = build/tools/embed
COMPILER_SRC = src/main.c src/shell.c $(FRONT_SRC) $(BACK_SRC) $(COMMON_SRC) $(GEN_RUNTIME)
UNIT_SRC = $(wildcard tests/unit/*.c) $(FRONT_SRC) $(COMMON_SRC)

RELEASE_DIR = build/release
DEBUG_DIR = build/debug

release_objs = $(patsubst %.c,$(RELEASE_DIR)/%.o,$(1))
debug_objs = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(1))

all: easyscript $(RELEASE_DIR)/unit_tests

$(EMBED): tools/embed.c
	@mkdir -p $(dir $@)
	$(CC) -std=c11 -Wall -Wextra -O2 -o $@ $<

$(GEN_RUNTIME): runtime/es_runtime.h $(EMBED)
	@mkdir -p $(dir $@)
	$(EMBED) runtime/es_runtime.h $@ es_runtime_source

easyscript: $(call release_objs,$(COMPILER_SRC))
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^

$(RELEASE_DIR)/unit_tests: $(call release_objs,$(UNIT_SRC))
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^

$(RELEASE_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -MMD -MP -c $< -o $@

# Sanitizer build of the compiler and unit tests, in build/debug/.
debug: $(DEBUG_DIR)/easyscript $(DEBUG_DIR)/unit_tests

$(DEBUG_DIR)/easyscript: $(call debug_objs,$(COMPILER_SRC))
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^

$(DEBUG_DIR)/unit_tests: $(call debug_objs,$(UNIT_SRC))
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^

$(DEBUG_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -MMD -MP -c $< -o $@

test: all
	sh tests/run.sh

# Full suite against the sanitizer build, then a leak check. On Linux,
# LeakSanitizer (on by default) checks every process in the suite; macOS's
# AddressSanitizer can't, so there tests/leaks.sh runs every test input
# under the system `leaks` tool instead.
test-debug: debug all
	ES=$(CURDIR)/$(DEBUG_DIR)/easyscript UNIT=$(CURDIR)/$(DEBUG_DIR)/unit_tests sh tests/run.sh
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

.PHONY: all debug test test-debug bless bench clean
