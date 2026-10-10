#!/bin/sh
# EasyScript test runner.
#
#   tests/run/NAME.es     compiled and run (stdin from NAME.in if present);
#                         stdout must match NAME.out. With NAME.err it must
#                         exit 1 with that stderr, otherwise exit 0 silently
#   tests/errors/NAME.es  compiled (emit); must fail, and stderr must match
#                         NAME.err exactly
#   tests/tokens/NAME.es  lexed with `easyscript tokens`; stdout must match
#                         NAME.out. If NAME.err exists the run must fail and
#                         stderr must match it; otherwise it must succeed with
#                         empty stderr
#   examples/programs/    like tests/run
#   tests/ast/NAME.es     like tests/tokens, but with `easyscript ast`
#   examples/lexer/       like tests/tokens, expected stdout in NAME.tokens
#   examples/parser/      like tests/ast, expected stdout in NAME.ast
#   unit tests            tests/unit/*.c, built into the UNIT binary
#   CLI checks            at the bottom of this file
#
# Programs run in a scratch directory, so nothing is written into the repo.
# ES and UNIT override the binaries under test (make test-debug uses this).

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
ES=${ES:-$ROOT/easyscript}
UNIT=${UNIT:-$ROOT/build/release/unit_tests}
WORK=$(mktemp -d "${TMPDIR:-/tmp}/easyscript-test-XXXXXX")
trap 'rm -rf "$WORK"' EXIT

for bin in "$ES" "$UNIT"; do
    if [ ! -x "$bin" ]; then
        echo "error: $bin not found; run make first" >&2
        exit 1
    fi
done

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

# --- Unit tests -------------------------------------------------------------
#
# The binary prints "RUN name", indented failure details, then "PASS name" or
# "FAIL name". A crash (e.g. a sanitizer report) shows up as a non-zero exit
# with no matching verdict line.

