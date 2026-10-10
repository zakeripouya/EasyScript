#!/bin/sh
# EasyScript test runner.
#
#   tests/run/NAME.es     compiled and run (stdin from NAME.in if present);
#                         stdout must match NAME.out. With NAME.err it must
#                         exit 1 with that stderr, otherwise exit 0 silently.
#                         Run with ES_DEBUG_MEMORY=1: no heap text may be
#                         alive at the end
#   tests/errors/NAME.es  compiled (emit); must fail, and stderr must match
#                         NAME.err exactly
#   tests/tokens/NAME.es  lexed with `easyscript tokens`; stdout must match
#                         NAME.out. If NAME.err exists the run must fail and
#                         stderr must match it; otherwise it must succeed with
#                         empty stderr
#   tests/memory/NAME.es  built text in big loops, run under a memory limit
#                         (see the Memory section below)
#   tests/shell/NAME.in   typed into the interactive shell; stdout must match
#                         NAME.out and stderr NAME.err (empty if none)
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
# error); without one it must exit 0 with empty stderr. Programs run with
# ES_DEBUG_MEMORY=1, so one that ends with heap text still alive fails.
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
        (cd "$WORK/cwd" && ES_DEBUG_MEMORY=1 "$ES" run "$src") <"$input" >"$WORK/stdout" 2>"$WORK/stderr"
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

# --- Memory -------------------------------------------------------------------
#
# tests/memory/NAME.es builds text in loops of up to a million rounds. Each is
# built, then run with ES_DEBUG_MEMORY=1 and ES_MEMORY_LIMIT=MEMORY_LIMIT, so
# the runtime stops it (exit 70) if live heap text ever takes more than that.
# Stdout must match NAME.out; with NAME.err it must exit 70 with that stderr.
# Where /usr/bin/time can report it, the program's peak memory (resident set
# size) must also stay under MAX_PEAK_KB, so memory really goes back (not
# under ES_SANITIZE=1, where AddressSanitizer's bookkeeping takes memory).

MEMORY_LIMIT=100000
MAX_PEAK_KB=16384

# Peak resident set size in KB from /usr/bin/time's report, or empty.
peak_kb() {
    if [ "$(uname)" = Darwin ]; then
        awk '/maximum resident set size/{printf "%d", $1 / 1024}' "$1"
    else
        awk -F': ' '/Maximum resident set size/{printf "%d", $2}' "$1"
    fi
}

timer=""
if [ -x /usr/bin/time ]; then
    if [ "$(uname)" = Darwin ]; then timer="/usr/bin/time -l"; else timer="/usr/bin/time -v"; fi
fi

