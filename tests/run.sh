#!/bin/sh
# EasyScript test runner.
#
#   tests/run/NAME.es     compiled and run; stdout must match NAME.out and the
#                         exit status must be 0
#   tests/errors/NAME.es  compiled (emit); must fail, and stderr must match
#                         NAME.err exactly
#   CLI checks            at the bottom of this file
#
# Programs run in a scratch directory, so nothing is written into the repo.

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
ES="$ROOT/easyscript"
WORK=$(mktemp -d "${TMPDIR:-/tmp}/easyscript-test-XXXXXX")
trap 'rm -rf "$WORK"' EXIT

if [ ! -x "$ES" ]; then
    echo "error: $ES not found; run make first" >&2
    exit 1
fi

pass=0
fail=0
failed=""

ok() {
    pass=$((pass + 1))
    echo "PASS $1"
}

not_ok() {
    fail=$((fail + 1))
    failed="$failed
  $1"
    echo "FAIL $1"
    if [ $# -gt 1 ]; then
        echo "$2" | sed 's/^/    /'
    fi
}

# Fresh empty directory to run a test in.
fresh_dir() {
    rm -rf "$WORK/cwd"
    mkdir "$WORK/cwd"
}

for src in "$ROOT"/tests/run/*.es; do
    [ -e "$src" ] || continue
    name="run/$(basename "$src" .es)"
    expected="${src%.es}.out"
    if [ ! -f "$expected" ]; then
        not_ok "$name" "missing $(basename "$expected")"
        continue
    fi
    fresh_dir
    (cd "$WORK/cwd" && "$ES" run "$src") >"$WORK/stdout" 2>"$WORK/stderr"
    status=$?
    if [ $status -ne 0 ]; then
        not_ok "$name" "exit status $status
$(cat "$WORK/stderr")"
    elif ! out=$(diff -u "$expected" "$WORK/stdout"); then
        not_ok "$name" "$out"
    else
        ok "$name"
    fi
done

for src in "$ROOT"/tests/errors/*.es; do
    [ -e "$src" ] || continue
    name="errors/$(basename "$src" .es)"
    expected="${src%.es}.err"
    if [ ! -f "$expected" ]; then
        not_ok "$name" "missing $(basename "$expected")"
        continue
    fi
    fresh_dir
    (cd "$WORK/cwd" && "$ES" emit "$src") >/dev/null 2>"$WORK/stderr"
    status=$?
    if [ $status -eq 0 ]; then
        not_ok "$name" "compiled successfully but an error was expected"
    elif ! out=$(diff -u "$expected" "$WORK/stderr"); then
        not_ok "$name" "$out"
    else
        ok "$name"
    fi
done

# --- CLI checks -------------------------------------------------------------

HELLO="$ROOT/tests/run/print_string.es"

# Asserts that the scratch directory is still empty after a command.
check_cwd_empty() {
    if [ -n "$(ls -A "$WORK/cwd")" ]; then
        not_ok "$1" "left files in the working directory: $(ls -A "$WORK/cwd" | tr '\n' ' ')"
        return 1
    fi
    return 0
}

fresh_dir
out=$(cd "$WORK/cwd" && "$ES" emit "$HELLO" 2>&1)
if [ $? -ne 0 ]; then
    not_ok "cli/emit" "$out"
elif ! echo "$out" | grep -q '^int main() {$'; then
    not_ok "cli/emit" "output does not look like C:
$out"
elif check_cwd_empty "cli/emit"; then
    ok "cli/emit"
fi

fresh_dir
out=$(cd "$WORK/cwd" && "$ES" build "$HELLO" -o hello 2>&1)
if [ $? -ne 0 ]; then
    not_ok "cli/build" "$out"
elif [ ! -x "$WORK/cwd/hello" ]; then
    not_ok "cli/build" "no executable at -o path"
elif [ "$(ls -A "$WORK/cwd")" != "hello" ]; then
    not_ok "cli/build" "unexpected files: $(ls -A "$WORK/cwd" | tr '\n' ' ')"
elif [ "$("$WORK/cwd/hello")" != "hello" ]; then
    not_ok "cli/build" "built program printed: $("$WORK/cwd/hello")"
else
    ok "cli/build"
fi

fresh_dir
out=$(cd "$WORK/cwd" && "$ES" run "$HELLO" 2>&1)
if [ $? -ne 0 ] || [ "$out" != "hello" ]; then
    not_ok "cli/run" "$out"
elif check_cwd_empty "cli/run"; then
    ok "cli/run"
fi

# Runs a command that must fail with the given status and stderr first line.
expect_failure() {
    name=$1 want_status=$2 want_line=$3
    shift 3
    fresh_dir
    (cd "$WORK/cwd" && "$@") >/dev/null 2>"$WORK/stderr"
    status=$?
    line=$(head -n 1 "$WORK/stderr")
    if [ $status -ne "$want_status" ]; then
        not_ok "$name" "exit status $status, expected $want_status
$(cat "$WORK/stderr")"
    elif [ "$line" != "$want_line" ]; then
        not_ok "$name" "stderr: $line
expected: $want_line"
    else
        ok "$name"
    fi
}

expect_failure "cli/missing-file" 1 "Error: Unable to open source file nope.es." \
    "$ES" run nope.es
expect_failure "cli/unknown-command" 2 "Error: Unknown command 'frobnicate'." \
    "$ES" frobnicate "$HELLO"
expect_failure "cli/no-source" 2 "Error: No source file given." \
    "$ES" run
expect_failure "cli/build-without-o" 2 "Error: build needs -o OUT." \
    "$ES" build "$HELLO"
expect_failure "cli/o-without-build" 2 "Error: -o is only valid with build." \
    "$ES" run "$HELLO" -o x
expect_failure "cli/two-sources" 2 "Error: Only one source file can be given." \
    "$ES" run "$HELLO" "$HELLO"

# Temp directories made by the compiler must be cleaned up, including on errors.
fresh_dir
TMPDIR="$WORK" "$ES" run "$HELLO" >/dev/null 2>&1
TMPDIR="$WORK" "$ES" emit "$ROOT/tests/errors/undefined_variable.es" >/dev/null 2>&1
leftover=$(find "$WORK" -mindepth 1 -maxdepth 1 -name 'easyscript-*')
if [ -n "$leftover" ]; then
    not_ok "cli/temp-cleanup" "left behind: $leftover"
else
    ok "cli/temp-cleanup"
fi

# --- Summary ----------------------------------------------------------------

echo
echo "$pass passed, $fail failed"
if [ $fail -ne 0 ]; then
    echo "Failed:$failed"
    exit 1
fi
