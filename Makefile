CC = cc
CFLAGS = -Wall -Wextra -Isrc
RELEASE_FLAGS = -O2
DEBUG_FLAGS = -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all

COMMON_SRC = src/common/arena.c src/common/util.c src/common/diag.c src/common/ast.c
FRONT_SRC = src/front/lexer.c src/front/parse_util.c src/front/parse_expr.c src/front/parse_stmt.c
LEGACY_SRC = src/legacy.c src/lexer.c src/parser.c src/codegen.c
COMPILER_SRC = main.c $(FRONT_SRC) $(LEGACY_SRC) $(COMMON_SRC)
UNIT_SRC = $(wildcard tests/unit/*.c) $(FRONT_SRC) $(COMMON_SRC)

RELEASE_DIR = build/release
DEBUG_DIR = build/debug

release_objs = $(patsubst %.c,$(RELEASE_DIR)/%.o,$(1))
debug_objs = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(1))

all: easyscript $(RELEASE_DIR)/unit_tests

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

# Full suite against the sanitizer build. Leak checking is off until the
# lexer, parser, and codegen allocate from the arena instead of malloc.
test-debug: debug
	ES=$(CURDIR)/$(DEBUG_DIR)/easyscript UNIT=$(CURDIR)/$(DEBUG_DIR)/unit_tests \
	ASAN_OPTIONS=detect_leaks=0 sh tests/run.sh

# Rewrites every golden file from the current output and shows git diff --stat.
# Only use it after confirming the new output is intended.
bless: all
	sh tests/bless.sh

clean:
	rm -rf build easyscript

-include $(shell find build -name '*.d' 2>/dev/null)

.PHONY: all debug test test-debug bless clean
