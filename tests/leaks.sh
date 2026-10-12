#!/bin/sh
# Leak sweep for macOS, where AddressSanitizer can't detect leaks: runs the
# compiler under the system `leaks` tool for every test input, in the mode
# its folder uses (tokens, ast, emit, run), plus every generated program
# (built without sanitizers, which `leaks` can't inspect) and every shell
# session. Fails if any process leaks. On Linux, `make
# test-debug` gets the same check from LeakSanitizer instead.
#
# Run by `make test-debug` when `leaks` is available.

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
ES=${ES:-$ROOT/easyscript}
WORK=$(mktemp -d "${TMPDIR:-/tmp}/easyscript-leaks-XXXXXX")
trap 'rm -rf "$WORK"' EXIT

if ! command -v leaks >/dev/null 2>&1; then
    echo "SKIP leak sweep (no leaks tool; LeakSanitizer covers Linux)"
    exit 0
fi

checked=0
leaky=0

# The same fixed clock and random numbers as tests/run.sh, so programs with
# events and waits don't wait for real (they're built with the test hooks).
export ES_NOW="2026-10-11 14:05:00"
export ES_RANDOM_SEED=7

# check LABEL INPUT CMD...: runs CMD under leaks with INPUT as stdin.
check() {
    label=$1 input=$2
    shift 2
    rm -rf "$WORK/cwd"
    mkdir "$WORK/cwd"
    (cd "$WORK/cwd" && leaks --atExit -- "$@") <"$input" >"$WORK/out" 2>&1
    checked=$((checked + 1))
    if ! grep -q ': 0 leaks for 0 total leaked bytes' "$WORK/out"; then
        leaky=$((leaky + 1))
        echo "LEAK $label"
        grep -E 'leaks for|^Leak:' "$WORK/out" | head -5 | sed 's/^/    /'
    fi
}

input_for() {
    if [ -f "${1%.es}.in" ]; then echo "${1%.es}.in"; else echo /dev/null; fi
}

sweep() {  # sweep MODE FOLDER...
    mode=$1
    shift
    for dir in "$@"; do
        for src in "$dir"/*.es; do
            [ -e "$src" ] || continue
            check "$mode ${src#$ROOT/}" "$(input_for "$src")" "$ES" "$mode" "$src"
        done
    done
}

sweep tokens "$ROOT/tests/tokens" "$ROOT/examples/lexer"
sweep ast "$ROOT/tests/ast" "$ROOT/examples/parser"
sweep emit "$ROOT/tests/errors" "$ROOT/tests/warnings" "$ROOT/tests/run" "$ROOT/examples/programs" \
    "$ROOT"/tests/modules/*

# Generated programs (the runtime and its reference counting), and the
# million-round memory tests. Programs that stop with a runtime error (they
# have a .err) are left out: by design they exit without releasing anything,
# so whether `leaks` still finds a pointer to a variable's list or text
# depends only on which registers the C compiler reused before exit. Their
# compiler runs are still checked above.
for src in "$ROOT"/tests/run/*.es "$ROOT"/examples/programs/*.es "$ROOT"/tests/memory/*.es "$ROOT"/tests/warnings/*.es \
    "$ROOT"/tests/modules/*/main.es "$ROOT"/examples/programs/*/main.es; do
    case $src in */tests/warnings/*) ;; *) [ -f "${src%.es}.err" ] && continue ;; esac  # a warning's .err is its warnings
    ES_SANITIZE=0 "$ES" emit "$src" >"$WORK/program.c" 2>/dev/null || continue
    ${CC:-cc} -O2 -DES_HOOKS "$WORK/program.c" -o "$WORK/program" -lm || continue
    words=""  # NAME.args: the words given to the program
    [ -f "${src%.es}.args" ] && words=$(cat "${src%.es}.args")
    # shellcheck disable=SC2086
    check "program ${src#$ROOT/}" "$(input_for "$src")" "$WORK/program" $words
done

# Interactive shell sessions.
for input in "$ROOT"/tests/shell/*.in; do
    [ -e "$input" ] || continue
    check "shell ${input#$ROOT/}" "$input" "$ES"
done

echo "leak sweep: $checked processes, $leaky leaking"
[ "$leaky" -eq 0 ]