"$UNIT" >"$WORK/unit" 2>&1
unit_status=$?
details=""
unit_fails=0
while IFS= read -r line; do
    case $line in
        "RUN  "*) details="" ;;
        "PASS "*) ok "unit/${line#PASS }" ;;
        "FAIL "*) not_ok "unit/${line#FAIL }" "$details"; unit_fails=$((unit_fails + 1)) ;;
        *) details="$details${details:+
}${line#    }" ;;
    esac
done <"$WORK/unit"
if [ $unit_status -ne 0 ] && [ $unit_fails -eq 0 ]; then
    not_ok "unit" "exit status $unit_status
$(tail -n 40 "$WORK/unit")"
fi

# --- Programs, compile errors, tokens, and examples ------------------------

# check_run DIR LABEL: each DIR/NAME.es is compiled and run, with NAME.in as
# its input if present (otherwise no input). Stdout must match NAME.out. With
# a NAME.err the program must exit 1 with exactly that stderr (a runtime
# error); without one it must exit 0 with empty stderr.
check_run() {
    for src in "$1"/*.es; do
        [ -e "$src" ] || continue
        name="$2/$(basename "$src" .es)"
        expected="${src%.es}.out"
        expected_err="${src%.es}.err"
        input="${src%.es}.in"
        [ -f "$input" ] || input=/dev/null
        if [ ! -f "$expected" ]; then
            not_ok "$name" "missing $(basename "$expected")"
            continue
        fi
        fresh_dir
        (cd "$WORK/cwd" && "$ES" run "$src") <"$input" >"$WORK/stdout" 2>"$WORK/stderr"
        status=$?
        if [ -f "$expected_err" ]; then
            want_status=1
        else
            want_status=0
            expected_err=/dev/null
        fi
        if [ $status -ne $want_status ]; then
            not_ok "$name" "exit status $status, expected $want_status
$(cat "$WORK/stderr")"
        elif ! out=$(diff -u "$expected" "$WORK/stdout"); then
            not_ok "$name" "$out"
        elif ! out=$(diff -u "$expected_err" "$WORK/stderr"); then
            not_ok "$name" "$out"
        else
            ok "$name"
        fi
    done
}

# check_errors DIR LABEL: each DIR/NAME.es must fail to compile with
# stderr matching NAME.err.
check_errors() {
    for src in "$1"/*.es; do
        [ -e "$src" ] || continue
        name="$2/$(basename "$src" .es)"
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
}

# check_dump COMMAND DIR LABEL OUT_EXT: runs `easyscript COMMAND` on each
# DIR/NAME.es; stdout must match NAME.OUT_EXT. With a NAME.err the run must
# fail with exactly that stderr; without one it must succeed with empty stderr.
check_dump() {
    cmd=$1
    shift
    for src in "$1"/*.es; do
        [ -e "$src" ] || continue
        name="$2/$(basename "$src" .es)"
        expected_out="${src%.es}.$3"
        expected_err="${src%.es}.err"
        if [ ! -f "$expected_out" ]; then
            not_ok "$name" "missing $(basename "$expected_out")"
            continue
        fi
        fresh_dir
        (cd "$WORK/cwd" && "$ES" "$cmd" "$src") >"$WORK/stdout" 2>"$WORK/stderr"
        status=$?
        if [ -f "$expected_err" ]; then
            want_status=1
        else
            want_status=0
            expected_err=/dev/null
        fi
        if [ $status -ne $want_status ]; then
            not_ok "$name" "exit status $status, expected $want_status
$(cat "$WORK/stderr")"
        elif ! out=$(diff -u "$expected_out" "$WORK/stdout"); then
            not_ok "$name" "$out"
        elif ! out=$(diff -u "$expected_err" "$WORK/stderr"); then
            not_ok "$name" "$out"
        else
            ok "$name"
        fi
    done
}

check_run "$ROOT/tests/run" run
check_errors "$ROOT/tests/errors" errors
check_dump tokens "$ROOT/tests/tokens" tokens out
check_dump ast "$ROOT/tests/ast" ast out

# Examples back the docs, so every one of them must be checked.
check_run "$ROOT/examples/programs" examples/programs
check_dump tokens "$ROOT/examples/lexer" examples/lexer tokens
check_dump ast "$ROOT/examples/parser" examples/parser ast
for dir in "$ROOT"/examples/*/; do
    [ -d "$dir" ] || continue
    case $(basename "$dir") in
        programs|lexer|parser) ;;
        *) not_ok "examples/$(basename "$dir")" "no test runner covers this directory; add it to tests/run.sh" ;;
    esac
done
for src in "$ROOT"/examples/*.es; do
    [ -e "$src" ] || continue
    not_ok "examples/$(basename "$src")" "examples must live in examples/programs/, examples/lexer/ or examples/parser/"
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
elif ! echo "$out" | grep -q '^int main(void) {$'; then
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

# --- Docs that must match tested files ---------------------------------------

# readme_block HEADING: the first code block after the README heading.
readme_block() {
    awk -v heading="$1" '
        index($0, heading) == 1 { found = 1; next }
        found && /^```/ { fences++; if (fences == 2) exit; next }
        found && fences == 1 { print }
    ' "$ROOT/README.md"
}

check_readme_block() {
    readme_block "$1" >"$WORK/readme_block"
    if ! out=$(diff -u "$2" "$WORK/readme_block"); then
        not_ok "$3" "README block under \"$1\" differs from $(basename "$2"):
$out"
    else
        ok "$3"
    fi
}

check_readme_block "## A first program" "$ROOT/examples/programs/first_program.es" "docs/readme-first-program"
check_readme_block "## A taste of what's coming" "$ROOT/examples/lexer/taste.es" "docs/readme-taste"

# --- Summary ----------------------------------------------------------------

echo
echo "$pass passed, $fail failed"
if [ $fail -ne 0 ]; then
    echo "Failed:$failed"
    exit 1
fi
