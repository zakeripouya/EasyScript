#!/bin/sh
# Speed check: `make bench-check`.
#
# Runs `make bench` (benchmarks/run.sh, each program 3 times, keeping the
# fastest) and compares EasyScript's times with the "Latest results" table
# in docs/performance.md. Fails if any benchmark got more than 20% slower.
# The timer's resolution is 10 ms, so a benchmark also has to be at least
# 0.02 s slower than recorded to count: a 0.04 s program taking 0.05 s is noise.
#
# BENCH_OUTPUT=FILE compares a saved run instead of running the benchmarks.

set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/build/bench"
mkdir -p "$OUT"
run="${BENCH_OUTPUT:-$OUT/bench-check.txt}"
if [ -z "${BENCH_OUTPUT:-}" ]; then
    sh "$ROOT/benchmarks/run.sh" > "$run" 2>&1 || {
        tail -n 20 "$run"
        echo "bench-check: make bench failed"
        exit 1
    }
fi

# "fib 0.25" for each EasyScript row of the latest results recorded.
recorded=$(awk '/^## Latest results/{on=1; next} /^## /{on=0} on && /^\| [a-z_]+ \| [0-9.]+ s/ {
    split($0, cells, "|"); name = cells[2]; time = cells[3]
    gsub(/ /, "", name); sub(/^ */, "", time); sub(/ s.*/, "", time); print name, time }' "$ROOT/docs/performance.md")

status=0
checked=0
printf "%-14s %9s %9s   %s\n" "benchmark" "recorded" "now" "result"
for pair in $(echo "$recorded" | tr ' ' ':'); do
    name=${pair%%:*}
    was=${pair#*:}
    now=$(awk -v n="$name" '$1 == n && $2 == "easyscript" {print $3; exit}' "$run")
    if [ -z "$now" ]; then
        printf "%-14s %9s %9s   %s\n" "$name" "$was" "-" "not run"
        continue
    fi
    checked=$((checked + 1))
    verdict=$(awk -v a="$was" -v b="$now" 'BEGIN { print (b > a * 1.2 && b - a >= 0.02) ? "SLOWER" : "ok" }')
    printf "%-14s %9s %9s   %s\n" "$name" "$was" "$now" "$verdict"
    [ "$verdict" = ok ] || status=1
done
if [ $checked -eq 0 ]; then
    echo "bench-check: no benchmark could be compared"
    exit 1
fi
[ $status -eq 0 ] && echo "bench-check: every benchmark is within 20% of docs/performance.md" ||
    echo "bench-check: a benchmark is more than 20% slower than docs/performance.md says"
exit $status
