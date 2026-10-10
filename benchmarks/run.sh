#!/bin/sh
# Benchmarks: `make bench`.
#
# Each folder in benchmarks/ holds one program written in EasyScript (.es)
# and, where available, Python (.py), Go (.go), and C (.c). Every version is
# built first (build time isn't measured), then run under /usr/bin/time
# for wall-clock time and peak memory (maximum resident set size), three times,
# keeping the fastest (BENCH_RUNS=N changes that). Every
# version must print the same result as the EasyScript one.
#
# Languages whose tools aren't installed are skipped.

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
ES="$ROOT/easyscript"
OUT="$ROOT/build/bench"
RUNS=${BENCH_RUNS:-3}
mkdir -p "$OUT"

if [ ! -x "$ES" ]; then
    echo "error: $ES not found; run make first" >&2
    exit 1
fi

have() { command -v "$1" >/dev/null 2>&1; }

# measure BIN [ARGS...]: runs it, sets SECONDS_TAKEN, PEAK_MB, RESULT, STATUS.
measure() {
    best=""
    i=0
    while [ $i -lt "$RUNS" ]; do
        if [ "$(uname)" = Darwin ]; then
            /usr/bin/time -l "$@" >"$OUT/stdout" 2>"$OUT/time"
            STATUS=$?
            secs=$(awk '/ real /{print $1}' "$OUT/time")
            peak=$(awk '/maximum resident set size/{printf "%.1f", $1 / 1048576}' "$OUT/time")
        else
            /usr/bin/time -v "$@" >"$OUT/stdout" 2>"$OUT/time"
            STATUS=$?
            secs=$(awk -F': ' '/Elapsed \(wall clock\)/{n = split($2, t, ":"); s = 0; for (k = 1; k <= n; k++) s = s * 60 + t[k]; printf "%.2f", s}' "$OUT/time")
            peak=$(awk -F': ' '/Maximum resident set size/{printf "%.1f", $2 / 1024}' "$OUT/time")
        fi
        if [ -z "$best" ] || awk -v a="$secs" -v b="$best" 'BEGIN{exit !(a < b)}'; then
            best=$secs
            PEAK_MB=$peak
        fi
        i=$((i + 1))
    done
    SECONDS_TAKEN=$best
    RESULT=$(head -n 1 "$OUT/stdout")
}

row() {
    printf "%-8s %-11s %9s %11s   %s\n" "$1" "$2" "$3" "$4" "$5"
}

failed=0
printf "%-8s %-11s %9s %11s   %s\n" "program" "language" "time (s)" "memory (MB)" "result"
printf "%-8s %-11s %9s %11s   %s\n" "-------" "--------" "--------" "-----------" "------"

for dir in "$ROOT"/benchmarks/*/; do
    name=$(basename "$dir")
    expected=""
    for lang in easyscript c go python; do
        case $lang in
            easyscript) src="$dir$name.es"; bin="$OUT/${name}_es" ;;
            c) src="$dir$name.c"; bin="$OUT/${name}_c" ;;
            go) src="$dir$name.go"; bin="$OUT/${name}_go" ;;
            python) src="$dir$name.py"; bin="" ;;
        esac
        [ -f "$src" ] || continue
        case $lang in
            easyscript) "$ES" build "$src" -o "$bin" || { failed=1; continue; } ;;
            c) have cc || { row "$name" c - - "(cc not installed)"; continue; }
               cc -O2 "$src" -o "$bin" || { failed=1; continue; } ;;
            go) have go || { row "$name" go - - "(go not installed)"; continue; }
                go build -o "$bin" "$src" || { failed=1; continue; } ;;
            python) have python3 || { row "$name" python - - "(python3 not installed)"; continue; } ;;
        esac
        if [ "$lang" = python ]; then
            measure python3 "$src"
        else
            measure "$bin"
        fi
        note=$RESULT
        if [ "$STATUS" -ne 0 ]; then
            note="FAILED (exit $STATUS)"
            failed=1
        elif [ -z "$expected" ]; then
            expected=$RESULT
        elif [ "$RESULT" != "$expected" ]; then
            note="$RESULT  (MISMATCH: expected $expected)"
            failed=1
        fi
        row "$name" "$lang" "$SECONDS_TAKEN" "$PEAK_MB" "$note"
    done
done

echo
echo "Machine: $(uname -sm); $(cc --version 2>/dev/null | head -n 1)"
exit $failed