for src in "$ROOT"/tests/memory/*.es; do
    [ -e "$src" ] || continue
    name="memory/$(basename "$src" .es)"
    expected="${src%.es}.out"
    expected_err="${src%.es}.err"
    input="${src%.es}.in"
    [ -f "$input" ] || input=/dev/null
    fresh_dir
    if ! "$ES" build "$src" -o "$WORK/program" 2>"$WORK/stderr"; then
        not_ok "$name" "$(cat "$WORK/stderr")"
        continue
    fi
    rm -f "$WORK/time"
    (cd "$WORK/cwd" && ES_DEBUG_MEMORY=1 ES_MEMORY_LIMIT=$MEMORY_LIMIT $timer ${timer:+-o "$WORK/time"} \
        "$WORK/program") <"$input" >"$WORK/stdout" 2>"$WORK/stderr"
    status=$?
    if [ -f "$expected_err" ]; then want_status=70; else want_status=0; expected_err=/dev/null; fi
    peak=""
    [ -f "$WORK/time" ] && peak=$(peak_kb "$WORK/time")
    if [ $status -ne $want_status ]; then
        not_ok "$name" "exit status $status, expected $want_status
$(cat "$WORK/stderr")"
    elif ! out=$(diff -u "$expected" "$WORK/stdout"); then
        not_ok "$name" "$out"
    elif ! out=$(diff -u "$expected_err" "$WORK/stderr"); then
        not_ok "$name" "$out"
    elif [ $want_status -eq 0 ] && [ -n "$peak" ] && [ "${ES_SANITIZE:-0}" != 1 ] && [ "$peak" -gt $MAX_PEAK_KB ]; then
        not_ok "$name" "peak memory ${peak} KB, more than ${MAX_PEAK_KB} KB"
    else
        ok "$name"
    fi
done

# --- Interactive shell --------------------------------------------------------
#
# tests/shell/NAME.in is typed into `easyscript` (the shell); its stdout must
# match NAME.out, and its stderr NAME.err (or be empty if there's no .err).

for input in "$ROOT"/tests/shell/*.in; do
    [ -e "$input" ] || continue
    name="shell/$(basename "$input" .in)"
    expected_out="${input%.in}.out"
    expected_err="${input%.in}.err"
    [ -f "$expected_err" ] || expected_err=/dev/null
    if [ ! -f "$expected_out" ]; then
        not_ok "$name" "missing $(basename "$expected_out")"
        continue
    fi
    fresh_dir
    (cd "$WORK/cwd" && "$ES") <"$input" >"$WORK/stdout" 2>"$WORK/stderr"
    status=$?
    if [ $status -ne 0 ]; then
        not_ok "$name" "exit status $status"
    elif ! out=$(diff -u "$expected_out" "$WORK/stdout"); then
        not_ok "$name" "$out"
    elif ! out=$(diff -u "$expected_err" "$WORK/stderr"); then
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

expect_failure "cli/missing-file" 1 "I couldn't open the file \"nope.es\": it doesn't exist." \
    "$ES" run nope.es
expect_failure "cli/folder-as-file" 1 "I couldn't read the file \"$ROOT/tests\": it's a folder, not a file." \
    "$ES" run "$ROOT/tests"
expect_failure "cli/unknown-command" 2 "I don't know the command \"frobnicate\"." \
    "$ES" frobnicate "$HELLO"
expect_failure "cli/no-source" 2 "Tell me which .es file to use." \
    "$ES" run
expect_failure "cli/build-without-o" 2 "Tell me what to call the program with -o, like \"easyscript build hello.es -o hello\"." \
    "$ES" build "$HELLO"
expect_failure "cli/o-without-build" 2 "-o only works with build." \
    "$ES" run "$HELLO" -o x
expect_failure "cli/o-without-path" 2 "After -o, give the name of the program to make." \
    "$ES" build "$HELLO" -o
expect_failure "cli/o-twice" 2 "Give -o only once." \
    "$ES" build "$HELLO" -o x -o y
expect_failure "cli/two-sources" 2 "Give one .es file at a time." \
    "$ES" run "$HELLO" "$HELLO"

# The memory check itself must catch a leak: the same program with one
# release taken out of the runtime (in es_say) has to fail it.
leak_control() {
    printf 'let n be 5\nsay "n is " followed by n\n' >leak.es &&
        "$ES" emit leak.es >leak.c &&
        awk 'skip { skip = 0; if ($0 ~ /es_release\(t\);/) next } { print } /es_out\("\\n", 1\);/ { skip = 1 }' \
            leak.c >broken.c &&
        ! cmp -s leak.c broken.c &&
        cc -O2 broken.c -o broken -lm 2>/dev/null &&
        ES_DEBUG_MEMORY=1 ./broken
}
expect_failure "memory/check-catches-a-leak" 70 \
    "Memory check: 1 text value was still alive at the end of the program (39 bytes)." leak_control

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
check_readme_block "## A bigger example" "$ROOT/examples/programs/taste.es" "docs/readme-bigger-example"

# --- Audits ---------------------------------------------------------------------
#
# Every error message in the source must be produced by some test, and every
# phrase the parser accepts must be in docs/vocabulary.md (see tests/tools/).

audit() {
    if ! command -v python3 >/dev/null 2>&1; then
        echo "SKIP $1 (needs python3)"
        return
    fi
    if out=$(python3 "$ROOT/tests/tools/$2" 2>&1); then
        ok "$1"
    else
        not_ok "$1" "$out"
    fi
}

audit "audit/every-error-message-tested" error_coverage.py
audit "audit/every-phrase-documented" vocab_coverage.py

# --- Summary ----------------------------------------------------------------

echo
echo "$pass passed, $fail failed"
if [ $fail -ne 0 ]; then
    echo "Failed:$failed"
    exit 1
fi
