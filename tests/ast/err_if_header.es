let x be 1
if x is 1
    say "no colon"
if x is 1: say "same line"
if x is 1:
say "nothing indented"
if:
    say "no condition"
if x is 1,
if x is 1 then
if x is 1, if x is 2, say "nested one-line"
if x plus:
    say "broken condition"
otherwise:
    say "belongs to the broken if, no extra error"
say "still checked" "after"
