note: running other programs, never through a shell
run "echo" with "hello" and call the result greeting
say greeting's output
say greeting's code
if greeting worked, say "it worked"
run "echo" with "a; echo b", "$HOME", "*" and 3 and call it quoted
say quoted's output
let words be a list of "one", "two" and "three"
run "echo" with "start", words and "end" and call the result listed
say listed's output
run "sh" with "-c" and "echo out; echo err >&2; exit 3" and call the result r
say "output: " followed by r's output
say "error output: " followed by r's error output
say "code: " followed by r's code
if r didn't work, say "it didn't work"
if r did not work, say "still didn't"
say r
run "printf" with "two\nlines\n" and call it lines
say the lines of lines's output
run "true".
run "sh" with "-c" and "echo shown directly".
let results be a list of greeting and r
for each result in results:
    say result's code
to status of program (text):
    run program and call the result attempt
    if attempt worked:
        give back "fine"
    give back "broken"
say status of "true"
say status of "false"
