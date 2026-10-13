#!/bin/sh
# run_attempt.sh TASKDIR CODE.es OUT.txt : runs one try the way plan.md says
ES=${ES:-/private/tmp/claude-501/-Users-pouyazakeri-Documents-GitHub-EasyScript/30cf7dc8-b355-4cbc-ae81-01b2eae4042c/scratchpad/es-master}
task=$(cd "$1" && pwd); code=$2; out=$3
work=$(mktemp -d)
for f in "$task"/*; do case "$(basename "$f")" in task.md|answers.txt) ;; *) cp "$f" "$work/";; esac; done
cp "$code" "$work/main.es"
ans=/dev/null; [ -f "$task/answers.txt" ] && ans="$task/answers.txt"
before=$(ls "$work")
( cd "$work" && ES_HOOKS=1 ES_NOW="2026-10-11 14:05:00" ES_RANDOM_SEED=7 perl -e 'alarm 10; exec @ARGV' "$ES" run main.es < "$ans" > stdout.txt 2> stderr.txt; echo $? > code.txt )
{
  echo "exit code: $(cat "$work/code.txt")"
  echo "--- output ---"; cat "$work/stdout.txt"
  echo "--- errors ---"; cat "$work/stderr.txt"
  echo "--- files made ---"
  ls "$work" | while IFS= read -r f; do if ! printf '%s
main.es
stdout.txt
stderr.txt
code.txt
' "$before" | grep -Fxq "$f"; then mkdir -p "${out%.txt}-files"; cp "$work/$f" "${out%.txt}-files/"; echo "$f ($(wc -c < "$work/$f" | tr -d ' ') bytes)"; fi; done
} > "$out"
rm -rf "$work"
