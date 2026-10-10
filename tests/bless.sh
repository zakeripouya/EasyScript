#!/bin/sh
# Rewrites every golden file (expected .out/.tokens/.ast/.err) from the
# compiler's current output, then shows what changed. Run by `make bless`.
#
# Only bless after reading the failing diffs from `make test` and confirming
# the new output is intended. Review `git diff` before committing.
#
# Unit tests (tests/unit) have no golden files and are not touched.

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
ES=${ES:-$ROOT/easyscript}
WORK=$(mktemp -d "${TMPDIR:-/tmp}/easyscript-bless-XXXXXX")
trap 'rm -rf "$WORK"' EXIT

if [ ! -x "$ES" ]; then
    echo "error: $ES not found; run make first" >&2
    exit 1
fi

warnings=0

warn() {
    warnings=$((warnings + 1))
    echo "NOT BLESSED $1: $2"
}

# Runs `easyscript CMD SRC` in an empty scratch directory.
run_es() {
    rm -rf "$WORK/cwd"
    mkdir "$WORK/cwd"
    (cd "$WORK/cwd" && "$ES" "$1" "$2") >"$WORK/stdout" 2>"$WORK/stderr"
}

# bless_run DIR [ERROR_STATUS]: programs run with NAME.in (if present) as
# input, under the same memory settings as tests/run.sh; stdout goes to
# NAME.out. Exit ERROR_STATUS (default 1, a runtime error) writes stderr to
# NAME.err; exit 0 removes NAME.err. For tests/memory, ERROR_STATUS is 70 (the
# memory limit).
bless_run() {
    error_status=${2:-1}
    limit=""
    [ "$error_status" -eq 70 ] && limit=100000
    for src in "$1"/*.es; do
        [ -e "$src" ] || continue
        input="${src%.es}.in"
        [ -f "$input" ] || input=/dev/null
        rm -rf "$WORK/cwd"
        mkdir "$WORK/cwd"
        (cd "$WORK/cwd" && ES_DEBUG_MEMORY=1 ES_MEMORY_LIMIT=$limit "$ES" run "$src") \
            <"$input" >"$WORK/stdout" 2>"$WORK/stderr"
        status=$?
        if [ $status -eq 0 ] && [ ! -s "$WORK/stderr" ]; then
            cp "$WORK/stdout" "${src%.es}.out"
            rm -f "${src%.es}.err"
        elif [ $status -eq "$error_status" ]; then
            cp "$WORK/stdout" "${src%.es}.out"
            cp "$WORK/stderr" "${src%.es}.err"
        else
            warn "${src#$ROOT/}" "exit status $status (or output on stderr after success), which no golden file can make pass:
$(sed 's/^/    /' "$WORK/stderr")"
        fi
    done
}

# bless_errors DIR: programs that must fail to compile; stderr goes to NAME.err.
bless_errors() {
    for src in "$1"/*.es; do
        [ -e "$src" ] || continue
        if run_es emit "$src"; then
            warn "${src#$ROOT/}" "it compiles now, but tests in this folder must fail; move or fix it"
        else
            cp "$WORK/stderr" "${src%.es}.err"
        fi
    done
}

# bless_dump CMD DIR EXT: stdout goes to NAME.EXT. On failure stderr goes to
# NAME.err; on success any NAME.err is removed.
bless_dump() {
    for src in "$2"/*.es; do
        [ -e "$src" ] || continue
        run_es "$1" "$src"
        status=$?
        cp "$WORK/stdout" "${src%.es}.$3"
        if [ $status -ne 0 ]; then
            cp "$WORK/stderr" "${src%.es}.err"
        elif [ -s "$WORK/stderr" ]; then
            warn "${src#$ROOT/}" "it succeeded but wrote to stderr, which no golden file can make pass"
        else
            rm -f "${src%.es}.err"
        fi
    done
}

# bless_shell: tests/shell/NAME.in typed into the shell; stdout goes to
# NAME.out, stderr to NAME.err (removed when empty).
bless_shell() {
    for input in "$ROOT"/tests/shell/*.in; do
        [ -e "$input" ] || continue
        rm -rf "$WORK/cwd"
        mkdir "$WORK/cwd"
        (cd "$WORK/cwd" && "$ES") <"$input" >"${input%.in}.out" 2>"$WORK/stderr"
        if [ -s "$WORK/stderr" ]; then
            cp "$WORK/stderr" "${input%.in}.err"
        else
            rm -f "${input%.in}.err"
        fi
    done
}

bless_shell
bless_run "$ROOT/tests/run"
bless_run "$ROOT/tests/memory" 70
bless_errors "$ROOT/tests/errors"
bless_dump tokens "$ROOT/tests/tokens" out
bless_dump ast "$ROOT/tests/ast" out
bless_run "$ROOT/examples/programs"
bless_dump tokens "$ROOT/examples/lexer" tokens
bless_dump ast "$ROOT/examples/parser" ast

cd "$ROOT"
echo "Golden files changed (review with git diff before committing):"
changed=$(git ls-files --modified --deleted -- tests examples | grep -E '\.(out|err|tokens|ast)$' | sort -u)
if [ -n "$changed" ]; then
    # shellcheck disable=SC2086
    git --no-pager diff --stat -- $changed
fi
untracked=$(git ls-files --others --exclude-standard -- tests examples | grep -E '\.(out|err|tokens|ast)$')
if [ -n "$untracked" ]; then
    echo "New files:"
    echo "$untracked" | sed 's/^/    /'
fi
if [ $warnings -ne 0 ]; then
    echo "$warnings file(s) could not be blessed; see NOT BLESSED above."
    exit 1
fi
